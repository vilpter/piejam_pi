// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/network_manager/network_controller.h>
#include <piejam/network_manager/nmcli.h>
#include <piejam/network_manager/wifi_manager.h>

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QTimer>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

// Runs the backends against a scripted stand-in for nmcli, placed first on
// PATH. The fake logs every call (one bracketed argument at a time), its
// standard input and its locale, and answers from files in its directory.

namespace piejam::network_manager::test
{

namespace
{

namespace fs = std::filesystem;

constexpr char const* fake_nmcli = R"(#!/bin/bash
dir=$(dirname "$0")
# One append per call, so concurrent calls can't interleave their records.
record='CALL'
for a in "$@"; do record+=" [$a]"; done
input=$(cat)
if [ -n "$input" ]; then
    record+=$'\n'"STDIN<<$input>>"
fi
record+=$'\n'"ENV LC_ALL=$LC_ALL"
printf '%s\n' "$record" >> "$dir/calls.log"

case "$*" in
"-t -f WIFI general")
    cat "$dir/radio" ;;
"radio wifi on"|"radio wifi off")
    [ -f "$dir/radio_fails" ] && { echo "Error: Not authorized." >&2; exit 1; }
    echo "${3/on/enabled}" | sed 's/off/disabled/' > "$dir/radio" ;;
"-t -f GENERAL.STATE,GENERAL.CONNECTION,IP4.ADDRESS,AP device show wlan0")
    cat "$dir/device_show" ;;
"-t -f IN-USE,SSID,BSSID,FREQ,SIGNAL,SECURITY device wifi list ifname wlan0 --rescan "*)
    [ -f "$dir/scan_fails" ] && { echo "Error: Scanning not allowed." >&2; exit 1; }
    cat "$dir/wifi_list" ;;
"-t -f NAME,UUID,TYPE,AUTOCONNECT,AUTOCONNECT-PRIORITY connection show")
    cat "$dir/connections" ;;
"-t -g 802-11-wireless.ssid,802-11-wireless-security.key-mgmt,802-11-wireless.hidden connection show uuid "*)
    cat "$dir/profile-${!#}" ;;
"connection add "*)
    echo "Connection 'x' (11111111-2222-3333-4444-555555555555) successfully added." ;;
"connection edit "*)
    echo "nmcli> set ..."
    echo "Connection 'x' (11111111-2222-3333-4444-555555555555) successfully updated." ;;
"--wait 60 connection up "*)
    if [ -f "$dir/up_fails" ]; then
        echo "Error: Connection activation failed: Secrets were required, but not provided." >&2
        exit 4
    fi
    echo "Connection successfully activated" ;;
"connection delete "*|"device disconnect wlan0")
    ;;
"sleep")
    sleep 5 ;;
*)
    echo "Error: unexpected arguments" >&2
    exit 2 ;;
esac
)";

auto
app() -> QCoreApplication&
{
    static int argc = 1;
    static char arg0[] = "piejam_network_manager_test";
    static char* argv[] = {arg0, nullptr};
    static QCoreApplication instance(argc, argv);
    return instance;
}

/// Run the event loop until pred() holds or the timeout passes.
template <class Pred>
auto
wait_for(Pred&& pred, int timeout_ms = 5000) -> bool
{
    app();
    QElapsedTimer timer;
    timer.start();
    while (!pred() && timer.elapsed() < timeout_ms)
    {
        QEventLoop loop;
        QTimer::singleShot(10, &loop, &QEventLoop::quit);
        loop.exec();
    }
    return pred();
}

