// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/network_manager/wifi_network.h>

#include <QStringList>

#include <functional>
#include <string>
#include <string_view>
#include <vector>

class QObject;

/// Talking to NetworkManager through its command line client, nmcli.
///
/// Every command runs asynchronously through QProcess with an argument list,
/// never through a shell, so SSIDs and other user input are passed verbatim.
/// Output is requested in terse mode (-t) under a fixed English UTF-8
/// locale, which keeps it machine-readable.
namespace piejam::network_manager::nmcli
{

/// Outcome of one nmcli invocation.
struct result
{
    bool ok{};          ///< nmcli ran and exited with status 0
    std::string output; ///< standard output
    std::string error;  ///< nmcli's error message, or why it didn't run
};

using result_handler = std::function<void(result const&)>;

/// Run nmcli with the given arguments. on_done is called exactly once, on
/// the thread that owns context, when nmcli exits, fails to start or times
/// out. It is not called if context is destroyed first. stdin_data, if not
/// empty, is written to nmcli's standard input (used for secrets, which must
/// never appear on a command line).
void
run(QObject& context,
    QStringList const& arguments,
    result_handler on_done,
    std::string const& stdin_data = {},
    int timeout_ms = 15000);

/// Split one line of terse tabular output into fields, undoing nmcli's
/// escaping of ':' and '\' inside values.
auto split_fields(std::string_view line) -> std::vector<std::string>;

/// Undo nmcli's escaping in a single terse value.
auto unescape(std::string_view value) -> std::string;

/// NetworkManager device states (NMDeviceState).
inline constexpr int device_state_unavailable = 20;
inline constexpr int device_state_disconnected = 30;
inline constexpr int device_state_prepare = 40;
inline constexpr int device_state_activated = 100;

/// State of one network device, from
/// `nmcli -t -f GENERAL.STATE,GENERAL.CONNECTION,IP4.ADDRESS,AP device show
/// IFACE`.
struct device_status
{
    int state{0};            ///< NMDeviceState; 100 = connected
    std::string connection;  ///< active connection profile, if any
    std::string ip_address;  ///< first IPv4 address, without the prefix
    std::string active_ssid; ///< SSID of the access point in use
    int active_signal{0};    ///< its signal strength, 0-100
};

auto parse_device_status(std::string_view output) -> device_status;

/// Parse `nmcli -t -f IN-USE,SSID,BSSID,FREQ,SIGNAL,SECURITY device wifi list`.
/// Hidden networks are skipped and each SSID is listed once, using its
/// strongest access point (or the one in use). Strongest first.
auto parse_wifi_list(std::string_view output) -> std::vector<wifi_network>;

/// Map nmcli's SECURITY column ("WPA2 WPA3", "WPA1", "WEP", "") to
/// wifi_security.
auto parse_security(std::string_view security) -> wifi_security;

/// A connection profile, from
/// `nmcli -t -f NAME,UUID,TYPE,AUTOCONNECT,AUTOCONNECT-PRIORITY connection
/// show`.
struct connection
{
    std::string name;
    std::string uuid;
    std::string type;
    bool autoconnect{};
    int priority{};
};

auto parse_connections(std::string_view output) -> std::vector<connection>;

inline constexpr std::string_view wifi_connection_type = "802-11-wireless";

/// Map a WiFi profile's 802-11-wireless-security.key-mgmt value to
/// wifi_security. An empty value means the profile has no security.
auto key_mgmt_to_security(std::string_view key_mgmt) -> wifi_security;

/// The key-mgmt value to use for a new profile on a network with the given
/// security, or an empty string for an open network.
auto security_to_key_mgmt(wifi_security security) -> std::string;

/// The UUID in nmcli's "Connection 'NAME' (UUID) successfully added."
auto parse_added_uuid(std::string_view output) -> std::string;

/// First line of nmcli's error output, without the "Error: " prefix.
auto error_message(std::string_view error_output) -> std::string;

} // namespace piejam::network_manager::nmcli
