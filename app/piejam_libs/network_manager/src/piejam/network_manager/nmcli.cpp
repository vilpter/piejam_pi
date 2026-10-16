// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/network_manager/nmcli.h>

#include <QProcess>
#include <QTimer>

#include <algorithm>
#include <charconv>
#include <map>
#include <memory>
#include <unordered_set>

namespace piejam::network_manager::nmcli
{

namespace
{

auto
to_int(std::string_view s) -> int
{
    int value{};
    auto const first = s.data();
    auto const last = s.data() + s.size();
    auto const begin =
        std::find_if(first, last, [](char c) { return c != ' '; });
    std::from_chars(begin, last, value);
    return value;
}

auto
contains(std::string_view s, std::string_view part) -> bool
{
    return s.find(part) != std::string_view::npos;
}

/// nmcli shows "--" for an empty value in some modes.
auto
value_or_empty(std::string_view value) -> std::string
{
    return value == "--" ? std::string{} : std::string{value};
}

/// Split "KEY:VALUE" at the first ':'. Multi-line output (device show)
/// doesn't escape ':' inside values, so only the first one separates.
auto
split_key_value(std::string_view line)
    -> std::pair<std::string_view, std::string_view>
{
    auto const pos = line.find(':');
    if (pos == std::string_view::npos)
    {
        return {line, {}};
    }
    return {line.substr(0, pos), line.substr(pos + 1)};
}

template <class F>
void
for_each_line(std::string_view text, F&& f)
{
    while (!text.empty())
    {
        auto const end = text.find('\n');
        auto line = text.substr(0, end);
        if (!line.empty() && line.back() == '\r')
        {
            line.remove_suffix(1);
        }
        if (!line.empty())
        {
            f(line);
        }
        if (end == std::string_view::npos)
        {
            break;
        }
        text.remove_prefix(end + 1);
    }
}

/// nmcli reports signal quality as a percentage. Estimate dBm with the
/// inverse of signal_dbm_to_percent().
auto
percent_to_dbm(int percent) -> int
{
    return percent * 60 / 100 - 90;
}

} // namespace

void
run(QObject& context,
    QStringList const& arguments,
    result_handler on_done,
    std::string const& stdin_data,
    int timeout_ms)
{
    auto* const process = new QProcess(&context);

    auto env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("LC_ALL"), QStringLiteral("C.UTF-8"));
    env.remove(QStringLiteral("LANGUAGE"));
    process->setProcessEnvironment(env);
    process->setProgram(QStringLiteral("nmcli"));
    process->setArguments(arguments);

    struct pending
    {
        result_handler on_done;
        bool timed_out{};
    };

    auto state = std::make_shared<pending>(pending{std::move(on_done)});

    auto finish = [process, state](result r) {
        if (auto handler = std::exchange(state->on_done, nullptr))
        {
            handler(r);
        }
        process->deleteLater();
    };

    QObject::connect(
        process,
        qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
        &context,
        [process, state, finish](int code, QProcess::ExitStatus status) {
            result r;
            r.ok = status == QProcess::NormalExit && code == 0;
            r.output = process->readAllStandardOutput().toStdString();
            r.error = state->timed_out
                          ? std::string{"nmcli timed out"}
                          : error_message(
                                process->readAllStandardError().toStdString());
            if (!r.ok && r.error.empty())
            {
                r.error = "nmcli failed";
            }
            finish(std::move(r));
        });

    QObject::connect(
        process,
        &QProcess::errorOccurred,
        &context,
        [process, finish](QProcess::ProcessError error) {
            // A crash or kill is reported through finished() as well.
            if (error == QProcess::FailedToStart)
            {
                finish(
                    result{
                        .ok = false,
                        .output = {},
                        .error = "nmcli could not be started: " +
                                 process->errorString().toStdString()});
            }
        });

    QTimer::singleShot(timeout_ms, process, [process, state]() {
        state->timed_out = true;
        process->kill();
    });

    process->start();

    if (!stdin_data.empty())
    {
        process->write(
            stdin_data.data(),
            static_cast<qint64>(stdin_data.size()));
    }
    process->closeWriteChannel();
}

auto
unescape(std::string_view value) -> std::string
{
    std::string result;
    result.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] == '\\' && i + 1 < value.size())
        {
            ++i;
        }
        result += value[i];
    }
    return result;
}

auto
split_fields(std::string_view line) -> std::vector<std::string>
{
    std::vector<std::string> fields(1);
    for (std::size_t i = 0; i < line.size(); ++i)
    {
        if (line[i] == '\\' && i + 1 < line.size())
        {
            fields.back() += line[++i];
        }
        else if (line[i] == ':')
        {
            fields.emplace_back();
        }
        else
        {
            fields.back() += line[i];
        }
    }
    return fields;
}

