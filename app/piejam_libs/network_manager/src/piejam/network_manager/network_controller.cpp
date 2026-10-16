// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/network_manager/network_controller.h>

#include <spdlog/spdlog.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <unistd.h>

namespace piejam::network_manager
{

namespace
{

/// Execute a shell command and return success status
bool
execute_command(char const* cmd)
{
    int result = std::system(cmd);
    return result == 0;
}

/// Execute a shell command and capture output
std::string
execute_command_with_output(char const* cmd)
{
    std::array<char, 128> buffer;
    std::string result;

    auto pipe_deleter = [](FILE* f) { if (f) pclose(f); };
    std::unique_ptr<FILE, decltype(pipe_deleter)> pipe(popen(cmd, "r"), pipe_deleter);
    if (!pipe)
    {
        return result;
    }

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) !=
           nullptr)
    {
        result += buffer.data();
    }

    return result;
}

/// Start wpa_supplicant if not already running
bool
ensure_wpa_supplicant_running()
{
    // Check if wpa_supplicant is already running
    std::string pid = execute_command_with_output("pidof wpa_supplicant 2>/dev/null");
    if (!pid.empty())
    {
        spdlog::debug("wpa_supplicant already running (pid: {})", pid);
        return true;
    }

    // Try common config file locations — prefer subdirectory
    // (overlay provides /etc/wpa_supplicant/wpa_supplicant.conf
    // with update_config=1 and country code)
    char const* config_paths[] = {
        "/etc/wpa_supplicant/wpa_supplicant.conf",
        "/etc/wpa_supplicant/wpa_supplicant-wlan0.conf",
        "/etc/wpa_supplicant.conf",
    };

    for (auto const* config_path : config_paths)
    {
        std::string check_cmd = std::string("test -f ") + config_path;
        if (execute_command(check_cmd.c_str()))
        {
            std::string start_cmd =
                std::string("wpa_supplicant -B -i wlan0 -c ") +
                config_path + " -D nl80211 2>/dev/null";
            if (execute_command(start_cmd.c_str()))
            {
                spdlog::info("Started wpa_supplicant with config {}", config_path);
                return true;
            }
        }
    }

    // Try starting without a config file (will create default)
    if (execute_command("wpa_supplicant -B -i wlan0 -D nl80211 -C /var/run/wpa_supplicant 2>/dev/null"))
    {
        spdlog::info("Started wpa_supplicant with control interface only");
        return true;
    }

    spdlog::warn("Failed to start wpa_supplicant");
    return false;
}

/// Enable WiFi interface
bool
enable_wifi_interface()
{
    spdlog::info("Enabling WiFi interface");

    // Check if brcmfmac driver is loaded (built-in or module)
    std::string brcm_check = execute_command_with_output(
        "ls /sys/module/brcmfmac 2>/dev/null && echo loaded || echo not_loaded");

    // Try loading as module in case it's not built-in
    if (brcm_check.find("not_loaded") != std::string::npos)
    {
        [[maybe_unused]] auto rc = std::system("modprobe brcmfmac 2>&1");
    }

    // Wait for interface to appear
    bool iface_found = false;
    for (int i = 0; i < 10; ++i)
    {
        std::string check = execute_command_with_output("ip link show wlan0 2>/dev/null");
        if (!check.empty())
        {
            spdlog::info("wlan0 interface found");
            iface_found = true;
            break;
        }
        spdlog::debug("Waiting for wlan0... attempt {}/10", i + 1);
        usleep(500000); // 500ms
    }

    if (!iface_found)
    {
        // List all interfaces again after waiting
        std::string ifaces_after = execute_command_with_output("ip link show 2>/dev/null");
        spdlog::warn("wlan0 not found. All interfaces:\n{}", ifaces_after);
    }

    // Bring up the interface (no sudo needed, running as root on buildroot)
    if (!execute_command("ip link set wlan0 up 2>/dev/null"))
    {
        spdlog::warn("Failed to bring up wlan0 interface");
        return false;
    }

    spdlog::info("wlan0 interface is up");

    // Start wpa_supplicant for WiFi management
    if (!ensure_wpa_supplicant_running())
    {
        spdlog::warn("wpa_supplicant not running, WiFi scanning may not work");
        // Not fatal - interface is up, wpa_supplicant may start later
    }

    return true;
}

/// Disable WiFi interface
bool
disable_wifi_interface()
{
    spdlog::info("Disabling WiFi interface");

    // Stop DHCP client and wpa_supplicant
    execute_command("killall -q udhcpc 2>/dev/null");
    execute_command("killall wpa_supplicant 2>/dev/null");

    // Bring down the interface (no sudo needed, running as root)
    if (!execute_command("ip link set wlan0 down 2>/dev/null"))
    {
        spdlog::warn("Failed to bring down wlan0 interface");
        return false;
    }

    return true;
}

/// Unmount all NFS mounts
void
unmount_all_nfs()
{
    spdlog::info("Unmounting NFS shares");
    execute_command("umount -a -t nfs,nfs4 2>/dev/null");
}

} // namespace

struct network_controller::impl
{
    bool enabled{false};
    state_change_callback on_state_change;

    impl()
    {
        spdlog::info("Network controller initialized, WiFi disabled (user must enable)");
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

bool
network_controller::enable()
{
    if (m_impl->enabled)
    {
        spdlog::debug("Network already enabled");
        return true;
    }

    if (!enable_wifi_interface())
    {
        spdlog::error("Failed to enable network");
        return false;
    }

    m_impl->enabled = true;
    spdlog::info("Network enabled");

    if (m_impl->on_state_change)
    {
        m_impl->on_state_change(true);
    }

    return true;
}

bool
network_controller::disable()
{
    if (!m_impl->enabled)
    {
        spdlog::debug("Network already disabled");
        return true;
    }

    // First unmount NFS to prevent hangs
    unmount_all_nfs();

    if (!disable_wifi_interface())
    {
        spdlog::error("Failed to disable network");
        return false;
    }

    m_impl->enabled = false;
    spdlog::info("Network disabled");

    if (m_impl->on_state_change)
    {
        m_impl->on_state_change(false);
    }

    return true;
}

void
network_controller::set_state_change_callback(state_change_callback cb)
{
    m_impl->on_state_change = std::move(cb);
}

} // namespace piejam::network_manager
