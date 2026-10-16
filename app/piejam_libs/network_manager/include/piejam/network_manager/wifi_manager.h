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
    disconnected,   ///< Not connected to any network
    connecting,     ///< Connection in progress
    connected,      ///< Successfully connected
    failed          ///< Connection attempt failed
};

/// Manages WiFi connections via wpa_supplicant.
///
/// This class provides:
/// - Network scanning
/// - Connection management
/// - Saved networks with priority ordering
/// - Auto-reconnect logic
/// - Status monitoring
class wifi_manager
{
public:
    /// Callback types
    using scan_complete_callback =
        std::function<void(std::vector<wifi_network> const&)>;
    using connection_changed_callback =
        std::function<void(wifi_connection_status, wifi_network const*)>;
    using error_callback = std::function<void(std::string const&)>;

    /// Construct WiFi manager for specified interface
    /// @param interface Network interface name (default: "wlan0")
    explicit wifi_manager(std::string interface = "wlan0");
    ~wifi_manager();

    wifi_manager(wifi_manager const&) = delete;
    wifi_manager& operator=(wifi_manager const&) = delete;

    wifi_manager(wifi_manager&&) noexcept;
    wifi_manager& operator=(wifi_manager&&) noexcept;

    // --- Status ---

    /// Get current connection status
    [[nodiscard]] wifi_connection_status status() const noexcept;

    /// Get current SSID if connected
    [[nodiscard]] std::string current_ssid() const;

    /// Get current IP address if connected
    [[nodiscard]] std::string ip_address() const;

    /// Get signal strength of current connection (0-100)
    [[nodiscard]] int signal_strength() const;

    /// Check if WiFi interface is available
    [[nodiscard]] bool is_interface_available() const;

    // --- Scanning ---

    /// Initiate async network scan
    /// Results delivered via scan_complete_callback
    void scan_networks();

    /// Get cached list of networks from last scan
    [[nodiscard]] std::vector<wifi_network> const& available_networks() const;

    // --- Connection Management ---

    /// Start connecting to a network (non-blocking).
    /// Configures wpa_supplicant and issues SELECT_NETWORK, then returns
    /// immediately. Caller should poll with poll_connection().
    /// @return network_id on success, -1 on setup failure
    int connect_start(
        std::string const& ssid,
        std::string const& password,
        bool remember);

    /// Poll connection status once after connect_start().
    /// @return current status (connected / connecting / disconnected / failed)
    wifi_connection_status poll_connection();

    /// Disconnect from current network
    void disconnect();

    /// Reconnect to current or last known network
    void reconnect();

    // --- Saved Networks ---

    /// Get list of saved networks
    [[nodiscard]] std::vector<wifi_saved_network> saved_networks() const;

    /// Remove a saved network
    /// @param ssid Network to forget
    /// @return true if network was found and removed
    bool forget_network(std::string const& ssid);

    /// Set priority for a saved network
    /// @param ssid Network name
    /// @param priority Priority value (higher = preferred)
    void set_network_priority(std::string const& ssid, int priority);

    /// Enable/disable auto-connect for a saved network
    void set_network_auto_connect(std::string const& ssid, bool enabled);

    /// Persist current network configuration to wpa_supplicant.conf
    /// Requires update_config=1 in the config file.
    void save_config();

    // --- Auto-reconnect ---

    /// Enable automatic reconnection when connection drops
    void set_auto_reconnect(bool enabled);

    /// Check if auto-reconnect is enabled
    [[nodiscard]] bool auto_reconnect() const noexcept;

    // --- Callbacks ---

    /// Register callback for scan completion
    void set_scan_complete_callback(scan_complete_callback cb);

    /// Register callback for connection status changes
    void set_connection_changed_callback(connection_changed_callback cb);

    /// Register callback for errors
    void set_error_callback(error_callback cb);

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

} // namespace piejam::network_manager