auto
parse_device_status(std::string_view output) -> device_status
{
    struct access_point
    {
        bool in_use{};
        std::string ssid;
        int signal{};
    };

    device_status status;
    std::map<int, access_point> access_points;

    for_each_line(output, [&](std::string_view line) {
        auto const [key, value] = split_key_value(line);

        if (key == "GENERAL.STATE")
        {
            status.state = to_int(value);
        }
        else if (key == "GENERAL.CONNECTION")
        {
            status.connection = value_or_empty(value);
        }
        else if (key.starts_with("IP4.ADDRESS") && status.ip_address.empty())
        {
            status.ip_address = std::string{value.substr(0, value.find('/'))};
        }
        else if (key.starts_with("AP["))
        {
            auto const close = key.find("].");
            if (close == std::string_view::npos)
            {
                return;
            }
            auto& ap = access_points[to_int(key.substr(3, close - 3))];
            auto const field = key.substr(close + 2);
            if (field == "IN-USE")
            {
                ap.in_use = value == "*";
            }
            else if (field == "SSID")
            {
                ap.ssid = std::string{value};
            }
            else if (field == "SIGNAL")
            {
                ap.signal = to_int(value);
            }
        }
    });

    for (auto const& [index, ap] : access_points)
    {
        if (ap.in_use)
        {
            status.active_ssid = ap.ssid;
            status.active_signal = ap.signal;
            break;
        }
    }

    return status;
}

auto
parse_security(std::string_view security) -> wifi_security
{
    if (contains(security, "802.1X"))
    {
        return wifi_security::unknown; // enterprise; needs more than a password
    }
    if (contains(security, "WPA3") && !contains(security, "WPA2") &&
        !contains(security, "WPA1"))
    {
        return wifi_security::wpa3;
    }
    if (contains(security, "WPA2"))
    {
        return wifi_security::wpa2;
    }
    if (contains(security, "WPA"))
    {
        return wifi_security::wpa;
    }
    if (contains(security, "WEP"))
    {
        return wifi_security::wep;
    }
    if (security.empty() || security == "--" || contains(security, "OWE"))
    {
        return wifi_security::open;
    }
    return wifi_security::unknown;
}

auto
parse_wifi_list(std::string_view output) -> std::vector<wifi_network>
{
    std::vector<wifi_network> networks;

    for_each_line(output, [&](std::string_view line) {
        auto const fields = split_fields(line);
        if (fields.size() < 6 || fields[1].empty())
        {
            return; // malformed, or a hidden network
        }

        wifi_network net;
        net.is_connected = fields[0] == "*";
        net.ssid = fields[1];
        net.bssid = fields[2];
        net.frequency_mhz = to_int(fields[3]);
        net.signal_percent = std::clamp(to_int(fields[4]), 0, 100);
        net.signal_strength = percent_to_dbm(net.signal_percent);
        net.band = frequency_to_band(net.frequency_mhz);
        net.security = parse_security(fields[5]);
        networks.push_back(std::move(net));
    });

    // Keep one entry per SSID: the access point in use, else the strongest.
    std::ranges::stable_sort(
        networks,
        [](wifi_network const& a, wifi_network const& b) {
            if (a.is_connected != b.is_connected)
            {
                return a.is_connected;
            }
            return a.signal_percent > b.signal_percent;
        });

    std::unordered_set<std::string> seen;
    std::erase_if(networks, [&seen](wifi_network const& net) {
        return !seen.insert(net.ssid).second;
    });

    std::ranges::stable_sort(
        networks,
        [](wifi_network const& a, wifi_network const& b) {
            return a.signal_percent > b.signal_percent;
        });

    return networks;
}

auto
parse_connections(std::string_view output) -> std::vector<connection>
{
    std::vector<connection> connections;

    for_each_line(output, [&](std::string_view line) {
        auto const fields = split_fields(line);
        if (fields.size() < 5 || fields[1].empty())
        {
            return;
        }

        connections.push_back(
            connection{
                .name = fields[0],
                .uuid = fields[1],
                .type = fields[2],
                .autoconnect = fields[3] == "yes",
                .priority = to_int(fields[4])});
    });

    return connections;
}

auto
key_mgmt_to_security(std::string_view key_mgmt) -> wifi_security
{
    if (key_mgmt.empty() || key_mgmt == "owe")
    {
        return wifi_security::open;
    }
    if (key_mgmt == "none")
    {
        return wifi_security::wep; // static WEP keys
    }
    if (key_mgmt == "wpa-psk")
    {
        return wifi_security::wpa2;
    }
    if (key_mgmt == "sae")
    {
        return wifi_security::wpa3;
    }
    return wifi_security::unknown;
}

auto
security_to_key_mgmt(wifi_security security) -> std::string
{
    switch (security)
    {
        case wifi_security::open:
            return {};
        case wifi_security::wep:
            return "none";
        case wifi_security::wpa3:
            return "sae";
        case wifi_security::wpa:
        case wifi_security::wpa2:
        case wifi_security::unknown:
            // WPA2 also covers WPA2/WPA3 transition networks.
            return "wpa-psk";
    }
    return "wpa-psk";
}

auto
parse_added_uuid(std::string_view output) -> std::string
{
    // Connection 'NAME' (UUID) successfully added.
    auto const suffix = output.rfind(") successfully added");
    if (suffix == std::string_view::npos)
    {
        return {};
    }
    auto const open = output.rfind('(', suffix);
    if (open == std::string_view::npos)
    {
        return {};
    }
    return std::string{output.substr(open + 1, suffix - open - 1)};
}

auto
error_message(std::string_view error_output) -> std::string
{
    std::string message;
    for_each_line(error_output, [&](std::string_view line) {
        if (message.empty())
        {
            if (line.starts_with("Error: "))
            {
                line.remove_prefix(7);
            }
            message = std::string{line};
        }
    });
    return message;
}

} // namespace piejam::network_manager::nmcli
