// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/network_manager/nfs_mount.h>

#include <gtest/gtest.h>

namespace piejam::network_manager::test
{

TEST(nfs_mount_test, nfs_version_to_string_v3)
{
    EXPECT_STREQ(nfs_version_to_string(nfs_version::v3), "NFSv3");
}

TEST(nfs_mount_test, nfs_version_to_string_v4)
{
    EXPECT_STREQ(nfs_version_to_string(nfs_version::v4), "NFSv4");
}

TEST(nfs_mount_test, nfs_mount_status_to_string)
{
    EXPECT_STREQ(nfs_mount_status_to_string(nfs_mount_status::unmounted), "Unmounted");
    EXPECT_STREQ(nfs_mount_status_to_string(nfs_mount_status::mounting), "Mounting...");
    EXPECT_STREQ(nfs_mount_status_to_string(nfs_mount_status::mounted), "Mounted");
    EXPECT_STREQ(nfs_mount_status_to_string(nfs_mount_status::unmounting), "Unmounting...");
    EXPECT_STREQ(nfs_mount_status_to_string(nfs_mount_status::error), "Error");
}

TEST(nfs_mount_test, mount_config_default_values)
{
    nfs_mount_config config;

    EXPECT_TRUE(config.id.empty());
    EXPECT_TRUE(config.name.empty());
    EXPECT_TRUE(config.server_host.empty());
    EXPECT_TRUE(config.remote_path.empty());
    EXPECT_TRUE(config.local_mount.empty());
    EXPECT_EQ(config.version, nfs_version::v4);
    EXPECT_TRUE(config.read_write);
    EXPECT_FALSE(config.auto_mount);
    EXPECT_EQ(config.timeout_decisecs, 30);
    EXPECT_EQ(config.retrans, 3);
}

TEST(nfs_mount_test, mount_state_default_values)
{
    nfs_mount_state state;

    EXPECT_TRUE(state.id.empty());
    EXPECT_EQ(state.status, nfs_mount_status::unmounted);
    EXPECT_TRUE(state.error_message.empty());
    EXPECT_EQ(state.available_space, 0u);
    EXPECT_EQ(state.total_space, 0u);
    EXPECT_EQ(state.latency_ms, -1);
}

TEST(nfs_mount_test, export_config_default_values)
{
    nfs_export_config config;

    EXPECT_TRUE(config.export_path.empty());
    EXPECT_TRUE(config.allowed_hosts.empty());
    EXPECT_TRUE(config.read_write);
    EXPECT_TRUE(config.sync);
    EXPECT_TRUE(config.no_subtree_check);
    EXPECT_FALSE(config.no_root_squash);
}

TEST(nfs_mount_test, generate_mount_command_basic)
{
    nfs_mount_config config;
    config.server_host = "192.168.1.100";
    config.remote_path = "/export/audio";
    config.local_mount = "/mnt/audio";
    config.version = nfs_version::v4;
    config.read_write = true;
    config.timeout_decisecs = 30;
    config.retrans = 3;

    auto cmd = generate_mount_command(config);

    EXPECT_NE(cmd.find("mount -t nfs"), std::string::npos);
    EXPECT_NE(cmd.find("192.168.1.100:/export/audio"), std::string::npos);
    EXPECT_NE(cmd.find("/mnt/audio"), std::string::npos);
    EXPECT_NE(cmd.find("soft"), std::string::npos);
    EXPECT_NE(cmd.find("timeo=30"), std::string::npos);
    EXPECT_NE(cmd.find("retrans=3"), std::string::npos);
    EXPECT_NE(cmd.find("rw"), std::string::npos);
    EXPECT_NE(cmd.find("nfsvers=4"), std::string::npos);
}

TEST(nfs_mount_test, generate_mount_command_read_only)
{
    nfs_mount_config config;
    config.server_host = "nas.local";
    config.remote_path = "/share";
    config.local_mount = "/mnt/share";
    config.version = nfs_version::v3;
    config.read_write = false;

    auto cmd = generate_mount_command(config);

    EXPECT_NE(cmd.find(",ro"), std::string::npos);
    EXPECT_NE(cmd.find("nfsvers=3"), std::string::npos);
}

TEST(nfs_mount_test, generate_export_line_basic)
{
    nfs_export_config config;
    config.export_path = "/home/piejam/recordings";
    config.allowed_hosts = "192.168.1.0/24";
    config.read_write = true;
    config.sync = true;
    config.no_subtree_check = true;
    config.no_root_squash = false;

    auto line = generate_export_line(config);

    EXPECT_NE(line.find("/home/piejam/recordings"), std::string::npos);
    EXPECT_NE(line.find("192.168.1.0/24"), std::string::npos);
    EXPECT_NE(line.find("rw"), std::string::npos);
    EXPECT_NE(line.find("sync"), std::string::npos);
    EXPECT_NE(line.find("no_subtree_check"), std::string::npos);
    EXPECT_EQ(line.find("no_root_squash"), std::string::npos);
}

TEST(nfs_mount_test, generate_export_line_with_root_squash)
{
    nfs_export_config config;
    config.export_path = "/export";
    config.allowed_hosts = "*";
    config.read_write = false;
    config.sync = false;
    config.no_subtree_check = false;
    config.no_root_squash = true;

    auto line = generate_export_line(config);

    EXPECT_NE(line.find("ro"), std::string::npos);
    EXPECT_NE(line.find("async"), std::string::npos);
    EXPECT_EQ(line.find("no_subtree_check"), std::string::npos);
    EXPECT_NE(line.find("no_root_squash"), std::string::npos);
}

} // namespace piejam::network_manager::test
