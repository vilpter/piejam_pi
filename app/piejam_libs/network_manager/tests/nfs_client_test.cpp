// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/network_manager/nfs_client.h>

#include <gtest/gtest.h>

#include <filesystem>

namespace piejam::network_manager::test
{

class nfs_client_test : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Use a temporary config file for tests
        m_config_path = std::filesystem::temp_directory_path() /
                        "piejam_test_nfs_mounts.json";
        // Clean up any previous test file
        std::filesystem::remove(m_config_path);
    }

    void TearDown() override
    {
        // Clean up test file
        std::filesystem::remove(m_config_path);
    }

    std::filesystem::path m_config_path;
};

TEST_F(nfs_client_test, constructor_creates_empty_state)
{
    nfs_client client(m_config_path.string());

    EXPECT_TRUE(client.saved_mounts().empty());
}

TEST_F(nfs_client_test, add_mount_config_generates_id)
{
    nfs_client client(m_config_path.string());

    nfs_mount_config config;
    config.name = "Test Mount";
    config.server_host = "192.168.1.100";
    config.remote_path = "/export";

    auto id = client.add_mount_config(config);

    EXPECT_FALSE(id.empty());
}

TEST_F(nfs_client_test, add_mount_config_can_be_retrieved)
{
    nfs_client client(m_config_path.string());

    nfs_mount_config config;
    config.name = "Test Mount";
    config.server_host = "192.168.1.100";
    config.remote_path = "/export";
    config.read_write = true;
    config.auto_mount = false;

    auto id = client.add_mount_config(config);

    auto const* retrieved = client.get_mount_config(id);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->name, "Test Mount");
    EXPECT_EQ(retrieved->server_host, "192.168.1.100");
    EXPECT_EQ(retrieved->remote_path, "/export");
    EXPECT_TRUE(retrieved->read_write);
    EXPECT_FALSE(retrieved->auto_mount);
}

TEST_F(nfs_client_test, saved_mounts_returns_all_configs)
{
    nfs_client client(m_config_path.string());

    nfs_mount_config config1;
    config1.name = "Mount 1";
    config1.server_host = "host1";
    config1.remote_path = "/path1";
    client.add_mount_config(config1);

    nfs_mount_config config2;
    config2.name = "Mount 2";
    config2.server_host = "host2";
    config2.remote_path = "/path2";
    client.add_mount_config(config2);

    auto mounts = client.saved_mounts();
    EXPECT_EQ(mounts.size(), 2u);
}

TEST_F(nfs_client_test, remove_mount_config_removes_entry)
{
    nfs_client client(m_config_path.string());

    nfs_mount_config config;
    config.name = "Test Mount";
    config.server_host = "host";
    config.remote_path = "/path";

    auto id = client.add_mount_config(config);
    EXPECT_EQ(client.saved_mounts().size(), 1u);

    bool removed = client.remove_mount_config(id);
    EXPECT_TRUE(removed);
    EXPECT_TRUE(client.saved_mounts().empty());
}

TEST_F(nfs_client_test, remove_nonexistent_config_returns_false)
{
    nfs_client client(m_config_path.string());

    bool removed = client.remove_mount_config("nonexistent-id");
    EXPECT_FALSE(removed);
}

TEST_F(nfs_client_test, update_mount_config_updates_entry)
{
    nfs_client client(m_config_path.string());

    nfs_mount_config config;
    config.name = "Original Name";
    config.server_host = "host";
    config.remote_path = "/path";

    auto id = client.add_mount_config(config);

    nfs_mount_config updated;
    updated.name = "Updated Name";
    updated.server_host = "new-host";
    updated.remote_path = "/new-path";

    bool success = client.update_mount_config(id, updated);
    EXPECT_TRUE(success);

    auto const* retrieved = client.get_mount_config(id);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->name, "Updated Name");
    EXPECT_EQ(retrieved->server_host, "new-host");
    EXPECT_EQ(retrieved->remote_path, "/new-path");
}

