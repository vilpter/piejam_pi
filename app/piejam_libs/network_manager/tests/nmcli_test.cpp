// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/network_manager/nmcli.h>

#include <gtest/gtest.h>

// Parsers for nmcli's terse output. The samples are real output from
// NetworkManager 1.52 on Raspberry Pi OS (trixie).

namespace piejam::network_manager::nmcli::test
{

TEST(nmcli_split_fields, unescapes_colons_and_backslashes)
{
    auto const fields = split_fields(
        R"(*:Turkey:E8\:9F\:80\:5C\:17\:77:2412 MHz:63:WPA2 WPA3)");
    ASSERT_EQ(fields.size(), 6u);
    EXPECT_EQ(fields[0], "*");
    EXPECT_EQ(fields[1], "Turkey");
    EXPECT_EQ(fields[2], "E8:9F:80:5C:17:77");
    EXPECT_EQ(fields[5], "WPA2 WPA3");

    auto const tricky = split_fields(R"(My\:Net\\work::x)");
    ASSERT_EQ(tricky.size(), 3u);
    EXPECT_EQ(tricky[0], R"(My:Net\work)");
    EXPECT_EQ(tricky[1], "");
    EXPECT_EQ(tricky[2], "x");
}

TEST(nmcli_unescape, single_value)
{
    EXPECT_EQ(unescape(R"(PieJam Test\:1)"), "PieJam Test:1");
    EXPECT_EQ(unescape(R"(a\\b)"), R"(a\b)");
    EXPECT_EQ(unescape("plain"), "plain");
}

TEST(nmcli_parse_device_status, connected_with_access_points)
{
    auto const st = parse_device_status(
        "GENERAL.STATE:100 (connected)\n"
        "GENERAL.CONNECTION:netplan-wlan0-Turkey\n"
        "IP4.ADDRESS[1]:192.168.30.234/24\n"
        "AP[1].IN-USE: \n"
        "AP[1].BSSID:EA:9F:80:5C:17:77\n"
        "AP[1].SSID:Turkey-Guest\n"
        "AP[1].SIGNAL:72\n"
        "AP[1].SECURITY:WPA2 WPA3\n"
        "AP[4].IN-USE:*\n"
        "AP[4].BSSID:E8:9F:80:5C:17:77\n"
        "AP[4].SSID:Turkey\n"
        "AP[4].SIGNAL:63\n"
        "AP[4].SECURITY:WPA2 WPA3\n");

    EXPECT_EQ(st.state, device_state_activated);
    EXPECT_EQ(st.connection, "netplan-wlan0-Turkey");
    EXPECT_EQ(st.ip_address, "192.168.30.234");
    EXPECT_EQ(st.active_ssid, "Turkey");
    EXPECT_EQ(st.active_signal, 63);
}

TEST(nmcli_parse_device_status, disconnected)
{
    auto const st = parse_device_status(
        "GENERAL.STATE:30 (disconnected)\n"
        "GENERAL.CONNECTION:--\n"
        "AP[1].IN-USE: \n"
        "AP[1].SSID:Turkey\n"
        "AP[1].SIGNAL:63\n");

    EXPECT_EQ(st.state, device_state_disconnected);
    EXPECT_EQ(st.connection, "");
    EXPECT_EQ(st.ip_address, "");
    EXPECT_EQ(st.active_ssid, "");
}

TEST(nmcli_parse_device_status, first_ipv4_address_only)
{
    auto const st = parse_device_status(
        "GENERAL.STATE:100 (connected)\n"
        "IP4.ADDRESS[1]:10.0.0.5/8\n"
        "IP4.ADDRESS[2]:192.168.1.5/24\n");
    EXPECT_EQ(st.ip_address, "10.0.0.5");
}

TEST(nmcli_parse_device_status, empty_output)
{
    auto const st = parse_device_status("");
    EXPECT_EQ(st.state, 0);
    EXPECT_EQ(st.active_ssid, "");
}

TEST(nmcli_parse_wifi_list, dedupes_by_ssid_and_sorts_by_signal)
{
    auto const networks = parse_wifi_list(
        " :Turkey-Guest:EA\\:9F\\:80\\:5C\\:17\\:77:2412 MHz:72:WPA2 WPA3\n"
        " ::EE\\:9F\\:80\\:5C\\:17\\:77:2412 MHz:72:WPA2 WPA3\n"
        "*:Turkey:E8\\:9F\\:80\\:5C\\:17\\:77:2412 MHz:63:WPA2 WPA3\n"
        " :Turkey:E8\\:9F\\:80\\:5C\\:17\\:78:5745 MHz:70:WPA2 WPA3\n"
        " :Cafe:00\\:11\\:22\\:33\\:44\\:55:5180 MHz:20:\n");

    ASSERT_EQ(networks.size(), 3u); // the hidden network is skipped

    EXPECT_EQ(networks[0].ssid, "Turkey-Guest");
    EXPECT_EQ(networks[0].signal_percent, 72);

    // The access point in use wins over a stronger one with the same SSID.
    EXPECT_EQ(networks[1].ssid, "Turkey");
    EXPECT_TRUE(networks[1].is_connected);
    EXPECT_EQ(networks[1].bssid, "E8:9F:80:5C:17:77");
    EXPECT_EQ(networks[1].frequency_mhz, 2412);
    EXPECT_EQ(networks[1].band, wifi_band::band_2_4_ghz);
    EXPECT_EQ(networks[1].security, wifi_security::wpa2);

    EXPECT_EQ(networks[2].ssid, "Cafe");
    EXPECT_EQ(networks[2].security, wifi_security::open);
    EXPECT_EQ(networks[2].band, wifi_band::band_5_ghz);
    EXPECT_FALSE(networks[2].is_connected);
}

TEST(nmcli_parse_wifi_list, ignores_malformed_lines)
{
    EXPECT_TRUE(parse_wifi_list("garbage\n\n:::\n").empty());
}

TEST(nmcli_parse_security, maps_nmcli_security_column)
{
    EXPECT_EQ(parse_security("WPA2 WPA3"), wifi_security::wpa2);
    EXPECT_EQ(parse_security("WPA3"), wifi_security::wpa3);
    EXPECT_EQ(parse_security("WPA1 WPA2"), wifi_security::wpa2);
    EXPECT_EQ(parse_security("WPA1"), wifi_security::wpa);
    EXPECT_EQ(parse_security("WEP"), wifi_security::wep);
    EXPECT_EQ(parse_security(""), wifi_security::open);
    EXPECT_EQ(parse_security("--"), wifi_security::open);
    EXPECT_EQ(parse_security("OWE"), wifi_security::open);
    EXPECT_EQ(parse_security("WPA2 802.1X"), wifi_security::unknown);
}

TEST(nmcli_parse_connections, real_output)
{
    auto const connections = parse_connections(
        "netplan-wlan0-Turkey:3c6cef8b-0d2f-30eb-a1b0-7169339bdf06:"
        "802-11-wireless:yes:0\n"
        "lo:598d5ee7-8389-41bb-9607-9c60c7b065fc:loopback:no:0\n"
        "PieJam Test\\:1:9d4e75e1-513f-41ea-8070-2bb4dbde50b7:"
        "802-11-wireless:no:5\n");

    ASSERT_EQ(connections.size(), 3u);
    EXPECT_EQ(connections[0].name, "netplan-wlan0-Turkey");
    EXPECT_EQ(connections[0].uuid, "3c6cef8b-0d2f-30eb-a1b0-7169339bdf06");
    EXPECT_EQ(connections[0].type, wifi_connection_type);
    EXPECT_TRUE(connections[0].autoconnect);
    EXPECT_EQ(connections[1].type, "loopback");
    EXPECT_EQ(connections[2].name, "PieJam Test:1");
    EXPECT_FALSE(connections[2].autoconnect);
    EXPECT_EQ(connections[2].priority, 5);
}

TEST(nmcli_key_mgmt, round_trip)
{
    EXPECT_EQ(key_mgmt_to_security(""), wifi_security::open);
    EXPECT_EQ(key_mgmt_to_security("none"), wifi_security::wep);
    EXPECT_EQ(key_mgmt_to_security("wpa-psk"), wifi_security::wpa2);
    EXPECT_EQ(key_mgmt_to_security("sae"), wifi_security::wpa3);
    EXPECT_EQ(key_mgmt_to_security("wpa-eap"), wifi_security::unknown);

    EXPECT_EQ(security_to_key_mgmt(wifi_security::open), "");
    EXPECT_EQ(security_to_key_mgmt(wifi_security::wep), "none");
    EXPECT_EQ(security_to_key_mgmt(wifi_security::wpa), "wpa-psk");
    EXPECT_EQ(security_to_key_mgmt(wifi_security::wpa2), "wpa-psk");
    EXPECT_EQ(security_to_key_mgmt(wifi_security::wpa3), "sae");
}

TEST(nmcli_parse_added_uuid, from_connection_add_output)
{
    EXPECT_EQ(
        parse_added_uuid(
            "Connection 'PieJam Test:1' "
            "(9d4e75e1-513f-41ea-8070-2bb4dbde50b7) "
            "successfully added.\n"),
        "9d4e75e1-513f-41ea-8070-2bb4dbde50b7");
    EXPECT_EQ(
        parse_added_uuid(
            "Connection 'a (b)' "
            "(9d4e75e1-513f-41ea-8070-2bb4dbde50b7) "
            "successfully added.\n"),
        "9d4e75e1-513f-41ea-8070-2bb4dbde50b7");
    EXPECT_EQ(parse_added_uuid("Error: something\n"), "");
}

TEST(nmcli_error_message, strips_prefix_and_keeps_first_line)
{
    EXPECT_EQ(
        error_message(
            "Error: Connection activation failed: Secrets were "
            "required, but not provided.\nHint: ...\n"),
        "Connection activation failed: Secrets were required, but not "
        "provided.");
    EXPECT_EQ(error_message(""), "");
}

} // namespace piejam::network_manager::nmcli::test
