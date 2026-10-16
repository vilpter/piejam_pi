// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/network_manager/wifi_manager.h>

#include <piejam/network_manager/nmcli.h>

#include <spdlog/spdlog.h>

#include <QObject>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <memory>

namespace piejam::network_manager
{

namespace
{

auto
qs(std::string const& s) -> QString
{
    return QString::fromStdString(s);
}

/// nmcli's interactive editor trims values and reads one command per line,
/// so passwords with surrounding spaces or control characters can't be
/// passed through it faithfully. Returns an error message, or empty.
auto
check_password(std::string const& password) -> std::string
{
    if (std::ranges::any_of(password, [](unsigned char c) {
            return c < 0x20 || c == 0x7f;
        }))
    {
        return "The password contains control characters.";
    }
    if (!password.empty() &&
        (std::isspace(static_cast<unsigned char>(password.front())) ||
         std::isspace(static_cast<unsigned char>(password.back()))))
    {
        return "Passwords that start or end with a space aren't supported.";
    }
    return {};
}

/// The profile property that holds the password for a security type.
auto
secret_property(wifi_security security) -> char const*
{
    return security == wifi_security::wep ? "802-11-wireless-security.wep-key0"
                                          : "802-11-wireless-security.psk";
}

/// Split `nmcli -g` output into its values, one per line, keeping empty
/// values (a profile without security has an empty key-mgmt).
auto
get_values(std::string const& output) -> std::vector<std::string>
{
    std::vector<std::string> values;
    std::size_t start = 0;
    while (start < output.size())
    {
        auto const end = output.find('\n', start);
        values.push_back(
            nmcli::unescape(
                std::string_view{output}.substr(
                    start,
                    end == std::string::npos ? std::string::npos
                                             : end - start)));
        if (end == std::string::npos)
        {
            break;
        }
        start = end + 1;
    }
    return values;
}

} // namespace

struct wifi_manager::impl
{
    std::string interface;
    wifi_connection_status connection_status{
        wifi_connection_status::disconnected};
    wifi_network current_network;
    std::string current_ip;
    std::vector<wifi_network> cached_networks;
    std::vector<wifi_saved_network> saved;
    bool auto_reconnect_enabled{true};

    bool status_pending{};
    bool scan_pending{};
    unsigned saved_generation{};

    scan_complete_callback on_scan_complete;
    connection_changed_callback on_connection_changed;
    saved_networks_callback on_saved_networks;
    error_callback on_error;

    // Owns the running nmcli processes. Declared last so it's destroyed
    // first: a pending process can't call back into a half-destroyed impl.
    std::unique_ptr<QObject> context{std::make_unique<QObject>()};

    explicit impl(std::string iface)
        : interface(std::move(iface))
    {
        spdlog::debug(
            "WiFi manager initialized for interface {} (NetworkManager)",
            interface);
    }

    void
    run(QStringList const& args,
        nmcli::result_handler on_done,
        std::string const& stdin_data = {},
        int timeout_ms = 15000)
    {
        nmcli::run(*context, args, std::move(on_done), stdin_data, timeout_ms);
    }

    void report_error(std::string const& message)
    {
        spdlog::warn("WiFi: {}", message);
        if (on_error)
        {
            on_error(message);
        }
    }

    auto find_saved(std::string const& ssid) const -> wifi_saved_network const*
    {
        auto it = std::ranges::find(saved, ssid, &wifi_saved_network::ssid);
        return it != saved.end() ? &*it : nullptr;
    }

    auto find_available(std::string const& ssid) const -> wifi_network const*
    {
        auto it = std::ranges::find(cached_networks, ssid, &wifi_network::ssid);
        return it != cached_networks.end() ? &*it : nullptr;
    }

    void refresh_status()
    {
        if (status_pending)
        {
            return;
        }
        status_pending = true;

        run({"-t",
             "-f",
             "GENERAL.STATE,GENERAL.CONNECTION,IP4.ADDRESS,AP",
             "device",
             "show",
             qs(interface)},
            [this](nmcli::result const& r) {
                status_pending = false;
                // A missing device or a stopped NetworkManager reads as
                // disconnected.
                apply_status(
                    r.ok ? nmcli::parse_device_status(r.output)
                         : nmcli::device_status{});
            });
    }

