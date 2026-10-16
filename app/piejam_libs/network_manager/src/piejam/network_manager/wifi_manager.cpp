// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/network_manager/wifi_manager.h>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <sstream>
#include <unordered_set>

#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace piejam::network_manager
{

namespace
{

/// Send a command to wpa_supplicant via its Unix control socket and
/// return the response. This avoids fork()/popen() entirely, making
/// it safe to call from any thread without deadlock risk.
///
/// Protocol:
///   - wpa_supplicant listens on /var/run/wpa_supplicant/<iface>
///   - Client binds a unique local socket path and sends datagram
///   - Response arrives as one or more datagrams
///   - Unsolicited events are prefixed with '<N>' (N=0..4); skip them
///   - Timeout via poll() — never blocks indefinitely
///
/// Returns empty string on error or timeout.
class WpaSocket
{
public:
    explicit WpaSocket(std::string const& interface)
    {
        // Build local socket path — must be unique per instance
        // Use pid + pointer to ensure uniqueness across concurrent calls
        char local_path[108];
        ::snprintf(
            local_path,
            sizeof(local_path),
            "/tmp/wpa_ctrl_%d_%p",
            static_cast<int>(::getpid()),
            static_cast<void*>(this));

        m_local_path = local_path;

        m_fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
        if (m_fd < 0)
        {
            spdlog::error("wpa_socket: socket() failed (errno={})", errno);
            return;
        }

        // Bind local socket so wpa_supplicant can send responses back
        struct sockaddr_un local_addr{};
        local_addr.sun_family = AF_UNIX;
        ::strncpy(
            local_addr.sun_path,
            m_local_path.c_str(),
            sizeof(local_addr.sun_path) - 1);

        if (::bind(m_fd, reinterpret_cast<struct sockaddr*>(&local_addr),
                   sizeof(local_addr)) != 0)
        {
            spdlog::error(
                "wpa_socket: bind({}) failed (errno={})",
                m_local_path,
                errno);
            ::close(m_fd);
            m_fd = -1;
            return;
        }

        // Connect to wpa_supplicant's control socket
        std::string ctrl_path = "/var/run/wpa_supplicant/" + interface;
        struct sockaddr_un remote_addr{};
        remote_addr.sun_family = AF_UNIX;
        ::strncpy(
            remote_addr.sun_path,
            ctrl_path.c_str(),
            sizeof(remote_addr.sun_path) - 1);

        if (::connect(
                m_fd,
                reinterpret_cast<struct sockaddr*>(&remote_addr),
                sizeof(remote_addr)) != 0)
        {
            spdlog::error(
                "wpa_socket: connect({}) failed (errno={})",
                ctrl_path,
                errno);
            ::close(m_fd);
            m_fd = -1;
            ::unlink(m_local_path.c_str());
            m_local_path.clear();
            return;
        }
    }

    ~WpaSocket()
    {
        if (m_fd >= 0)
            ::close(m_fd);
        if (!m_local_path.empty())
            ::unlink(m_local_path.c_str());
    }

    // Non-copyable, non-movable — owns fd and path
    WpaSocket(WpaSocket const&) = delete;
    WpaSocket& operator=(WpaSocket const&) = delete;

    bool is_open() const { return m_fd >= 0; }

    /// Send command and return response. Skips unsolicited event
    /// messages (prefixed with '<N>'). Times out after timeout_ms.
    std::string
    send_cmd(std::string const& cmd, int timeout_ms = 5000)
    {
        if (m_fd < 0)
            return {};

        if (::send(m_fd, cmd.c_str(), cmd.size(), 0) < 0)
        {
            spdlog::error(
                "wpa_socket: send({}) failed (errno={})", cmd, errno);
            return {};
        }

        // Read response, skipping unsolicited event datagrams
        std::array<char, 4096> buf;
        struct pollfd pfd{m_fd, POLLIN, 0};

        for (;;)
        {
            int ret = ::poll(&pfd, 1, timeout_ms);
            if (ret < 0)
            {
                if (errno == EINTR)
                    continue; // signal interrupted poll — retry
                spdlog::error(
                    "wpa_socket: poll() failed (errno={})", errno);
                return {};
            }
            if (ret == 0)
            {
                spdlog::warn(
                    "wpa_socket: timeout waiting for response to: {}",
                    cmd);
                return {};
            }

            ssize_t n = ::recv(m_fd, buf.data(), buf.size() - 1, 0);
            if (n <= 0)
                return {};

            buf[static_cast<size_t>(n)] = '\0';

            // Skip unsolicited event messages — they start with '<N>'
            // where N is a single digit priority level (0-4)
            if (n >= 3 && buf[0] == '<' && buf[2] == '>')
                continue;

            return std::string(buf.data(), static_cast<size_t>(n));
        }
    }

private:
    int m_fd{-1};
    std::string m_local_path;
};

/// Send a wpa_supplicant control command via Unix socket.
/// Safe to call from any thread — no fork/popen involved.
std::string
wpa_ctrl(std::string const& interface, std::string const& cmd)
{
    // wpa_supplicant control socket requires uppercase command verbs.
    // Uppercase only the first word (the command); arguments (SSIDs,
    // passwords, network IDs) must remain as-is.
    std::string upper_cmd = cmd;
    auto first_space = upper_cmd.find(' ');
    auto verb_end = (first_space == std::string::npos) ? upper_cmd.size() : first_space;
    for (std::size_t i = 0; i < verb_end; ++i)
        upper_cmd[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(upper_cmd[i])));

    WpaSocket sock(interface);
    if (!sock.is_open())
    {
        spdlog::warn("wpa_ctrl: socket not available for: {}", cmd);
        return {};
    }
    auto response = sock.send_cmd(upper_cmd);
    if (response.empty())
        spdlog::warn("wpa_ctrl: empty response for '{}'", upper_cmd);
    return response;
}

/// Execute a shell command and return output.
/// NOTE: Uses popen/fork — only call from the Qt main thread.
/// For wpa_supplicant commands use wpa_ctrl() instead.
std::string
execute_command(char const* cmd)
{
    std::array<char, 256> buffer;
    std::string result;

    auto pipe_deleter = [](FILE* f) { if (f) pclose(f); };
    std::unique_ptr<FILE, decltype(pipe_deleter)> pipe(
        popen(cmd, "r"), pipe_deleter);
    if (!pipe)
        return result;

    while (fgets(
               buffer.data(),
               static_cast<int>(buffer.size()),
               pipe.get()) != nullptr)
        result += buffer.data();

    return result;
}

/// Thin wrapper — kept for call sites that use the old wpa_cli()
/// name but now routes through the socket-based wpa_ctrl().
std::string
wpa_cli(std::string const& interface, std::string const& cmd)
{
    return wpa_ctrl(interface, cmd);
}

/// Parse security flags from wpa_cli scan results
wifi_security
parse_security_flags(std::string const& flags)
{
    if (flags.find("WPA3") != std::string::npos)
        return wifi_security::wpa3;
    if (flags.find("WPA2") != std::string::npos)
        return wifi_security::wpa2;
    if (flags.find("WPA") != std::string::npos)
        return wifi_security::wpa;
    if (flags.find("WEP") != std::string::npos)
        return wifi_security::wep;
    if (flags.find("ESS") != std::string::npos && flags.find("WPA") == std::string::npos)
        return wifi_security::open;
    return wifi_security::unknown;
}

/// Parse scan results from wpa_cli
std::vector<wifi_network>
parse_scan_results(std::string const& output)
{
    std::vector<wifi_network> networks;
    std::istringstream stream(output);
    std::string line;

    // Skip header line
    std::getline(stream, line);

    // Format: bssid / frequency / signal level / flags / ssid
    while (std::getline(stream, line))
    {
        if (line.empty())
            continue;

        std::istringstream line_stream(line);
        wifi_network net;

        std::string freq_str;
        std::string signal_str;
        std::string flags;

        line_stream >> net.bssid >> freq_str >> signal_str >> flags;

        // Rest of line is SSID (may contain spaces)
        std::getline(line_stream >> std::ws, net.ssid);

        if (net.ssid.empty() || net.bssid.empty())
            continue;

        try
        {
            net.frequency_mhz = std::stoi(freq_str);
            net.signal_strength = std::stoi(signal_str);
        }
        catch (...)
        {
            continue;
        }

        net.signal_percent = signal_dbm_to_percent(net.signal_strength);
        net.band = frequency_to_band(net.frequency_mhz);
        net.security = parse_security_flags(flags);

        networks.push_back(std::move(net));
    }

    // Sort by signal strength (strongest first)
    std::sort(
        networks.begin(),
        networks.end(),
        [](wifi_network const& a, wifi_network const& b) {
            return a.signal_percent > b.signal_percent;
        });

    // Remove duplicates (same SSID, keep strongest signal)
    // std::unique only works on adjacent elements, so use a set
    std::unordered_set<std::string> seen;
    auto it = std::remove_if(
        networks.begin(),
        networks.end(),
        [&seen](wifi_network const& net) {
            return !seen.insert(net.ssid).second;
        });
    networks.erase(it, networks.end());

    return networks;
}

/// Get current connection info from wpa_cli status
struct connection_info
{
    std::string ssid;
    std::string bssid;
    std::string ip_address;
    std::string wpa_state;
    int frequency{0};
};

connection_info
get_connection_info(std::string const& interface)
{
    connection_info info;
    std::string output = wpa_cli(interface, "status");
    std::istringstream stream(output);
    std::string line;

    while (std::getline(stream, line))
    {
        auto pos = line.find('=');
        if (pos == std::string::npos)
            continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        if (key == "ssid")
            info.ssid = value;
        else if (key == "bssid")
            info.bssid = value;
        else if (key == "ip_address")
            info.ip_address = value;
        else if (key == "wpa_state")
            info.wpa_state = value;
        else if (key == "freq")
        {
            try
            {
                info.frequency = std::stoi(value);
            }
            catch (...)
            {
            }
        }
    }

    return info;
}

/// Get IP address from interface
std::string
get_ip_address(std::string const& interface)
{
    std::string cmd = "ip -4 addr show " + interface +
                      " | awk '/inet /{split($2,a,\"/\"); print a[1]}'";
    std::string result = execute_command(cmd.c_str());
    // Remove trailing newline
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
        result.pop_back();
    return result;
}

/// Get signal strength of current connection
int
get_current_signal(std::string const& interface)
{
    std::string output = wpa_cli(interface, "signal_poll");
    std::istringstream stream(output);
    std::string line;

    while (std::getline(stream, line))
    {
        if (line.find("RSSI=") == 0)
        {
            try
            {
                int rssi = std::stoi(line.substr(5));
                return signal_dbm_to_percent(rssi);
            }
            catch (...)
            {
            }
        }
    }
    return 0;
}

/// Parse saved networks from wpa_cli list_networks
std::vector<wifi_saved_network>
parse_saved_networks(std::string const& output)
{
    std::vector<wifi_saved_network> networks;
    std::istringstream stream(output);
    std::string line;

    // Skip header line
    std::getline(stream, line);

    // Format: network id / ssid / bssid / flags
    while (std::getline(stream, line))
    {
        if (line.empty())
            continue;

        std::istringstream line_stream(line);
        wifi_saved_network net;

        std::string id;
        std::string bssid;
        std::string flags;

        line_stream >> id >> net.ssid >> bssid >> flags;

        if (net.ssid.empty())
            continue;

        // Parse flags for disabled status
        net.auto_connect = (flags.find("DISABLED") == std::string::npos);

        networks.push_back(std::move(net));
    }

    return networks;
}

/// Add or update network in wpa_supplicant
int
add_network_to_wpa(
    std::string const& interface,
    std::string const& ssid,
    std::string const& password,
    wifi_security security)
{
    // Add new network
    std::string result = wpa_cli(interface, "add_network");
    int network_id = -1;
    try
    {
        network_id = std::stoi(result);
    }
    catch (...)
    {
        spdlog::error("ADD_NETWORK failed: '{}'", result);
        return -1;
    }

    std::string id_str = std::to_string(network_id);

    // Set SSID — wpa_supplicant control socket protocol uses
    // double-quoted strings directly (no shell quoting needed)
    wpa_cli(interface, "set_network " + id_str + " ssid \"" + ssid + "\"");

    // Set security
    if (security == wifi_security::open || password.empty())
    {
        wpa_cli(interface, "set_network " + id_str + " key_mgmt NONE");
    }
    else
    {
        wpa_cli(interface, "set_network " + id_str + " psk \"" + password + "\"");
    }

    // Enable network
    wpa_cli(interface, "enable_network " + id_str);

    return network_id;
}

} // namespace