class nmcli_process_test : public ::testing::Test
{
protected:
    void SetUp() override
    {
        app();
        char tmpl[] = "/tmp/piejam_fake_nmcli_XXXXXX";
        ASSERT_NE(::mkdtemp(tmpl), nullptr);
        dir = tmpl;

        write("nmcli", fake_nmcli);
        fs::permissions(dir / "nmcli", fs::perms::owner_all);

        write("radio", "enabled\n");
        write(
            "device_show",
            "GENERAL.STATE:100 (connected)\n"
            "GENERAL.CONNECTION:netplan-wlan0-Turkey\n"
            "IP4.ADDRESS[1]:192.168.30.234/24\n"
            "AP[1].IN-USE:*\n"
            "AP[1].SSID:Turkey\n"
            "AP[1].SIGNAL:63\n");
        write(
            "wifi_list",
            "*:Turkey:E8\\:9F\\:80\\:5C\\:17\\:77:2412 MHz:63:WPA2 WPA3\n"
            " :Studio:00\\:11\\:22\\:33\\:44\\:55:5180 MHz:40:WPA3\n"
            " :Cafe:00\\:11\\:22\\:33\\:44\\:56:2437 MHz:20:\n");
        write(
            "connections",
            "netplan-wlan0-Turkey:aaaaaaaa-0000-0000-0000-000000000001:"
            "802-11-wireless:yes:0\n"
            "Studio:aaaaaaaa-0000-0000-0000-000000000002:"
            "802-11-wireless:yes:10\n"
            "netplan-eth0:aaaaaaaa-0000-0000-0000-000000000003:"
            "802-3-ethernet:yes:0\n");
        write(
            "profile-aaaaaaaa-0000-0000-0000-000000000001",
            "Turkey\nwpa-psk\nno\n");
        write(
            "profile-aaaaaaaa-0000-0000-0000-000000000002",
            "Studio\nsae\nno\n");

        old_path = std::getenv("PATH") ? std::getenv("PATH") : "";
        ::setenv("PATH", (dir.string() + ":" + old_path).c_str(), 1);
    }

    void TearDown() override
    {
        ::setenv("PATH", old_path.c_str(), 1);
        fs::remove_all(dir);
    }

    void write(std::string const& name, std::string const& content)
    {
        std::ofstream{dir / name} << content;
    }

    void touch(std::string const& name)
    {
        write(name, "");
    }

    auto log() const -> std::string
    {
        std::ifstream in{dir / "calls.log"};
        std::stringstream ss;
        ss << in.rdbuf();
        return ss.str();
    }

    auto calls() const -> int
    {
        auto const text = log();
        int n = 0;
        for (std::size_t pos = 0;
             (pos = text.find("CALL", pos)) != std::string::npos;
             ++pos)
        {
            ++n;
        }
        return n;
    }

    /// Load the saved networks so connect() knows which ones exist.
    void load_saved(wifi_manager& mgr)
    {
        bool loaded = false;
        mgr.set_saved_networks_callback(
            [&](std::vector<wifi_saved_network> const&) { loaded = true; });
        mgr.refresh_saved_networks();
        ASSERT_TRUE(wait_for([&] { return loaded; }));
        mgr.set_saved_networks_callback({});
        fs::remove(dir / "calls.log");
    }

    struct outcome
    {
        bool done{};
        bool ok{};
        std::string error;
    };

    auto connect(
        wifi_manager& mgr,
        std::string const& ssid,
        std::string const& password,
        bool remember) -> outcome
    {
        outcome o;
        mgr.connect(
            ssid,
            password,
            remember,
            [&](bool ok, std::string const& e) {
                o.done = true;
                o.ok = ok;
                o.error = e;
            });
        wait_for([&] { return o.done; });
        // Let follow-up refreshes and cleanups finish too.
        wait_for([] { return false; }, 300);
        return o;
    }

    fs::path dir;
    std::string old_path;
};

constexpr char const* added_uuid = "11111111-2222-3333-4444-555555555555";

} // namespace

TEST_F(nmcli_process_test, runs_with_utf8_c_locale_and_no_shell)
{
    nmcli::result got;
    bool done = false;
    QObject context;
    nmcli::run(
        context,
        {"-t", "-f", "WIFI", "general"},
        [&](nmcli::result const& r) {
            got = r;
            done = true;
        });
    ASSERT_TRUE(wait_for([&] { return done; }));
    EXPECT_TRUE(got.ok);
    EXPECT_EQ(got.output, "enabled\n");
    EXPECT_NE(log().find("ENV LC_ALL=C.UTF-8"), std::string::npos);
}

TEST_F(nmcli_process_test, reports_nmcli_errors)
{
    nmcli::result got;
    bool done = false;
    QObject context;
    nmcli::run(context, {"bogus"}, [&](nmcli::result const& r) {
        got = r;
        done = true;
    });
    ASSERT_TRUE(wait_for([&] { return done; }));
    EXPECT_FALSE(got.ok);
    EXPECT_EQ(got.error, "unexpected arguments");
}

TEST_F(nmcli_process_test, times_out)
{
    nmcli::result got;
    bool done = false;
    QObject context;
    nmcli::run(
        context,
        {"sleep"},
        [&](nmcli::result const& r) {
            got = r;
            done = true;
        },
        {},
        200);
    ASSERT_TRUE(wait_for([&] { return done; }, 3000));
    EXPECT_FALSE(got.ok);
    EXPECT_EQ(got.error, "nmcli timed out");
}