    void apply_status(nmcli::device_status const& st)
    {
        auto status = wifi_connection_status::disconnected;
        if (st.state == nmcli::device_state_activated)
        {
            status = wifi_connection_status::connected;
        }
        else if (
            st.state >= nmcli::device_state_prepare &&
            st.state < nmcli::device_state_activated)
        {
            status = wifi_connection_status::connecting;
        }

        wifi_network network;
        std::string ip;
        if (status == wifi_connection_status::connected)
        {
            auto const ssid =
                st.active_ssid.empty() ? st.connection : st.active_ssid;
            if (auto const* known = find_available(ssid))
            {
                network = *known;
            }
            network.ssid = ssid;
            network.signal_percent = st.active_signal;
            network.is_connected = true;
            ip = st.ip_address;
        }

        for (auto& net : cached_networks)
        {
            net.is_connected =
                !network.ssid.empty() && net.ssid == network.ssid;
        }

        bool const changed =
            status != connection_status ||
            network.ssid != current_network.ssid ||
            network.signal_percent != current_network.signal_percent ||
            ip != current_ip;

        connection_status = status;
        current_network = std::move(network);
        current_ip = std::move(ip);

        if (changed && on_connection_changed)
        {
            on_connection_changed(
                connection_status,
                connection_status == wifi_connection_status::connected
                    ? &current_network
                    : nullptr);
        }
    }

    void list_networks(bool rescan)
    {
        if (scan_pending)
        {
            return;
        }
        scan_pending = true;

        run(
            {"-t",
             "-f",
             "IN-USE,SSID,BSSID,FREQ,SIGNAL,SECURITY",
             "device",
             "wifi",
             "list",
             "ifname",
             qs(interface),
             "--rescan",
             rescan ? "yes" : "no"},
            [this, rescan](nmcli::result const& r) {
                scan_pending = false;
                if (r.ok)
                {
                    // nmcli marks the network in use; apply_status() keeps
                    // the mark current afterwards.
                    cached_networks = nmcli::parse_wifi_list(r.output);
                    if (rescan)
                    {
                        spdlog::debug(
                            "WiFi scan complete, found {} networks",
                            cached_networks.size());
                    }
                }
                else
                {
                    report_error("WiFi scan failed: " + r.error);
                }
                if (on_scan_complete)
                {
                    on_scan_complete(cached_networks);
                }
            },
            {},
            30000);
    }

    void refresh_saved_networks()
    {
        auto const generation = ++saved_generation;

        run({"-t",
             "-f",
             "NAME,UUID,TYPE,AUTOCONNECT,AUTOCONNECT-PRIORITY",
             "connection",
             "show"},
            [this, generation](nmcli::result const& r) {
                if (generation != saved_generation)
                {
                    return; // a newer refresh is under way
                }
                if (!r.ok)
                {
                    report_error("Couldn't list saved networks: " + r.error);
                    return;
                }

                auto profiles =
                    std::make_shared<std::vector<wifi_saved_network>>();
                for (auto const& c : nmcli::parse_connections(r.output))
                {
                    if (c.type == nmcli::wifi_connection_type)
                    {
                        profiles->push_back(
                            wifi_saved_network{
                                .ssid = {},
                                .priority = c.priority,
                                .auto_connect = c.autoconnect,
                                .hidden = false,
                                .security = wifi_security::unknown,
                                .uuid = c.uuid});
                    }
                }
                load_profile_details(generation, std::move(profiles));
            });
    }

    /// Look up each profile's SSID and security, then publish the list.
    void load_profile_details(
        unsigned generation,
        std::shared_ptr<std::vector<wifi_saved_network>> profiles)
    {
        auto remaining = std::make_shared<std::size_t>(profiles->size());

        auto publish = [this, generation, profiles]() {
            if (generation != saved_generation)
            {
                return;
            }
            std::erase_if(*profiles, [](wifi_saved_network const& n) {
                return n.ssid.empty();
            });
            std::ranges::stable_sort(
                *profiles,
                [](wifi_saved_network const& a, wifi_saved_network const& b) {
                    return a.priority > b.priority;
                });
            saved = std::move(*profiles);
            if (on_saved_networks)
            {
                on_saved_networks(saved);
            }
        };

        if (profiles->empty())
        {
            publish();
            return;
        }

        for (std::size_t i = 0; i < profiles->size(); ++i)
        {
            run({"-t",
                 "-g",
                 "802-11-wireless.ssid,802-11-wireless-security.key-mgmt,"
                 "802-11-wireless.hidden",
                 "connection",
                 "show",
                 "uuid",
                 qs((*profiles)[i].uuid)},
                [profiles, remaining, publish, i](nmcli::result const& r) {
                    if (r.ok)
                    {
                        auto const values = get_values(r.output);
                        auto& profile = (*profiles)[i];
                        profile.ssid = values.empty() ? "" : values[0];
                        profile.security = nmcli::key_mgmt_to_security(
                            values.size() > 1 ? values[1] : "");
                        profile.hidden =
                            values.size() > 2 && values[2] == "yes";
                    }
                    if (--*remaining == 0)
                    {
                        publish();
                    }
                });
        }
    }

