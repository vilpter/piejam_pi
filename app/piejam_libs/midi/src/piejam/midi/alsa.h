// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/pimpl.h>
#include <piejam/system/device.h>

#include <cstdint>
#include <string>
#include <thread>
#include <variant>
#include <vector>

namespace piejam::midi::alsa
{

using midi_client_id_t = int;
using midi_port_t = int;

class event_handler
{
public:
    virtual ~event_handler() = default;

    virtual void process_cc_event(
        midi_client_id_t,
        midi_port_t,
        std::uint8_t channel,
        std::uint8_t cc_id,
        std::int8_t value) = 0;

    virtual void process_pitch_bend_event(
        midi_client_id_t,
        midi_port_t,
        std::uint8_t channel,
        std::int16_t value) = 0;
};

class midi_io
{
public:
    midi_io();
    ~midi_io();

    [[nodiscard]]
    auto client_id() const noexcept -> midi_client_id_t
    {
        return m_client_id;
    }

    [[nodiscard]]
    auto in_port() const noexcept -> midi_port_t
    {
        return m_in_port;
    }

    void process_input(event_handler&);

private:
    system::device m_seq;

    midi_client_id_t m_client_id{};
    midi_port_t m_in_port{};

    struct impl;
    pimpl<impl> m_input_events;

    std::jthread m_in_thread;
};

struct midi_device
{
    midi_client_id_t client_id{};
    midi_port_t port{};
    std::string name;

    [[nodiscard]]
    constexpr auto operator==(midi_device const& other) const noexcept -> bool
    {
        return client_id == other.client_id && port == other.port;
    }

    [[nodiscard]]
    constexpr auto operator!=(midi_device const& other) const noexcept -> bool
    {
        return !(*this == other);
    }
};

struct midi_device_added
{
    midi_device device;
};

struct midi_device_removed
{
    midi_device device;
};

using midi_device_event = std::variant<midi_device_added, midi_device_removed>;

class midi_devices
{
public:
    midi_devices(midi_client_id_t in_client_id, midi_port_t in_port);

    midi_devices(midi_devices const&) = delete;
    midi_devices(midi_devices&&) noexcept = delete;

    ~midi_devices();

    auto operator=(midi_devices const&) -> midi_devices& = delete;
    auto operator=(midi_devices&&) noexcept -> midi_devices& = delete;

    [[nodiscard]]
    auto connect_input(midi_client_id_t, midi_port_t) -> bool;
    void disconnect_input(midi_client_id_t, midi_port_t);

    [[nodiscard]]
    auto update() -> std::vector<midi_device_event>;

private:
    midi_client_id_t m_in_client_id{};
    midi_port_t m_in_port{};

    system::device m_seq;
    midi_client_id_t m_client_id{};
    midi_port_t m_port;

    std::vector<midi_device_event> m_initial_updates;
};

} // namespace piejam::midi::alsa