TEST_F(nfs_client_test, update_nonexistent_config_returns_false)
{
    nfs_client client(m_config_path.string());

    nfs_mount_config config;
    config.name = "Test";

    bool success = client.update_mount_config("nonexistent-id", config);
    EXPECT_FALSE(success);
}

TEST_F(nfs_client_test, get_mount_state_returns_default_for_valid_id)
{
    nfs_client client(m_config_path.string());

    nfs_mount_config config;
    config.name = "Test";
    config.server_host = "host";
    config.remote_path = "/path";

    auto id = client.add_mount_config(config);

    auto state = client.get_mount_state(id);
    EXPECT_EQ(state.id, id);
    EXPECT_EQ(state.status, nfs_mount_status::unmounted);
}

TEST_F(nfs_client_test, get_mount_state_returns_default_for_invalid_id)
{
    nfs_client client(m_config_path.string());

    auto state = client.get_mount_state("nonexistent-id");
    EXPECT_EQ(state.id, "nonexistent-id");
    EXPECT_EQ(state.status, nfs_mount_status::unmounted);
}

TEST_F(nfs_client_test, is_mounted_returns_false_for_new_config)
{
    nfs_client client(m_config_path.string());

    nfs_mount_config config;
    config.name = "Test";
    config.server_host = "host";
    config.remote_path = "/path";

    auto id = client.add_mount_config(config);

    EXPECT_FALSE(client.is_mounted(id));
}

TEST_F(nfs_client_test, generate_mount_point_sanitizes_name)
{
    nfs_client client(m_config_path.string());

    auto path = client.generate_mount_point("My NAS Share!");

    EXPECT_NE(path.find("/mnt/"), std::string::npos);
    EXPECT_EQ(path.find(" "), std::string::npos);
    EXPECT_EQ(path.find("!"), std::string::npos);
}

TEST_F(nfs_client_test, config_persists_after_reload)
{
    {
        nfs_client client(m_config_path.string());

        nfs_mount_config config;
        config.name = "Persistent Mount";
        config.server_host = "192.168.1.100";
        config.remote_path = "/export";
        config.version = nfs_version::v4;
        config.read_write = true;
        config.auto_mount = true;

        client.add_mount_config(config);
    }

    // Reload from file
    nfs_client client2(m_config_path.string());

    auto mounts = client2.saved_mounts();
    ASSERT_EQ(mounts.size(), 1u);
    EXPECT_EQ(mounts[0].name, "Persistent Mount");
    EXPECT_EQ(mounts[0].server_host, "192.168.1.100");
    EXPECT_EQ(mounts[0].remote_path, "/export");
    EXPECT_EQ(mounts[0].version, nfs_version::v4);
    EXPECT_TRUE(mounts[0].read_write);
    EXPECT_TRUE(mounts[0].auto_mount);
}

TEST_F(nfs_client_test, move_constructor)
{
    nfs_client client1(m_config_path.string());

    nfs_mount_config config;
    config.name = "Test";
    config.server_host = "host";
    config.remote_path = "/path";
    client1.add_mount_config(config);

    nfs_client client2(std::move(client1));
    EXPECT_EQ(client2.saved_mounts().size(), 1u);
}

TEST_F(nfs_client_test, move_assignment)
{
    nfs_client client1(m_config_path.string());

    nfs_mount_config config;
    config.name = "Test";
    config.server_host = "host";
    config.remote_path = "/path";
    client1.add_mount_config(config);

    auto other_path = std::filesystem::temp_directory_path() /
                      "piejam_test_nfs_mounts2.json";
    nfs_client client2(other_path.string());
    client2 = std::move(client1);

    EXPECT_EQ(client2.saved_mounts().size(), 1u);

    std::filesystem::remove(other_path);
}

} // namespace piejam::network_manager::test