    /// Store a password in a profile through nmcli's editor, which reads it
    /// from standard input.
    void set_secret(
        std::string const& uuid,
        wifi_security security,
        std::string const& password,
        bool persistent,
        done_callback on_done)
    {
        std::string commands = "set ";
        commands += secret_property(security);
        commands += ' ';
        commands += password;
        commands += persistent ? "\nsave persistent\n" : "\nsave temporary\n";
        commands += "quit\n";

        run(
            {"connection", "edit", "uuid", qs(uuid)},
            [on_done = std::move(on_done)](nmcli::result const& r) {
                // The editor exits with status 0 even if saving failed.
                if (r.ok &&
                    r.output.find("successfully updated") != std::string::npos)
                {
                    on_done(true, {});
                    return;
                }
                if (auto const pos = r.output.find("Error: ");
                    pos != std::string::npos)
                {
                    auto const message = r.output.substr(pos + 7);
                    on_done(false, message.substr(0, message.find('\n')));
                    return;
                }
                on_done(
                    false,
                    r.error.empty() ? "Couldn't save the password" : r.error);
            },
            commands);
    }

    /// Bring up a profile. A profile created just for this attempt is
    /// deleted again if it fails, so a wrong password leaves nothing behind.
    void activate(
        std::string const& uuid,
        bool delete_on_failure,
        done_callback on_done)
    {
        run(
            {"--wait",
             "60",
             "connection",
             "up",
             "uuid",
             qs(uuid),
             "ifname",
             qs(interface)},
            [this, uuid, delete_on_failure, on_done = std::move(on_done)](
                nmcli::result const& r) {
                if (!r.ok && delete_on_failure)
                {
                    run({"connection", "delete", "uuid", qs(uuid)},
                        [this](nmcli::result const&) {
                            refresh_saved_networks();
                        });
                }
                else
                {
                    refresh_saved_networks();
                }
                refresh_status();
                on_done(r.ok, r.error);
            },
            {},
            75000);
    }

    /// Create a profile for a network that isn't saved yet, then connect.
    void add_and_activate(
        std::string const& ssid,
        std::string const& password,
        bool remember,
        done_callback on_done)
    {
        auto security =
            password.empty() ? wifi_security::open : wifi_security::wpa2;
        if (auto const* net = find_available(ssid))
        {
            security = net->security;
        }

        QStringList args{
            "connection",
            "add",
            "type",
            "wifi",
            "ifname",
            qs(interface),
            "con-name",
            qs(ssid),
            "ssid",
            qs(ssid),
            "save",
            remember ? "yes" : "no"};
        if (auto const key_mgmt = nmcli::security_to_key_mgmt(security);
            !key_mgmt.empty())
        {
            args << "802-11-wireless-security.key-mgmt" << qs(key_mgmt);
        }

        run(args,
            [this, password, remember, security, on_done = std::move(on_done)](
                nmcli::result const& r) {
                auto const uuid = nmcli::parse_added_uuid(r.output);
                if (!r.ok || uuid.empty())
                {
                    on_done(
                        false,
                        r.error.empty() ? "Couldn't add the network" : r.error);
                    return;
                }

                if (password.empty() || security == wifi_security::open)
                {
                    activate(uuid, true, on_done);
                    return;
                }

                set_secret(
                    uuid,
                    security,
                    password,
                    remember,
                    [this, uuid, on_done](bool ok, std::string const& error) {
                        if (ok)
                        {
                            activate(uuid, true, on_done);
                            return;
                        }
                        run({"connection", "delete", "uuid", qs(uuid)},
                            [this](nmcli::result const&) {
                                refresh_saved_networks();
                            });
                        on_done(false, error);
                    });
            });
    }
};

wifi_manager::wifi_manager(std::string interface)
    : m_impl(std::make_unique<impl>(std::move(interface)))
{
}

wifi_manager::~wifi_manager() = default;

wifi_manager::wifi_manager(wifi_manager&&) noexcept = default;
wifi_manager& wifi_manager::operator=(wifi_manager&&) noexcept = default;

void
wifi_manager::refresh_status()
{
    m_impl->refresh_status();
}

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
    return m_impl->current_ip;
}

int
wifi_manager::signal_strength() const
{
    return m_impl->current_network.signal_percent;
}

bool
wifi_manager::is_interface_available() const
{
    std::error_code ec;
    return std::filesystem::exists(
        std::filesystem::path{"/sys/class/net"} / m_impl->interface,
        ec);
}

void
wifi_manager::scan_networks()
{
    m_impl->list_networks(true);
}

void
wifi_manager::list_networks()
{
    m_impl->list_networks(false);
}

std::vector<wifi_network> const&
wifi_manager::available_networks() const
{
    return m_impl->cached_networks;
}