struct wifi_manager::impl
{
    std::string interface;
    wifi_connection_status connection_status{wifi_connection_status::disconnected};
    std::vector<wifi_network> cached_networks;
    wifi_network current_network;
    bool auto_reconnect_enabled{true};

    scan_complete_callback on_scan_complete;
    connection_changed_callback on_connection_changed;
    error_callback on_error;

    explicit impl(std::string iface)
        : interface(std::move(iface))
    {
        // Check initial connection status
        auto info = get_connection_info(interface);
        if (info.wpa_state == "COMPLETED")
        {
            connection_status = wifi_connection_status::connected;
            current_network.ssid = info.ssid;
            current_network.bssid = info.bssid;
            current_network.frequency_mhz = info.frequency;
            current_network.band = frequency_to_band(info.frequency);
            current_network.is_connected = true;
        }

        spdlog::info(
            "WiFi manager initialized for interface {}, status: {}",
            interface,
            info.wpa_state);
    }

    void update_connection_status()
    {
        auto info = get_connection_info(interface);
        wifi_connection_status new_status;

        if (info.wpa_state == "COMPLETED")
        {
            new_status = wifi_connection_status::connected;
            current_network.ssid = info.ssid;
            current_network.bssid = info.bssid;
            current_network.frequency_mhz = info.frequency;
            current_network.band = frequency_to_band(info.frequency);
            current_network.is_connected = true;
        }
        else if (info.wpa_state == "ASSOCIATING" || info.wpa_state == "ASSOCIATED" ||
                 info.wpa_state == "4WAY_HANDSHAKE" ||
                 info.wpa_state == "GROUP_HANDSHAKE")
        {
            new_status = wifi_connection_status::connecting;
        }
        else
        {
            new_status = wifi_connection_status::disconnected;
            current_network = {};
        }

        if (new_status != connection_status)
        {
            auto prev_status = connection_status;
            connection_status = new_status;

            // Start DHCP when transitioning to connected.
            // Must use std::system with & to avoid blocking:
            // execute_command (popen) blocks until the child closes
            // stdout, but udhcpc runs as a long-lived daemon and
            // never exits, so popen would block forever.
            if (new_status == wifi_connection_status::connected &&
                prev_status != wifi_connection_status::connected)
            {
                spdlog::info("Starting udhcpc on {}", interface);
                std::string dhcp_cmd =
                    "udhcpc -i " + interface +
                    " -p /var/run/udhcpc." + interface + ".pid"
                    " -t 5 -T 2 -S"
                    " >/dev/null 2>&1 &";
                [[maybe_unused]] auto rc = std::system(dhcp_cmd.c_str());
            }

            if (on_connection_changed)
            {
                on_connection_changed(
                    connection_status,
                    connection_status == wifi_connection_status::connected
                        ? &current_network
                        : nullptr);
            }
        }
    }
};

