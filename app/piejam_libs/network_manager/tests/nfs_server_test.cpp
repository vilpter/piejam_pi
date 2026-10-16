// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/network_manager/nfs_server.h>

#include <gtest/gtest.h>

namespace piejam::network_manager::test
{

class nfs_server_test : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Tests run in isolation - actual NFS operations will fail
        // without proper system setup, so we mainly test the logic
    }
};

TEST_F(nfs_server_test, constructor_with_default_path)
{
    nfs_server server;

    EXPECT_EQ(server.export_path(), "/home/piejam/recordings");
}

TEST_F(nfs_server_test, constructor_with_custom_path)
{
    nfs_server server("/custom/path");

    EXPECT_EQ(server.export_path(), "/custom/path");
}

TEST_F(nfs_server_test, initial_status_is_inactive)
{
    nfs_server server;

    // Without NFS server packages installed, status should be inactive or error
    auto status = server.status();
    EXPECT_TRUE(
        status == nfs_server_status::inactive ||
        status == nfs_server_status::error);
}

TEST_F(nfs_server_test, is_active_returns_false_initially)
{
    nfs_server server;

    // Server won't be active without explicit enable call
    EXPECT_FALSE(server.is_active());
}

TEST_F(nfs_server_test, export_config_has_correct_path)
{
    nfs_server server("/test/export");

    auto const& config = server.export_config();
    EXPECT_EQ(config.export_path, "/test/export");
}

TEST_F(nfs_server_test, export_config_default_values)
{
    nfs_server server;

    auto const& config = server.export_config();
    EXPECT_TRUE(config.read_write);
    EXPECT_TRUE(config.sync);
    EXPECT_TRUE(config.no_subtree_check);
    EXPECT_FALSE(config.no_root_squash);
}

TEST_F(nfs_server_test, set_read_write_updates_config)
{
    nfs_server server;

    server.set_read_write(false);
    EXPECT_FALSE(server.export_config().read_write);

    server.set_read_write(true);
    EXPECT_TRUE(server.export_config().read_write);
}

TEST_F(nfs_server_test, mount_command_contains_export_path)
{
    nfs_server server("/home/piejam/recordings");

    auto cmd = server.mount_command();

    EXPECT_NE(cmd.find("/home/piejam/recordings"), std::string::npos);
}

TEST_F(nfs_server_test, mount_command_contains_mount_syntax)
{
    nfs_server server;

    auto cmd = server.mount_command();

    EXPECT_NE(cmd.find("mount"), std::string::npos);
    EXPECT_NE(cmd.find("-t nfs"), std::string::npos);
}

TEST_F(nfs_server_test, set_allowed_hosts)
{
    nfs_server server;

    server.set_allowed_hosts("192.168.1.0/24");
    EXPECT_EQ(server.export_config().allowed_hosts, "192.168.1.0/24");

    server.set_allowed_hosts("*");
    EXPECT_EQ(server.export_config().allowed_hosts, "*");
}

TEST_F(nfs_server_test, status_callback_can_be_set)
{
    nfs_server server;

    bool callback_called = false;
    server.set_status_callback([&](nfs_server_status) {
        callback_called = true;
    });

    // Callback is set but won't be called until status changes
    EXPECT_FALSE(callback_called);
}

TEST_F(nfs_server_test, error_callback_can_be_set)
{
    nfs_server server;

    bool callback_called = false;
    server.set_error_callback([&](std::string const&) {
        callback_called = true;
    });

    // Callback is set but won't be called until an error occurs
    EXPECT_FALSE(callback_called);
}

TEST_F(nfs_server_test, move_constructor)
{
    nfs_server server1("/custom/path");
    server1.set_read_write(false);

    nfs_server server2(std::move(server1));

    EXPECT_EQ(server2.export_path(), "/custom/path");
    EXPECT_FALSE(server2.export_config().read_write);
}

TEST_F(nfs_server_test, move_assignment)
{
    nfs_server server1("/custom/path");
    server1.set_read_write(false);

    nfs_server server2;
    server2 = std::move(server1);

    EXPECT_EQ(server2.export_path(), "/custom/path");
    EXPECT_FALSE(server2.export_config().read_write);
}

// Note: The following tests would require root access or mock system calls
// to properly test. They are here as placeholders documenting expected behavior.

// TEST_F(nfs_server_test, enable_starts_server)
// {
//     nfs_server server;
//     // Requires root privileges and nfs-kernel-server installed
//     // bool result = server.enable();
//     // EXPECT_TRUE(result);
//     // EXPECT_TRUE(server.is_active());
// }

// TEST_F(nfs_server_test, disable_stops_server)
// {
//     nfs_server server;
//     // Requires root privileges and nfs-kernel-server installed
//     // server.enable();
//     // bool result = server.disable();
//     // EXPECT_TRUE(result);
//     // EXPECT_FALSE(server.is_active());
// }

// TEST_F(nfs_server_test, test_connection_returns_true_when_active)
// {
//     nfs_server server;
//     // Requires server to be running
//     // server.enable();
//     // bool result = server.test_connection();
//     // EXPECT_TRUE(result);
// }

} // namespace piejam::network_manager::test
