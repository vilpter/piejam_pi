// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

namespace piejam::network_manager
{

/// Security type for WiFi networks
enum class wifi_security
{
    open,       ///< No security
    wep,        ///< WEP (legacy, insecure)
    wpa,        ///< WPA
    wpa2,       ///< WPA2
    wpa3,       ///< WPA3
    unknown     ///< Unknown security type
};

/// Frequency band for WiFi networks
enum class wifi_band
{
    band_2_4_ghz,   ///< 2.4 GHz band
    band_5_ghz,     ///< 5 GHz band
    band_6_ghz,     ///< 6 GHz band (WiFi 6E)
    unknown
};

/// Information about a detected WiFi network
struct wifi_network
{
    std::string ssid;           ///< Network name
    std::string bssid;          ///< MAC address of access point
    int signal_strength{0};     ///< Signal strength in dBm (typically -30 to -90)
    int signal_percent{0};      ///< Signal strength as percentage (0-100)
    wifi_security security{wifi_security::unknown};
    wifi_band band{wifi_band::unknown};
    int frequency_mhz{0};       ///< Frequency in MHz (e.g., 2412, 5180)
    bool is_connected{false};   ///< Currently connected to this network
};

/// Configuration for a saved WiFi network
struct wifi_saved_network
{
    std::string ssid;           ///< Network name
    int priority{0};            ///< Priority for auto-connect (higher = preferred)
    bool auto_connect{true};    ///< Automatically connect when in range
    bool hidden{false};         ///< Hidden network (not broadcasting SSID)
    wifi_security security{wifi_security::unknown};
    // Note: Password is stored in wpa_supplicant.conf, not here
};

/// Convert signal strength in dBm to percentage
inline int signal_dbm_to_percent(int dbm)
{
    // Typical range: -30 dBm (excellent) to -90 dBm (very weak)
    if (dbm >= -30)
        return 100;
    if (dbm <= -90)
        return 0;
    return static_cast<int>((dbm + 90) * 100 / 60);
}

/// Convert frequency to band
inline wifi_band frequency_to_band(int freq_mhz)
{
    if (freq_mhz >= 2400 && freq_mhz < 2500)
        return wifi_band::band_2_4_ghz;
    if (freq_mhz >= 5150 && freq_mhz < 5900)
        return wifi_band::band_5_ghz;
    if (freq_mhz >= 5925 && freq_mhz < 7125)
        return wifi_band::band_6_ghz;
    return wifi_band::unknown;
}

/// Get human-readable name for security type
inline char const* security_to_string(wifi_security sec)
{
    switch (sec)
    {
    case wifi_security::open:
        return "Open";
    case wifi_security::wep:
        return "WEP";
    case wifi_security::wpa:
        return "WPA";
    case wifi_security::wpa2:
        return "WPA2";
    case wifi_security::wpa3:
        return "WPA3";
    default:
        return "Unknown";
    }
}

/// Get human-readable name for frequency band
inline char const* band_to_string(wifi_band band)
{
    switch (band)
    {
    case wifi_band::band_2_4_ghz:
        return "2.4 GHz";
    case wifi_band::band_5_ghz:
        return "5 GHz";
    case wifi_band::band_6_ghz:
        return "6 GHz";
    default:
        return "Unknown";
    }
}

} // namespace piejam::network_manager
