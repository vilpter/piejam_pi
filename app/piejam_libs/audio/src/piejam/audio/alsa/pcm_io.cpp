// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pcm_io.h"

#include "get_set_hw_params.h"
#include "process_step.h"

#include <piejam/audio/process_thread.h>
#include <piejam/audio/sound_card_descriptor.h>
#include <piejam/audio/sound_card_hw_params.h>

#include <spdlog/spdlog.h>

#include <fcntl.h>
#include <sound/asound.h>
#include <sys/ioctl.h>

#include <boost/assert.hpp>

#include <utility>

namespace piejam::audio::alsa
{

static auto
open_pcm(
    std::filesystem::path const& path,
    sound_card_config const& process_config)
    -> std::pair<system::device, sound_card_stream_config>
{
    if (!path.empty())
    {
        system::device fd(path);

        auto [num_channels, format, period_count] =
            set_hw_params(fd, process_config);

        unsigned const buffer_size =
            process_config.period_size.value() * period_count;
        snd_pcm_sw_params sw_params{};
        sw_params.proto = SNDRV_PCM_VERSION;
        sw_params.tstamp_mode = SNDRV_PCM_TSTAMP_ENABLE;
        sw_params.tstamp_type = SNDRV_PCM_TSTAMP_TYPE_MONOTONIC_RAW;
        sw_params.period_step = 1;
        sw_params.sleep_min = 0;
        sw_params.avail_min = process_config.period_size.value();
        sw_params.xfer_align = 1;
        sw_params.start_threshold = process_config.period_size.value();
        sw_params.stop_threshold = buffer_size;
        sw_params.silence_threshold = 0;
        sw_params.boundary = buffer_size;
        while (sw_params.boundary * 2 <=
               static_cast<long unsigned>(
                   std::numeric_limits<long>::max() - buffer_size))
        {
            sw_params.boundary *= 2;
        }
        sw_params.silence_size = sw_params.boundary;

        if (auto err = fd.ioctl(SNDRV_PCM_IOCTL_SW_PARAMS, sw_params))
        {
            throw std::system_error(err);
        }

        return {
            std::move(fd),
            sound_card_stream_config{
                .format = format,
                .num_channels = num_channels,
            }};
    }

    return {};
}

pcm_io::pcm_io() noexcept = default;

pcm_io::pcm_io(
    std::filesystem::path const& in,
    std::filesystem::path const& out,
    sound_card_config const& sc_config)
{
    std::tie(m_input_fd, m_io_config.in_config) = open_pcm(in, sc_config);
    std::tie(m_output_fd, m_io_config.out_config) = open_pcm(out, sc_config);
    m_io_config.sc_config = sc_config;

    if (m_input_fd && m_output_fd)
    {
        if (auto err = m_input_fd.ioctl(
                SNDRV_PCM_IOCTL_LINK,
                std::as_const(m_output_fd)))
        {
            throw std::system_error(err);
        }
    }
}

pcm_io::~pcm_io()
{
    if (m_process_thread)
    {
        stop();
    }

    if (is_open())
    {
        close();
    }
}

auto
pcm_io::is_open() const noexcept -> bool
{
    return m_input_fd || m_output_fd;
}

void
pcm_io::close()
{
    BOOST_ASSERT(!is_running());

    auto input_fd = std::move(m_input_fd);
    auto output_fd = std::move(m_output_fd);

    if (input_fd && output_fd)
    {
        if (auto err = output_fd.ioctl(SNDRV_PCM_IOCTL_UNLINK))
        {
            auto const message = err.message();
            spdlog::error("pcm_io::close: {}", message);
        }
    }
}

auto
pcm_io::is_running() const noexcept -> bool
{
    return m_process_thread && m_process_thread->is_running();
}

void
pcm_io::start(
    thread::configuration const& thread_config,
    init_process_function const& init_process_function,
    process_function process_function)
{
    BOOST_ASSERT(is_open());
    BOOST_ASSERT(!m_process_thread);

    m_xruns.store(0, std::memory_order_relaxed);

    m_process_thread = std::make_unique<process_thread>();
    m_process_thread->start(
        thread_config,
        process_step(
            m_input_fd,
            m_output_fd,
            m_io_config,
            m_cpu_load,
            m_xruns,
            init_process_function,
            std::move(process_function)));
}

void
pcm_io::stop()
{
    BOOST_ASSERT(is_open());
    BOOST_ASSERT(m_process_thread);

    if (m_process_thread->is_running())
    {
        m_process_thread->stop();
    }
    else if (auto err = m_process_thread->error())
    {
        auto const message = err.message();
        spdlog::error("Process thread stopped with: {}", message);
    }

    m_process_thread.reset();

    system::device& fd = m_input_fd ? m_input_fd : m_output_fd;
    if (auto err = fd.ioctl(SNDRV_PCM_IOCTL_DROP))
    {
        auto const message = err.message();
        spdlog::error("pcm_io::stop: {}", message);
    }
}

} // namespace piejam::audio::alsa
