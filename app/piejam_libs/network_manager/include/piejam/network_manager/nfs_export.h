// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace piejam::network_manager
{

/// NFS server export configuration.
///
/// The appliance exports one directory (its recordings) and nothing else, so
/// this holds only what the UI shows. The export is applied with `exportfs`
/// and never written to /etc/exports (see nfs_server).
struct nfs_export_config
{
    std::string export_path;   ///< Path exported (e.g. "/home/piejam/recordings")
    std::string allowed_hosts; ///< Subnet allowed to mount (e.g. "192.168.30.0/24")
    bool read_write{true};     ///< Export read-write (false = read-only)
};

/// Parse a dotted-quad IPv4 string into four octets, rejecting anything that
/// isn't exactly four decimal octets in 0..255.
[[nodiscard]] inline auto
parse_ipv4(std::string const& ip) noexcept
    -> std::optional<std::array<unsigned, 4>>
{
    std::array<unsigned, 4> octets{};
    std::size_t part = 0;
    std::size_t digits = 0;
    unsigned value = 0;

    for (char const c : ip)
    {
        if (c == '.')
        {
            if (digits == 0 || part >= 3)
                return std::nullopt;
            octets[part++] = value;
            value = 0;
            digits = 0;
        }
        else if (c >= '0' && c <= '9')
        {
            if (++digits > 3)
                return std::nullopt;
            value = value * 10 + static_cast<unsigned>(c - '0');
            if (value > 255)
                return std::nullopt;
        }
        else
        {
            return std::nullopt;
        }
    }

    if (digits == 0 || part != 3)
        return std::nullopt;
    octets[3] = value;

    return octets;
}

/// True for an RFC 1918 private IPv4 address (10/8, 172.16/12, 192.168/16).
/// Loopback and link-local are not private in this sense.
[[nodiscard]] inline auto
is_private_ipv4(std::array<unsigned, 4> const& o) noexcept -> bool
{
    return o[0] == 10 || (o[0] == 172 && o[1] >= 16 && o[1] <= 31) ||
           (o[0] == 192 && o[1] == 168);
}

/// Compute the network address in CIDR form ("a.b.c.d/pp") for an IPv4
/// address and prefix length. Returns nullopt for a malformed address or a
/// prefix outside 0..32. The host bits are masked off, so 192.168.30.234/24
/// yields "192.168.30.0/24" -- the whole subnet the address sits on.
[[nodiscard]] inline auto
ipv4_network_cidr(std::string const& ip, int prefix)
    -> std::optional<std::string>
{
    if (prefix < 0 || prefix > 32)
        return std::nullopt;

    auto const octets = parse_ipv4(ip);
    if (!octets)
        return std::nullopt;

    std::uint32_t const addr = (static_cast<std::uint32_t>((*octets)[0]) << 24) |
                               (static_cast<std::uint32_t>((*octets)[1]) << 16) |
                               (static_cast<std::uint32_t>((*octets)[2]) << 8) |
                               static_cast<std::uint32_t>((*octets)[3]);
    std::uint32_t const mask =
        prefix == 0 ? 0u : (0xFFFFFFFFu << (32 - prefix));
    std::uint32_t const net = addr & mask;

    return std::to_string((net >> 24) & 0xFFu) + "." +
           std::to_string((net >> 16) & 0xFFu) + "." +
           std::to_string((net >> 8) & 0xFFu) + "." +
           std::to_string(net & 0xFFu) + "/" + std::to_string(prefix);
}

} // namespace piejam::network_manager