TEST_F(nmcli_process_test, reports_missing_nmcli)
{
    ::setenv("PATH", dir.string().c_str(), 1);
    fs::remove(dir / "nmcli");

    nmcli::result got;
    bool done = false;
    QObject context;
    nmcli::run(context, {"general"}, [&](nmcli::result const& r) {
        got = r;
        done = true;
    });
    ASSERT_TRUE(wait_for([&] { return done; }));
    EXPECT_FALSE(got.ok);
    EXPECT_NE(got.error.find("could not be started"), std::string::npos);
}

TEST_F(nmcli_process_test, no_callback_after_context_is_destroyed)
{
    bool called = false;
    {
        QObject context;
        nmcli::run(context, {"sleep"}, [&](nmcli::result const&) {
            called = true;
        });
    }
    wait_for([] { return false; }, 300);
    EXPECT_FALSE(called);
}

TEST_F(nmcli_process_test, refresh_status_reports_connection_once)
{
    wifi_manager mgr;
    int changes = 0;
    wifi_connection_status last{};
    std::string ssid;
    mgr.set_connection_changed_callback(
        [&](wifi_connection_status s, wifi_network const* net) {
            ++changes;
            last = s;
            ssid = net ? net->ssid : "";
        });

    mgr.refresh_status();
    ASSERT_TRUE(wait_for([&] { return changes == 1; }));
    EXPECT_EQ(last, wifi_connection_status::connected);
    EXPECT_EQ(ssid, "Turkey");
    EXPECT_EQ(mgr.status(), wifi_connection_status::connected);
    EXPECT_EQ(mgr.current_ssid(), "Turkey");
    EXPECT_EQ(mgr.ip_address(), "192.168.30.234");
    EXPECT_EQ(mgr.signal_strength(), 63);

    // Nothing changed: no second notification.
    mgr.refresh_status();
    wait_for([] { return false; }, 300);
    EXPECT_EQ(changes, 1);

    // NetworkManager dropped the connection.
    write(
        "device_show",
        "GENERAL.STATE:30 (disconnected)\nGENERAL.CONNECTION:\n");
    mgr.refresh_status();
    ASSERT_TRUE(wait_for([&] { return changes == 2; }));
    EXPECT_EQ(last, wifi_connection_status::disconnected);
    EXPECT_EQ(mgr.ip_address(), "");
}

TEST_F(nmcli_process_test, scan_lists_networks_and_survives_failure)
{
    wifi_manager mgr;
    std::vector<wifi_network> networks;
    int scans = 0;
    std::string error;
    mgr.set_scan_complete_callback([&](std::vector<wifi_network> const& n) {
        networks = n;
        ++scans;
    });
    mgr.set_error_callback([&](std::string const& e) { error = e; });

    mgr.scan_networks();
    ASSERT_TRUE(wait_for([&] { return scans == 1; }));
    ASSERT_EQ(networks.size(), 3u);
    EXPECT_EQ(networks[0].ssid, "Turkey");
    EXPECT_TRUE(networks[0].is_connected);
    EXPECT_NE(log().find("[--rescan] [yes]"), std::string::npos);

    touch("scan_fails");
    mgr.scan_networks();
    ASSERT_TRUE(wait_for([&] { return scans == 2; }));
    EXPECT_EQ(networks.size(), 3u); // previous list kept
    EXPECT_EQ(error, "WiFi scan failed: Scanning not allowed.");
}

TEST_F(nmcli_process_test, saved_networks_have_ssid_and_security)
{
    wifi_manager mgr;
    std::vector<wifi_saved_network> saved;
    bool loaded = false;
    mgr.set_saved_networks_callback(
        [&](std::vector<wifi_saved_network> const& s) {
            saved = s;
            loaded = true;
        });

    mgr.refresh_saved_networks();
    ASSERT_TRUE(wait_for([&] { return loaded; }));
    ASSERT_EQ(saved.size(), 2u);        // the ethernet profile is left out
    EXPECT_EQ(saved[0].ssid, "Studio"); // higher priority first
    EXPECT_EQ(saved[0].security, wifi_security::wpa3);
    EXPECT_EQ(saved[0].priority, 10);
    EXPECT_EQ(saved[1].ssid, "Turkey");
    EXPECT_EQ(saved[1].uuid, "aaaaaaaa-0000-0000-0000-000000000001");
    EXPECT_EQ(saved[1].security, wifi_security::wpa2);
}