wifi_manager::wifi_manager(std::string interface)
    : m_impl(std::make_unique<impl>(std::move(interface)))
{
}

wifi_manager::~wifi_manager() = default;

wifi_manager::wifi_manager(wifi_manager&&) noexcept = default;
wifi_manager& wifi_manager::operator=(wifi_manager&&) noexcept = default;

wifi_connection_status
wifi_manager::status() const noexcept
{
    return m_impl->connection_status;
}

std::string
wifi_manager::current_ssid() const
{
    return m_impl->current_network.ssid;
}

std::string
wifi_manager::ip_address() const
{
    return get_ip_address(m_impl->interface);
}

int
wifi_manager::signal_strength() const
{
    if (m_impl->connection_status != wifi_connection_status::connected)
        return 0;
    return get_current_signal(m_impl->interface);
}

bool
wifi_manager::is_interface_available() const
{
    std::string cmd = "ip link show " + m_impl->interface + " 2>/dev/null";
    std::string result = execute_command(cmd.c_str());
    return !result.empty();
}

void
wifi_manager::scan_networks()
{
    // Trigger scan — retry on FAIL-BUSY (wpa_supplicant may be
    // in its initial auto-scan right after startup)
    std::string result;
    constexpr int scan_retries = 3;
    for (int attempt = 0; attempt < scan_retries; ++attempt)
    {
        result = wpa_cli(m_impl->interface, "scan");

        if (result.find("OK") != std::string::npos)
            break;

        if (attempt + 1 < scan_retries)
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    }

    if (result.find("OK") == std::string::npos)
    {
        spdlog::warn("Failed to initiate WiFi scan");
        if (m_impl->on_error)
            m_impl->on_error("Failed to initiate WiFi scan");
        return;
    }

    // Wait briefly for scan to complete
    // In production, this should be async with event monitoring
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Get results
    result = wpa_cli(m_impl->interface, "scan_results");
    m_impl->cached_networks = parse_scan_results(result);

    // Mark current network as connected
    for (auto& net : m_impl->cached_networks)
    {
        if (net.ssid == m_impl->current_network.ssid)
        {
            net.is_connected = true;
        }
    }

    spdlog::info("WiFi scan complete, found {} networks", m_impl->cached_networks.size());

    if (m_impl->on_scan_complete)
    {
        m_impl->on_scan_complete(m_impl->cached_networks);
    }
}

