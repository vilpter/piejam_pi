// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/network_manager/nfs_export.h>

#include <gtest/gtest.h>

namespace piejam::network_manager::test
{

TEST(nfs_export_test, parse_ipv4_accepts_dotted_quad)
{
    auto const o = parse_ipv4("192.168.30.234");
    ASSERT_TRUE(o.has_value());
    EXPECT_EQ((*o)[0], 192u);
    EXPECT_EQ((*o)[1], 168u);
    EXPECT_EQ((*o)[2], 30u);
    EXPECT_EQ((*o)[3], 234u);
}

TEST(nfs_export_test, parse_ipv4_accepts_bounds)
{
    EXPECT_TRUE(parse_ipv4("0.0.0.0").has_value());
    EXPECT_TRUE(parse_ipv4("255.255.255.255").has_value());
}

TEST(nfs_export_test, parse_ipv4_rejects_malformed)
{
    EXPECT_FALSE(parse_ipv4("").has_value());
    EXPECT_FALSE(parse_ipv4("192.168.30").has_value());
    EXPECT_FALSE(parse_ipv4("192.168.30.1.2").has_value());
    EXPECT_FALSE(parse_ipv4("192.168.30.").has_value());
    EXPECT_FALSE(parse_ipv4(".168.30.1").has_value());
    EXPECT_FALSE(parse_ipv4("192.168.30.256").has_value());
    EXPECT_FALSE(parse_ipv4("192.168.030.1111").has_value());
    EXPECT_FALSE(parse_ipv4("192.168.30.a").has_value());
    EXPECT_FALSE(parse_ipv4("192.168.30.1 ").has_value());
    EXPECT_FALSE(parse_ipv4("192:168:30:1").has_value());
}

TEST(nfs_export_test, is_private_ipv4)
{
    EXPECT_TRUE(is_private_ipv4(*parse_ipv4("10.0.0.1")));
    EXPECT_TRUE(is_private_ipv4(*parse_ipv4("172.16.5.4")));
    EXPECT_TRUE(is_private_ipv4(*parse_ipv4("172.31.255.1")));
    EXPECT_TRUE(is_private_ipv4(*parse_ipv4("192.168.30.234")));

    EXPECT_FALSE(is_private_ipv4(*parse_ipv4("172.15.0.1")));
    EXPECT_FALSE(is_private_ipv4(*parse_ipv4("172.32.0.1")));
    EXPECT_FALSE(is_private_ipv4(*parse_ipv4("8.8.8.8")));
    EXPECT_FALSE(is_private_ipv4(*parse_ipv4("127.0.0.1")));
    EXPECT_FALSE(is_private_ipv4(*parse_ipv4("169.254.1.1")));
}

TEST(nfs_export_test, network_cidr_masks_host_bits)
{
    EXPECT_EQ(ipv4_network_cidr("192.168.30.234", 24), "192.168.30.0/24");
    EXPECT_EQ(ipv4_network_cidr("192.168.30.234", 16), "192.168.0.0/16");
    EXPECT_EQ(ipv4_network_cidr("10.1.2.3", 8), "10.0.0.0/8");
    EXPECT_EQ(ipv4_network_cidr("172.16.5.9", 20), "172.16.0.0/20");
}

TEST(nfs_export_test, network_cidr_prefix_extremes)
{
    EXPECT_EQ(ipv4_network_cidr("192.168.30.234", 32), "192.168.30.234/32");
    EXPECT_EQ(ipv4_network_cidr("192.168.30.234", 0), "0.0.0.0/0");
}

TEST(nfs_export_test, network_cidr_rejects_bad_input)
{
    EXPECT_FALSE(ipv4_network_cidr("192.168.30.234", -1).has_value());
    EXPECT_FALSE(ipv4_network_cidr("192.168.30.234", 33).has_value());
    EXPECT_FALSE(ipv4_network_cidr("not-an-ip", 24).has_value());
}

} // namespace piejam::network_manager::test
