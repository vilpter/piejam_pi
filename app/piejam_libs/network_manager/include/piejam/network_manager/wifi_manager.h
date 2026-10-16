// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/network_manager/fwd.h>
#include <piejam/network_manager/wifi_network.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace piejam::network_manager
{

/// Connection status for WiFi
enum class wifi_connection_status
{
    disconnected, ///< Not connected to any network
    connecting,   ///< Connection in progress
    connected,    ///< Successfully connected
    failed        ///< Connection attempt failed
};

/// Manages WiFi through NetworkManager (nmcli).
///
/// NetworkManager owns the interface, its wpa_supplicant and DHCP, and
/// keeps saved networks across reboots. This class mirrors its state for
/// the UI and asks it to scan, connect, disconnect and forget.
///
/// All operations are asynchronous: they return immediately and report
/// through callbacks on the calling thread, which must run a Qt event loop
/// (the GUI thread). The status getters return the state seen by the last
/// refresh_status() and never block.
class wifi_manager
{
public:
    /// Callback types
    using scan_complete_callback =
        std::function<void(std::vector<wifi_network> const&)>;
    using connection_changed_callback =
        std::function<void(wifi_connection_status, wifi_network const*)>;
    using saved_networks_callback =
        std::function<void(std::vector<wifi_saved_network> const&)>;
    using error_callback = std::function<void(std::string const&)>;
    /// Result of an operation: success, or failure with a message.
    using done_callback =
        std::function<void(bool ok, std::string const& error)>;

    /// Construct WiFi manager for specified interface
    /// @param interface Network interface name (default: "wlan0")
    explicit wifi_manager(std::string interface = "wlan0");
    ~wifi_manager();

    wifi_manager(wifi_manager const&) = delete;
    wifi_manager& operator=(wifi_manager const&) = delete;

    wifi_manager(wifi_manager&&) noexcept;
    wifi_manager& operator=(wifi_manager&&) noexcept;

    // --- Status ---

    /// Ask NetworkManager for the interface's current state. Fires the
    /// connection_changed callback if the status, SSID, IP address or
    /// signal changed.
    void refresh_status();

    /// Get current connection status
    [[nodiscard]]
    wifi_connection_status status() const noexcept;

    /// Get current SSID if connected
    [[nodiscard]]
    std::string current_ssid() const;

    /// Get current IP address if connected
    [[nodiscard]]
    std::string ip_address() const;

    /// Get signal strength of current connection (0-100)
    [[nodiscard]]
    int signal_strength() const;

    /// Check if WiFi interface is available
    [[nodiscard]]
    bool is_interface_available() const;

    // --- Scanning ---

    /// Scan for networks. Results are delivered via scan_complete_callback;
    /// on failure, the error callback fires and the previous list is
    /// delivered again.
    void scan_networks();

    /// List the networks NetworkManager already knows about, without a new
    /// scan. Results are delivered via scan_complete_callback.
    void list_networks();

    /// Get cached list of networks from last scan
    [[nodiscard]]
    std::vector<wifi_network> const& available_networks() const;

    // --- Connection Management ---

    /// Connect to a network. A saved network is activated as saved, unless
    /// a new password is given. A new network gets a profile that is kept
    /// across reboots only if remember is true. The password is passed to
    /// NetworkManager on standard input, never on a command line.
    void connect(
        std::string const& ssid,
        std::string const& password,
        bool remember,
        done_callback on_done);

    /// Disconnect from current network
    void disconnect(done_callback on_done = {});

    /// Reconnect to the best available saved network
    void reconnect(done_callback on_done = {});

    // --- Saved Networks ---

    /// Ask NetworkManager for its saved WiFi profiles. Results are
    /// delivered via saved_networks_callback.
    void refresh_saved_networks();

    /// Get list of saved networks, as of the last refresh
    [[nodiscard]]
    std::vector<wifi_saved_network> const& saved_networks() const;

    /// Remove a saved network
    /// @param ssid Network to forget
    /// @return true if network was found in the saved list
    bool forget_network(std::string const& ssid, done_callback on_done = {});

    /// Set priority for a saved network
    /// @param ssid Network name
    /// @param priority Priority value (higher = preferred)
    void set_network_priority(std::string const& ssid, int priority);

    /// Enable/disable auto-connect for a saved network
    void set_network_auto_connect(std::string const& ssid, bool enabled);

    // --- Auto-reconnect ---

    /// NetworkManager reconnects to saved networks by itself; this flag is
    /// kept for the UI's sake and doesn't change its behavior.
    void set_auto_reconnect(bool enabled);

    /// Check if auto-reconnect is enabled
    [[nodiscard]]
    bool auto_reconnect() const noexcept;

    // --- Callbacks ---

    /// Register callback for scan completion
    void set_scan_complete_callback(scan_complete_callback cb);

    /// Register callback for connection status changes
    void set_connection_changed_callback(connection_changed_callback cb);

    /// Register callback for saved network list updates
    void set_saved_networks_callback(saved_networks_callback cb);

    /// Register callback for errors
    void set_error_callback(error_callback cb);

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

} // namespace piejam::network_manager