std::vector<wifi_network> const&
wifi_manager::available_networks() const
{
    return m_impl->cached_networks;
}

int
wifi_manager::connect_start(
    std::string const& ssid,
    std::string const& password,
    bool remember)
{
    spdlog::info("Connecting to WiFi network: {}", ssid);

    // Clean up orphaned networks (empty SSID from crashed ADD_NETWORK
    // or stale config). These can interfere with SELECT_NETWORK.
    {
        std::string list = wpa_cli(m_impl->interface, "list_networks");
        std::istringstream stream(list);
        std::string line;
        std::getline(stream, line); // skip header
        while (std::getline(stream, line))
        {
            if (line.empty())
                continue;
            std::istringstream ls(line);
            std::string id, net_ssid;
            ls >> id >> net_ssid;
            if (net_ssid.empty() || net_ssid == "any")
            {
                wpa_cli(m_impl->interface, "remove_network " + id);
            }
        }
    }

    // Check if network is already saved
    auto saved = saved_networks();
    int network_id = -1;

    for (size_t i = 0; i < saved.size(); ++i)
    {
        if (saved[i].ssid == ssid)
        {
            network_id = static_cast<int>(i);
            break;
        }
    }

    // Find network in scan results to get security type
    wifi_security security = wifi_security::wpa2;
    for (auto const& net : m_impl->cached_networks)
    {
        if (net.ssid == ssid)
        {
            security = net.security;
            break;
        }
    }

    // Add network if not saved, or if we want to remember with new password
    if (network_id < 0 || remember)
    {
        network_id = add_network_to_wpa(m_impl->interface, ssid, password, security);
        if (network_id < 0)
        {
            spdlog::error("Failed to add network to wpa_supplicant");
            if (m_impl->on_error)
                m_impl->on_error("Failed to add network to wpa_supplicant");
            return -1;
        }
    }

    // Select and connect to network
    std::string id_str = std::to_string(network_id);
    wpa_cli(m_impl->interface, "select_network " + id_str);

    m_impl->connection_status = wifi_connection_status::connecting;
    if (m_impl->on_connection_changed)
    {
        m_impl->on_connection_changed(m_impl->connection_status, nullptr);
    }

    return network_id;
}

