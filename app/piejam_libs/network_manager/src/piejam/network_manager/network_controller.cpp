// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/network_manager/network_controller.h>

#include <piejam/network_manager/nmcli.h>

#include <spdlog/spdlog.h>

#include <QObject>

#include <memory>
#include <string>

namespace piejam::network_manager
{

struct network_controller::impl
{
    bool enabled{false};
    bool refresh_pending{};
    state_change_callback on_state_change;
    error_callback on_error;

    // Owns the running nmcli processes. Declared last so it's destroyed
    // first: a pending process can't call back into a half-destroyed impl.
    std::unique_ptr<QObject> context{std::make_unique<QObject>()};

    impl()
    {
        spdlog::debug("Network controller initialized (NetworkManager)");
    }

    /// changed_here: the app switched the radio (logged for the user), as
    /// opposed to a refresh finding NetworkManager's current state.
    void set_enabled(bool value, bool changed_here)
    {
        if (enabled == value)
        {
            return;
        }
        enabled = value;
        spdlog::log(
            changed_here ? spdlog::level::info : spdlog::level::debug,
            "Network {}",
            enabled ? "enabled" : "disabled");
        if (on_state_change)
        {
            on_state_change(enabled);
        }
    }

    void refresh()
    {
        if (refresh_pending)
        {
            return;
        }
        refresh_pending = true;

        nmcli::run(
            *context,
            {"-t", "-f", "WIFI", "general"},
            [this](nmcli::result const& r) {
                refresh_pending = false;
                if (r.ok)
                {
                    set_enabled(r.output.starts_with("enabled"), false);
                }
            });
    }

    void set_radio(bool on)
    {
        nmcli::run(
            *context,
            {"radio", "wifi", on ? "on" : "off"},
            [this, on](nmcli::result const& r) {
                if (r.ok)
                {
                    set_enabled(on, true);
                    return;
                }
                auto const message =
                    std::string{
                        on ? "Couldn't turn WiFi on: "
                           : "Couldn't turn WiFi off: "} +
                    r.error;
                spdlog::error("{}", message);
                if (on_error)
                {
                    on_error(message);
                }
                refresh();
            });
    }
};

network_controller::network_controller()
    : m_impl(std::make_unique<impl>())
{
}

network_controller::~network_controller() = default;

network_controller::network_controller(network_controller&&) noexcept = default;
network_controller&
network_controller::operator=(network_controller&&) noexcept = default;

bool
network_controller::is_enabled() const noexcept
{
    return m_impl->enabled;
}

void
network_controller::refresh()
{
    m_impl->refresh();
}

bool
network_controller::enable()
{
    spdlog::debug("Enabling WiFi");
    m_impl->set_radio(true);
    return true;
}

bool
network_controller::disable()
{
    spdlog::debug("Disabling WiFi");
    m_impl->set_radio(false);
    return true;
}

void
network_controller::set_state_change_callback(state_change_callback cb)
{
    m_impl->on_state_change = std::move(cb);
}

void
network_controller::set_error_callback(error_callback cb)
{
    m_impl->on_error = std::move(cb);
}

} // namespace piejam::network_manager
