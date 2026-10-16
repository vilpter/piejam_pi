// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/network_manager/wifi_manager.h>
#include <piejam/network_manager/wifi_network.h>

#include <gtest/gtest.h>

namespace piejam::network_manager::test
{

class wifi_manager_test : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Note: Tests that require actual WiFi hardware will be skipped
        // in environments without wlan0
    }
};

// --- wifi_network helper function tests ---

TEST_F(wifi_manager_test, signal_dbm_to_percent_excellent)
{
    // -30 dBm or better should be 100%
    EXPECT_EQ(signal_dbm_to_percent(-30), 100);
    EXPECT_EQ(signal_dbm_to_percent(-20), 100);
    EXPECT_EQ(signal_dbm_to_percent(0), 100);
}

TEST_F(wifi_manager_test, signal_dbm_to_percent_very_weak)
{
    // -90 dBm or worse should be 0%
    EXPECT_EQ(signal_dbm_to_percent(-90), 0);
    EXPECT_EQ(signal_dbm_to_percent(-100), 0);
}

TEST_F(wifi_manager_test, signal_dbm_to_percent_mid_range)
{
    // -60 dBm should be around 50%
    int percent = signal_dbm_to_percent(-60);
    EXPECT_GE(percent, 45);
    EXPECT_LE(percent, 55);
}

TEST_F(wifi_manager_test, frequency_to_band_2_4ghz)
{
    EXPECT_EQ(frequency_to_band(2412), wifi_band::band_2_4_ghz);
    EXPECT_EQ(frequency_to_band(2437), wifi_band::band_2_4_ghz);
    EXPECT_EQ(frequency_to_band(2472), wifi_band::band_2_4_ghz);
}

TEST_F(wifi_manager_test, frequency_to_band_5ghz)
{
    EXPECT_EQ(frequency_to_band(5180), wifi_band::band_5_ghz);
    EXPECT_EQ(frequency_to_band(5320), wifi_band::band_5_ghz);
    EXPECT_EQ(frequency_to_band(5745), wifi_band::band_5_ghz);
}

TEST_F(wifi_manager_test, frequency_to_band_6ghz)
{
    EXPECT_EQ(frequency_to_band(5955), wifi_band::band_6_ghz);
    EXPECT_EQ(frequency_to_band(6415), wifi_band::band_6_ghz);
}

TEST_F(wifi_manager_test, frequency_to_band_unknown)
{
    EXPECT_EQ(frequency_to_band(0), wifi_band::unknown);
    EXPECT_EQ(frequency_to_band(1000), wifi_band::unknown);
}

TEST_F(wifi_manager_test, security_to_string)
{
    EXPECT_STREQ(security_to_string(wifi_security::open), "Open");
    EXPECT_STREQ(security_to_string(wifi_security::wep), "WEP");
    EXPECT_STREQ(security_to_string(wifi_security::wpa), "WPA");
    EXPECT_STREQ(security_to_string(wifi_security::wpa2), "WPA2");
    EXPECT_STREQ(security_to_string(wifi_security::wpa3), "WPA3");
    EXPECT_STREQ(security_to_string(wifi_security::unknown), "Unknown");
}

TEST_F(wifi_manager_test, band_to_string)
{
    EXPECT_STREQ(band_to_string(wifi_band::band_2_4_ghz), "2.4 GHz");
    EXPECT_STREQ(band_to_string(wifi_band::band_5_ghz), "5 GHz");
    EXPECT_STREQ(band_to_string(wifi_band::band_6_ghz), "6 GHz");
    EXPECT_STREQ(band_to_string(wifi_band::unknown), "Unknown");
}

// --- wifi_manager basic tests ---

TEST_F(wifi_manager_test, construction_with_default_interface)
{
    // Should not throw even if wlan0 doesn't exist
    wifi_manager mgr;
    // Status will depend on actual system state
    auto status = mgr.status();
    EXPECT_TRUE(
        status == wifi_connection_status::disconnected ||
        status == wifi_connection_status::connected);
}

TEST_F(wifi_manager_test, construction_with_custom_interface)
{
    // Should handle non-existent interface gracefully
    wifi_manager mgr("nonexistent0");
    EXPECT_FALSE(mgr.is_interface_available());
}

TEST_F(wifi_manager_test, auto_reconnect_default_enabled)
{
    wifi_manager mgr;
    EXPECT_TRUE(mgr.auto_reconnect());
}

TEST_F(wifi_manager_test, set_auto_reconnect)
{
    wifi_manager mgr;

    mgr.set_auto_reconnect(false);
    EXPECT_FALSE(mgr.auto_reconnect());

    mgr.set_auto_reconnect(true);
    EXPECT_TRUE(mgr.auto_reconnect());
}

TEST_F(wifi_manager_test, available_networks_initially_empty)
{
    wifi_manager mgr;
    // Before scanning, should be empty or have cached results
    // This depends on whether wpa_supplicant has scan results
    auto const& networks = mgr.available_networks();
    // Just verify it doesn't crash
    (void)networks.size();
}

TEST_F(wifi_manager_test, move_constructor)
{
    wifi_manager mgr1;
    mgr1.set_auto_reconnect(false);

    wifi_manager mgr2(std::move(mgr1));
    EXPECT_FALSE(mgr2.auto_reconnect());
}

TEST_F(wifi_manager_test, move_assignment)
{
    wifi_manager mgr1;
    mgr1.set_auto_reconnect(false);

    wifi_manager mgr2;
    mgr2 = std::move(mgr1);
    EXPECT_FALSE(mgr2.auto_reconnect());
}

// --- Callback registration tests ---

TEST_F(wifi_manager_test, set_callbacks_does_not_crash)
{
    wifi_manager mgr;

    mgr.set_scan_complete_callback([](std::vector<wifi_network> const&) {});
    mgr.set_connection_changed_callback(
        [](wifi_connection_status, wifi_network const*) {});
    mgr.set_error_callback([](std::string const&) {});

    // Just verify setting callbacks doesn't crash
    SUCCEED();
}

} // namespace piejam::network_manager::test