wifi_connection_status
wifi_manager::poll_connection()
{
    m_impl->update_connection_status();
    return m_impl->connection_status;
}

void
wifi_manager::save_config()
{
    wpa_cli(m_impl->interface, "save_config");
}

void
wifi_manager::disconnect()
{
    spdlog::info("Disconnecting from WiFi");
    [[maybe_unused]] auto rc = std::system("killall -q udhcpc 2>/dev/null");
    wpa_cli(m_impl->interface, "disconnect");
    m_impl->update_connection_status();
}

void
wifi_manager::reconnect()
{
    spdlog::info("Reconnecting to WiFi");
    wpa_cli(m_impl->interface, "reconnect");
    m_impl->update_connection_status();
}

std::vector<wifi_saved_network>
wifi_manager::saved_networks() const
{
    std::string output = wpa_cli(m_impl->interface, "list_networks");
    return parse_saved_networks(output);
}

bool
wifi_manager::forget_network(std::string const& ssid)
{
    spdlog::info("Forgetting network: {}", ssid);

    auto networks = saved_networks();
    for (size_t i = 0; i < networks.size(); ++i)
    {
        if (networks[i].ssid == ssid)
        {
            wpa_cli(m_impl->interface, "remove_network " + std::to_string(i));
            wpa_cli(m_impl->interface, "save_config");
            return true;
        }
    }

    return false;
}

