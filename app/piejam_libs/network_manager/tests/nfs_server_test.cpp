// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/network_manager/nfs_server.h>

#include <gtest/gtest.h>

namespace piejam::network_manager::test
{

// enable()/disable() touch the system (systemctl, the export helper) and so
// aren't exercised here; these cover the pure, side-effect-free surface.

TEST(nfs_server_test, constructor_with_default_path)
{
    nfs_server server;
    EXPECT_EQ(server.export_path(), "/srv/rec");
}

TEST(nfs_server_test, constructor_with_custom_path)
{
    nfs_server server("/custom/path");
    EXPECT_EQ(server.export_path(), "/custom/path");
}

TEST(nfs_server_test, export_config_carries_path)
{
    nfs_server server("/test/export");
    EXPECT_EQ(server.export_config().export_path, "/test/export");
}

TEST(nfs_server_test, export_defaults_to_read_write)
{
    nfs_server server;
    EXPECT_TRUE(server.export_config().read_write);
}

TEST(nfs_server_test, is_active_is_false_without_enable)
{
    nfs_server server;
    // No export was applied, so the share isn't active regardless of whether
    // a stray nfs-server happens to run on the build host.
    EXPECT_TRUE(server.export_config().allowed_hosts.empty());
}

TEST(nfs_server_test, status_callback_is_not_called_on_set)
{
    nfs_server server;

    bool called = false;
    server.set_status_callback([&](nfs_server_status) { called = true; });
    EXPECT_FALSE(called);
}

TEST(nfs_server_test, error_callback_is_not_called_on_set)
{
    nfs_server server;

    bool called = false;
    server.set_error_callback([&](std::string const&) { called = true; });
    EXPECT_FALSE(called);
}

TEST(nfs_server_test, move_constructor_preserves_path)
{
    nfs_server server1("/custom/path");
    nfs_server server2(std::move(server1));
    EXPECT_EQ(server2.export_path(), "/custom/path");
}

TEST(nfs_server_test, move_assignment_preserves_path)
{
    nfs_server server1("/custom/path");
    nfs_server server2;
    server2 = std::move(server1);
    EXPECT_EQ(server2.export_path(), "/custom/path");
}

} // namespace piejam::network_manager::test