TEST_F(nmcli_process_test, connect_new_network_passes_password_on_stdin_only)
{
    wifi_manager mgr;
    load_saved(mgr);

    std::string const ssid = "Cafe; rm -rf / \"$(x)\":";
    std::string const password = R"(p@ss w0rd"'\$x)";
    auto const o = connect(mgr, ssid, password, true);

    EXPECT_TRUE(o.done);
    EXPECT_TRUE(o.ok) << o.error;

    auto const text = log();
    EXPECT_NE(
        text.find(
            "CALL [connection] [add] [type] [wifi] [ifname] [wlan0] "
            "[con-name] [" +
            ssid + "] [ssid] [" + ssid +
            "] [save] [yes] [802-11-wireless-security.key-mgmt] "
            "[wpa-psk]"),
        std::string::npos)
        << text;
    EXPECT_NE(
        text.find(
            std::string{"STDIN<<set 802-11-wireless-security.psk "} + password +
            "\nsave persistent\nquit>>"),
        std::string::npos)
        << text;
    EXPECT_NE(
        text.find(
            std::string{"CALL [--wait] [60] [connection] [up] "
                        "[uuid] ["} +
            added_uuid + "] [ifname] [wlan0]"),
        std::string::npos)
        << text;

    // The password never appears on a command line.
    std::istringstream lines{text};
    for (std::string line; std::getline(lines, line);)
    {
        if (line.starts_with("CALL"))
        {
            EXPECT_EQ(line.find("p@ss"), std::string::npos) << line;
        }
    }
}

TEST_F(nmcli_process_test, connect_without_remember_keeps_profile_in_memory)
{
    wifi_manager mgr;
    load_saved(mgr);

    auto const o = connect(mgr, "Somewhere", "password123", false);
    EXPECT_TRUE(o.ok) << o.error;

    auto const text = log();
    EXPECT_NE(text.find("[save] [no]"), std::string::npos) << text;
    EXPECT_NE(text.find("save temporary"), std::string::npos) << text;
}

TEST_F(nmcli_process_test, connect_uses_security_from_scan)
{
    wifi_manager mgr;
    bool scanned = false;
    mgr.set_scan_complete_callback(
        [&](std::vector<wifi_network> const&) { scanned = true; });
    mgr.scan_networks();
    ASSERT_TRUE(wait_for([&] { return scanned; }));

    // "Studio" is WPA3-only in the scan; an unsaved copy needs SAE.
    write("connections", "");
    load_saved(mgr);
    auto const o = connect(mgr, "Studio", "password123", true);
    EXPECT_TRUE(o.ok) << o.error;
    EXPECT_NE(
        log().find("[802-11-wireless-security.key-mgmt] [sae]"),
        std::string::npos);

    // An open network gets no security and no password step.
    fs::remove(dir / "calls.log");
    auto const open = connect(mgr, "Cafe", "", true);
    EXPECT_TRUE(open.ok) << open.error;
    EXPECT_EQ(
        log().find("[802-11-wireless-security.key-mgmt]"),
        std::string::npos);
    EXPECT_EQ(log().find("[edit]"), std::string::npos);
}

TEST_F(nmcli_process_test, failed_connect_removes_the_new_profile)
{
    wifi_manager mgr;
    load_saved(mgr);
    touch("up_fails");

    auto const o = connect(mgr, "Somewhere", "wrong-password", true);
    EXPECT_FALSE(o.ok);
    EXPECT_EQ(
        o.error,
        "Connection activation failed: Secrets were required, but not "
        "provided.");
    EXPECT_NE(
        log().find(
            std::string{"CALL [connection] [delete] [uuid] ["} + added_uuid +
            "]"),
        std::string::npos)
        << log();
}

TEST_F(nmcli_process_test, connect_saved_network_activates_it)
{
    wifi_manager mgr;
    load_saved(mgr);

    auto const o = connect(mgr, "Turkey", "", false);
    EXPECT_TRUE(o.ok) << o.error;

    auto const text = log();
    EXPECT_EQ(text.find("[add]"), std::string::npos) << text;
    EXPECT_NE(
        text.find(
            "CALL [--wait] [60] [connection] [up] [uuid] "
            "[aaaaaaaa-0000-0000-0000-000000000001]"),
        std::string::npos)
        << text;
}

