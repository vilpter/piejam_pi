// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/network_manager/fwd.h>

#include <functional>
#include <memory>

namespace piejam::network_manager
{

/// Controls network functionality (WiFi, NFS).
///
/// This class provides:
/// - Manual enable/disable of all network interfaces
/// - State persistence across sessions
/// - Callbacks for state change notifications
class network_controller
{
public:
    /// Callback type for network state changes
    using state_change_callback = std::function<void(bool enabled)>;

    network_controller();
    ~network_controller();

    network_controller(network_controller const&) = delete;
    network_controller& operator=(network_controller const&) = delete;

    network_controller(network_controller&&) noexcept;
    network_controller& operator=(network_controller&&) noexcept;

    /// Returns true if network is currently enabled
    [[nodiscard]] bool is_enabled() const noexcept;

    /// Enable network interfaces (WiFi)
    /// Returns true if operation succeeded
    bool enable();

    /// Disable network interfaces (WiFi, unmount NFS)
    /// Returns true if operation succeeded
    bool disable();

    /// Register callback for network state changes
    void set_state_change_callback(state_change_callback cb);

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

} // namespace piejam::network_manager