void
wifi_manager::set_network_priority(std::string const& ssid, int priority)
{
    auto networks = saved_networks();
    for (size_t i = 0; i < networks.size(); ++i)
    {
        if (networks[i].ssid == ssid)
        {
            wpa_cli(
                m_impl->interface,
                "set_network " + std::to_string(i) + " priority " +
                    std::to_string(priority));
            wpa_cli(m_impl->interface, "save_config");
            return;
        }
    }
}

void
wifi_manager::set_network_auto_connect(std::string const& ssid, bool enabled)
{
    auto networks = saved_networks();
    for (size_t i = 0; i < networks.size(); ++i)
    {
        if (networks[i].ssid == ssid)
        {
            if (enabled)
            {
                wpa_cli(m_impl->interface, "enable_network " + std::to_string(i));
            }
            else
            {
                wpa_cli(m_impl->interface, "disable_network " + std::to_string(i));
            }
            wpa_cli(m_impl->interface, "save_config");
            return;
        }
    }
}

void
wifi_manager::set_auto_reconnect(bool enabled)
{
    m_impl->auto_reconnect_enabled = enabled;
    spdlog::info("Auto-reconnect {}", enabled ? "enabled" : "disabled");
}

bool
wifi_manager::auto_reconnect() const noexcept
{
    return m_impl->auto_reconnect_enabled;
}

void
wifi_manager::set_scan_complete_callback(scan_complete_callback cb)
{
    m_impl->on_scan_complete = std::move(cb);
}

void
wifi_manager::set_connection_changed_callback(connection_changed_callback cb)
{
    m_impl->on_connection_changed = std::move(cb);
}

void
wifi_manager::set_error_callback(error_callback cb)
{
    m_impl->on_error = std::move(cb);
}

} // namespace piejam::network_manager