TEST_F(nmcli_process_test, failed_connect_keeps_a_saved_profile)
{
    wifi_manager mgr;
    load_saved(mgr);
    touch("up_fails");

    auto const o = connect(mgr, "Turkey", "", false);
    EXPECT_FALSE(o.ok);
    EXPECT_EQ(log().find("[delete]"), std::string::npos) << log();
}

TEST_F(nmcli_process_test, connect_saved_network_with_new_password)
{
    wifi_manager mgr;
    load_saved(mgr);

    auto const o = connect(mgr, "Turkey", "new-password", false);
    EXPECT_TRUE(o.ok) << o.error;

    auto const text = log();
    EXPECT_EQ(text.find("[add]"), std::string::npos) << text;
    EXPECT_NE(
        text.find(
            "CALL [connection] [edit] [uuid] "
            "[aaaaaaaa-0000-0000-0000-000000000001]"),
        std::string::npos)
        << text;
    EXPECT_NE(text.find("save persistent"), std::string::npos) << text;
}

TEST_F(nmcli_process_test, rejects_passwords_the_editor_would_change)
{
    wifi_manager mgr;
    load_saved(mgr);

    for (std::string const password : std::vector<std::string>{
             " leading",
             "trailing ",
             "new\nline",
             "tab\tinside"})
    {
        auto const o = connect(mgr, "Somewhere", password, true);
        EXPECT_TRUE(o.done);
        EXPECT_FALSE(o.ok) << password;
    }
    EXPECT_EQ(calls(), 0) << log();
}

TEST_F(nmcli_process_test, forget_deletes_every_profile_for_the_ssid)
{
    write(
        "connections",
        "netplan-wlan0-Turkey:aaaaaaaa-0000-0000-0000-000000000001:"
        "802-11-wireless:yes:0\n"
        "Turkey:aaaaaaaa-0000-0000-0000-000000000004:"
        "802-11-wireless:yes:0\n");
    write(
        "profile-aaaaaaaa-0000-0000-0000-000000000004",
        "Turkey\nwpa-psk\nno\n");

    wifi_manager mgr;
    load_saved(mgr);

    bool done = false;
    EXPECT_TRUE(mgr.forget_network("Turkey", [&](bool ok, std::string const&) {
        done = ok;
    }));
    ASSERT_TRUE(wait_for([&] { return done; }));
    EXPECT_NE(
        log().find(
            "CALL [connection] [delete] "
            "[uuid] [aaaaaaaa-0000-0000-0000-000000000001] "
            "[uuid] [aaaaaaaa-0000-0000-0000-000000000004]"),
        std::string::npos)
        << log();

    EXPECT_FALSE(mgr.forget_network("Unknown"));
}

TEST_F(nmcli_process_test, disconnect_disconnects_the_device)
{
    wifi_manager mgr;
    bool done = false;
    mgr.disconnect([&](bool ok, std::string const&) { done = ok; });
    ASSERT_TRUE(wait_for([&] { return done; }));
    EXPECT_NE(
        log().find("CALL [device] [disconnect] [wlan0]"),
        std::string::npos);
}

TEST_F(nmcli_process_test, network_controller_follows_the_radio)
{
    network_controller ctrl;
    std::vector<bool> states;
    std::string error;
    ctrl.set_state_change_callback([&](bool on) { states.push_back(on); });
    ctrl.set_error_callback([&](std::string const& e) { error = e; });

    ctrl.refresh();
    ASSERT_TRUE(wait_for([&] { return states.size() == 1; }));
    EXPECT_TRUE(states.back());
    EXPECT_TRUE(ctrl.is_enabled());

    ctrl.disable();
    ASSERT_TRUE(wait_for([&] { return states.size() == 2; }));
    EXPECT_FALSE(states.back());
    EXPECT_NE(log().find("CALL [radio] [wifi] [off]"), std::string::npos);

    ctrl.enable();
    ASSERT_TRUE(wait_for([&] { return states.size() == 3; }));
    EXPECT_TRUE(states.back());

    // A refused change is reported and the state is re-read.
    touch("radio_fails");
    ctrl.disable();
    ASSERT_TRUE(wait_for([&] { return !error.empty(); }));
    EXPECT_EQ(error, "Couldn't turn WiFi off: Not authorized.");
    wait_for([] { return false; }, 300);
    EXPECT_TRUE(ctrl.is_enabled());
    EXPECT_EQ(states.size(), 3u);
}

} // namespace piejam::network_manager::test
