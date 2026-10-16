// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/network_manager/fwd.h>

#include <functional>
#include <memory>
#include <string>

namespace piejam::network_manager
{

/// Controls network functionality (WiFi, NFS).
///
/// "Network enabled" is NetworkManager's WiFi radio switch. NetworkManager
/// remembers it across reboots, and turning it off drops any WiFi
/// connection. Operations are asynchronous and report on the calling
/// thread, which must run a Qt event loop (the GUI thread).
class network_controller
{
public:
    /// Callback type for network state changes
    using state_change_callback = std::function<void(bool enabled)>;
    using error_callback = std::function<void(std::string const&)>;

    network_controller();
    ~network_controller();

    network_controller(network_controller const&) = delete;
    network_controller& operator=(network_controller const&) = delete;

    network_controller(network_controller&&) noexcept;
    network_controller& operator=(network_controller&&) noexcept;

    /// Returns true if network was enabled at the last refresh or change
    [[nodiscard]]
    bool is_enabled() const noexcept;

    /// Read the WiFi radio state from NetworkManager. Fires the state
    /// change callback if it changed.
    void refresh();

    /// Turn the WiFi radio on
    /// Returns true if the request was sent
    bool enable();

    /// Unmount NFS shares and turn the WiFi radio off
    /// Returns true if the request was sent
    bool disable();

    /// Register callback for network state changes
    void set_state_change_callback(state_change_callback cb);

    /// Register callback for errors
    void set_error_callback(error_callback cb);

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

} // namespace piejam::network_manager
