// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "alsa.h"

#include <piejam/thread/name.h>
#include <piejam/thread/priority.h>

#include <spdlog/spdlog.h>

#include <boost/lockfree/spsc_queue.hpp>

#include <sound/asequencer.h>
#include <sound/asound.h>
#include <sys/ioctl.h>

#include <algorithm>
#include <ctime>

namespace piejam::midi::alsa
{

namespace
{

[[nodiscard]]
auto
open_seq() -> system::device
{
    system::device seq("/dev/snd/seq", system::device::blocking::off);

    int version{};
    if (auto err = seq.ioctl(SNDRV_SEQ_IOCTL_PVERSION, version))
    {
        throw std::system_error(err);
    }

    if (SNDRV_PROTOCOL_INCOMPATIBLE(SNDRV_SEQ_VERSION, version))
    {
        throw std::runtime_error("midi seq incompatible version");
    }

    return seq;
}

[[nodiscard]]
auto
get_client_id(system::device& seq) -> int
{
    int client_id{};

    if (auto err = seq.ioctl(SNDRV_SEQ_IOCTL_CLIENT_ID, client_id))
    {
        throw std::system_error(err);
    }

    return client_id;
}

[[nodiscard]]
auto
make_input_port(system::device& seq, midi_client_id_t client_id) -> midi_port_t
{
    snd_seq_port_info port_info{};
    port_info.addr.client = client_id;
    port_info.type =
        SNDRV_SEQ_PORT_TYPE_APPLICATION | SNDRV_SEQ_PORT_TYPE_MIDI_GENERIC;
    port_info.capability =
        SNDRV_SEQ_PORT_CAP_WRITE | SNDRV_SEQ_PORT_CAP_SUBS_WRITE;
    port_info.flags =
        SNDRV_SEQ_PORT_FLG_TIMESTAMP | SNDRV_SEQ_PORT_FLG_TIME_REAL;

    if (auto err = seq.ioctl(SNDRV_SEQ_IOCTL_CREATE_PORT, port_info))
    {
        throw std::system_error(err);
    }

    return port_info.addr.port;
}

template <class Handler>
void
scan_devices(system::device& seq, Handler&& handler)
{
    snd_seq_client_info client_info{};
    client_info.client = SNDRV_SEQ_CLIENT_SYSTEM;

    while (!seq.ioctl(SNDRV_SEQ_IOCTL_QUERY_NEXT_CLIENT, client_info))
    {
        if (client_info.client == SNDRV_SEQ_CLIENT_SYSTEM)
        {
            continue;
        }

        if (client_info.num_ports <= 0)
        {
            continue;
        }

        snd_seq_port_info port_info{};
        port_info.addr.client = client_info.client;
        port_info.addr.port = 0;

        if (seq.ioctl(SNDRV_SEQ_IOCTL_GET_PORT_INFO, port_info))
        {
            continue;
        }

        do
        {
            std::invoke(handler, client_info, port_info);
        } while (!seq.ioctl(SNDRV_SEQ_IOCTL_QUERY_NEXT_PORT, port_info));
    }
}

[[nodiscard]]
auto
scan_input_devices(system::device& seq) -> std::vector<midi_device>
{
    std::vector<midi_device> result;

    scan_devices(
        seq,
        [&result](
            snd_seq_client_info const& client_info,
            snd_seq_port_info const& port_info) {
            if ((client_info.type == KERNEL_CLIENT) &&
                (port_info.capability & SNDRV_SEQ_PORT_CAP_READ) &&
                (port_info.capability & SNDRV_SEQ_PORT_CAP_SUBS_READ))
            {
                result.emplace_back(
                    midi_device{
                        .client_id = port_info.addr.client,
                        .port = port_info.addr.port,
                        .name = client_info.name});
            }
        });

    return result;
}

constexpr std::size_t input_events_capacity = 1024;

} // namespace

struct midi_io::impl
{
    boost::lockfree::spsc_queue<
        snd_seq_event,
        boost::lockfree::capacity<input_events_capacity>>
        queue;
};

midi_io::midi_io()
    : m_seq(open_seq())
    , m_client_id(get_client_id(m_seq))
    , m_in_port(make_input_port(m_seq, m_client_id))
    , m_input_events(make_pimpl<impl>())
    , m_in_thread([this](std::stop_token stoken) {
        this_thread::set_name("midi_in");
        this_thread::set_realtime_priority(80);

        std::array<snd_seq_event, input_events_capacity> read_event_buffer{};

        while (!stoken.stop_requested())
        {
            auto poll_result = m_seq.poll(std::chrono::milliseconds(100));
            if (poll_result.has_value() && poll_result.value())
            {
                auto read_result = m_seq.read(std::span{read_event_buffer});
                if (read_result)
                {
                    auto input_events = read_result.value();

                    struct timespec ts{};
                    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
                    for (auto& ev : input_events)
                    {
                        // Set the event timestamp to the current time
                        ev.time.time.tv_sec = ts.tv_sec;
                        ev.time.time.tv_nsec = ts.tv_nsec;
                    }

                    m_input_events->queue.push(
                        read_result.value().data(),
                        read_result.value().size());
                }
            }
        }
    })
{
}

midi_io::~midi_io()
{
    m_in_thread.request_stop();
}

void
midi_io::process_input(event_handler& handler)
{
    m_input_events->queue.consume_all([&](snd_seq_event const& ev) {
        switch (ev.type)
        {
            case SNDRV_SEQ_EVENT_CONTROLLER:
                handler.process_cc_event(
                    ev.source.client,
                    ev.source.port,
                    ev.data.control.channel,
                    ev.data.control.param,
                    static_cast<std::int8_t>(ev.data.control.value));
                break;

            case SNDRV_SEQ_EVENT_PITCHBEND:
                handler.process_pitch_bend_event(
                    ev.source.client,
                    ev.source.port,
                    ev.data.control.channel,
                    static_cast<std::int16_t>(ev.data.control.value));
                break;

            default:
                break;
        }
    });
}

midi_devices::midi_devices(midi_client_id_t in_client_id, midi_port_t in_port)
    : m_in_client_id(in_client_id)
    , m_in_port(in_port)
    , m_seq(open_seq())
    , m_client_id(get_client_id(m_seq))
    , m_port(make_input_port(m_seq, m_client_id))
{
    snd_seq_port_subscribe port_sub{};
    port_sub.sender.client = SNDRV_SEQ_CLIENT_SYSTEM;
    port_sub.sender.port = SNDRV_SEQ_PORT_SYSTEM_ANNOUNCE;
    port_sub.dest.client = m_client_id;
    port_sub.dest.port = m_port;

    if (auto err = m_seq.ioctl(SNDRV_SEQ_IOCTL_SUBSCRIBE_PORT, port_sub))
    {
        throw std::system_error(err);
    }

    std::ranges::transform(
        scan_input_devices(m_seq),
        std::back_inserter(m_initial_updates),
        [](auto const& d) { return midi_device_added{.device = d}; });
}

midi_devices::~midi_devices()
{
    snd_seq_port_subscribe port_sub{};
    port_sub.sender.client = SNDRV_SEQ_CLIENT_SYSTEM;
    port_sub.sender.port = SNDRV_SEQ_PORT_SYSTEM_ANNOUNCE;
    port_sub.dest.client = m_client_id;
    port_sub.dest.port = m_port;

    if (auto err = m_seq.ioctl(SNDRV_SEQ_IOCTL_UNSUBSCRIBE_PORT, port_sub))
    {
        auto const message = err.message();
        spdlog::error(
            "midi_devices: failed to unsubscribe from system "
            "announcements: {}",
            message);
    }
}

[[nodiscard]]
auto
midi_devices::connect_input(
    midi_client_id_t source_client_id,
    midi_port_t source_port) -> bool
{
    snd_seq_port_subscribe port_sub{};
    port_sub.sender.client = source_client_id;
    port_sub.sender.port = source_port;
    port_sub.dest.client = m_in_client_id;
    port_sub.dest.port = m_in_port;
    port_sub.flags =
        SNDRV_SEQ_PORT_SUBS_TIMESTAMP | SNDRV_SEQ_PORT_SUBS_TIME_REAL;

    return !m_seq.ioctl(SNDRV_SEQ_IOCTL_SUBSCRIBE_PORT, port_sub);
}

void
midi_devices::disconnect_input(
    midi_client_id_t source_client_id,
    midi_port_t source_port)
{
    snd_seq_port_subscribe port_sub{};
    port_sub.sender.client = source_client_id;
    port_sub.sender.port = source_port;
    port_sub.dest.client = m_in_client_id;
    port_sub.dest.port = m_in_port;

    if (auto err = m_seq.ioctl(SNDRV_SEQ_IOCTL_UNSUBSCRIBE_PORT, port_sub))
    {
        auto const message = err.message();
        spdlog::warn("midi_devices: disconnect from input failed: {}", message);
    }
}

[[nodiscard]]
auto
midi_devices::update() -> std::vector<midi_device_event>
{
    std::vector<midi_device_event> result = std::move(m_initial_updates);

    snd_seq_event ev{};
    while (auto read_result = m_seq.read(ev))
    {
        switch (ev.type)
        {
            case SNDRV_SEQ_EVENT_PORT_START:
            {
                snd_seq_port_info port_info{};
                port_info.addr = ev.data.addr;

                if (!m_seq.ioctl(SNDRV_SEQ_IOCTL_GET_PORT_INFO, port_info))
                {
                    if ((port_info.capability & SNDRV_SEQ_PORT_CAP_READ) &&
                        (port_info.capability & SNDRV_SEQ_PORT_CAP_SUBS_READ))
                    {
                        snd_seq_client_info client_info{};
                        client_info.client = port_info.addr.client;
                        if (!m_seq.ioctl(
                                SNDRV_SEQ_IOCTL_GET_CLIENT_INFO,
                                client_info))
                        {
                            if (client_info.type == KERNEL_CLIENT)
                            {
                                result.emplace_back(
                                    midi_device_added{
                                        .device = midi_device{
                                            .client_id = port_info.addr.client,
                                            .port = port_info.addr.port,
                                            .name = client_info.name}});
                            }
                        }
                    }
                }
                break;
            }

            case SNDRV_SEQ_EVENT_PORT_EXIT:
                result.emplace_back(
                    midi_device_removed{
                        .device = midi_device{
                            .client_id = ev.data.addr.client,
                            .port = ev.data.addr.port,
                            .name = {}}});
                break;

            default:
                break;
        }
    }

    return result;
}

} // namespace piejam::midi::alsa