void
wifi_manager::connect(
    std::string const& ssid,
    std::string const& password,
    bool remember,
    done_callback on_done)
{
    spdlog::debug("Connecting to WiFi network: {}", ssid);

    done_callback done =
        [ssid,
         on_done = std::move(on_done)](bool ok, std::string const& error) {
            if (ok)
            {
                spdlog::info("Connected to WiFi network: {}", ssid);
            }
            else
            {
                spdlog::warn("Couldn't connect to {}: {}", ssid, error);
            }
            if (on_done)
            {
                on_done(ok, error);
            }
        };

    if (auto const problem = check_password(password); !problem.empty())
    {
        done(false, problem);
        return;
    }

    auto const* saved = m_impl->find_saved(ssid);
    if (!saved)
    {
        m_impl->add_and_activate(ssid, password, remember, std::move(done));
        return;
    }

    auto const uuid = saved->uuid;
    if (password.empty())
    {
        m_impl->activate(uuid, false, std::move(done));
        return;
    }

    // A saved network with a new password: update it, then connect.
    m_impl->set_secret(
        uuid,
        saved->security,
        password,
        true,
        [impl = m_impl.get(), uuid, done](bool ok, std::string const& error) {
            if (ok)
            {
                impl->activate(uuid, false, done);
            }
            else
            {
                done(false, error);
            }
        });
}

void
wifi_manager::disconnect(done_callback on_done)
{
    spdlog::info("Disconnecting from WiFi");
    m_impl->run(
        {"device", "disconnect", qs(m_impl->interface)},
        [impl = m_impl.get(),
         on_done = std::move(on_done)](nmcli::result const& r) {
            impl->refresh_status();
            if (!r.ok)
            {
                impl->report_error("Couldn't disconnect: " + r.error);
            }
            if (on_done)
            {
                on_done(r.ok, r.error);
            }
        });
}

void
wifi_manager::reconnect(done_callback on_done)
{
    spdlog::debug("Reconnecting to WiFi");
    m_impl->run(
        {"--wait", "60", "device", "connect", qs(m_impl->interface)},
        [impl = m_impl.get(),
         on_done = std::move(on_done)](nmcli::result const& r) {
            impl->refresh_status();
            if (on_done)
            {
                on_done(r.ok, r.error);
            }
        },
        {},
        75000);
}

void
wifi_manager::refresh_saved_networks()
{
    m_impl->refresh_saved_networks();
}

std::vector<wifi_saved_network> const&
wifi_manager::saved_networks() const
{
    return m_impl->saved;
}

bool
wifi_manager::forget_network(std::string const& ssid, done_callback on_done)
{
    // NetworkManager may hold more than one profile for an SSID.
    QStringList args{"connection", "delete"};
    for (auto const& net : m_impl->saved)
    {
        if (net.ssid == ssid)
        {
            args << "uuid" << qs(net.uuid);
        }
    }
    if (args.size() == 2)
    {
        return false;
    }

    spdlog::info("Forgetting network: {}", ssid);
    m_impl->run(
        args,
        [impl = m_impl.get(),
         on_done = std::move(on_done)](nmcli::result const& r) {
            impl->refresh_saved_networks();
            impl->refresh_status();
            if (!r.ok)
            {
                impl->report_error("Couldn't forget the network: " + r.error);
            }
            if (on_done)
            {
                on_done(r.ok, r.error);
            }
        });
    return true;
}

void
wifi_manager::set_network_priority(std::string const& ssid, int priority)
{
    if (auto const* net = m_impl->find_saved(ssid))
    {
        m_impl->run(
            {"connection",
             "modify",
             "uuid",
             qs(net->uuid),
             "connection.autoconnect-priority",
             QString::number(priority)},
            [impl = m_impl.get()](nmcli::result const&) {
                impl->refresh_saved_networks();
            });
    }
}

void
wifi_manager::set_network_auto_connect(std::string const& ssid, bool enabled)
{
    if (auto const* net = m_impl->find_saved(ssid))
    {
        m_impl->run(
            {"connection",
             "modify",
             "uuid",
             qs(net->uuid),
             "connection.autoconnect",
             enabled ? "yes" : "no"},
            [impl = m_impl.get()](nmcli::result const&) {
                impl->refresh_saved_networks();
            });
    }
}

void
wifi_manager::set_auto_reconnect(bool enabled)
{
    m_impl->auto_reconnect_enabled = enabled;
    spdlog::debug("Auto-reconnect {}", enabled ? "enabled" : "disabled");
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
wifi_manager::set_saved_networks_callback(saved_networks_callback cb)
{
    m_impl->on_saved_networks = std::move(cb);
}

void
wifi_manager::set_error_callback(error_callback cb)
{
    m_impl->on_error = std::move(cb);
}

} // namespace piejam::network_manager
