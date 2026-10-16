// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/network_manager/nfs_mount.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace piejam::network_manager
{

/// Callback types
using nfs_mount_status_callback =
    std::function<void(std::string const& id, nfs_mount_status)>;
using nfs_mount_error_callback =
    std::function<void(std::string const& id, std::string const& error)>;

/// NFS client manager for mounting remote NFS shares
///
/// Manages mount/unmount operations, saved mount configurations,
/// and provides connection monitoring.
class nfs_client
{
public:
    /// Default mount point base directory
    static constexpr char const* default_mount_base = "/mnt";

    explicit nfs_client(
        std::string config_path = "/home/piejam/.piejam/nfs_mounts.json");
    ~nfs_client();

    nfs_client(nfs_client const&) = delete;
    nfs_client& operator=(nfs_client const&) = delete;
    nfs_client(nfs_client&&) noexcept;
    nfs_client& operator=(nfs_client&&) noexcept;

    /// Check if NFS client utilities are installed
    [[nodiscard]] auto is_available() const -> bool;

    // --- Mount Configuration Management ---

    /// Get all saved mount configurations
    [[nodiscard]] auto saved_mounts() const -> std::vector<nfs_mount_config>;

    /// Get a specific mount configuration by ID
    [[nodiscard]] auto get_mount_config(std::string const& id) const
        -> nfs_mount_config const*;

    /// Add a new mount configuration
    /// @return The generated ID for the new configuration
    auto add_mount_config(nfs_mount_config config) -> std::string;

    /// Update an existing mount configuration
    /// @return true if config was found and updated
    bool update_mount_config(
        std::string const& id,
        nfs_mount_config const& config);

    /// Remove a mount configuration
    /// @return true if config was found and removed
    bool remove_mount_config(std::string const& id);

    // --- Mount Operations ---

    /// Get current state for a mount
    [[nodiscard]] auto get_mount_state(std::string const& id) const
        -> nfs_mount_state;

    /// Check if a mount is currently mounted
    [[nodiscard]] auto is_mounted(std::string const& id) const -> bool;

    /// Mount a configured NFS share
    /// @param id Mount configuration ID
    /// @return true if mount operation was started
    bool mount(std::string const& id);

    /// Unmount an NFS share
    /// @param id Mount configuration ID
    /// @return true if unmount operation was started
    bool unmount(std::string const& id);

    /// Unmount all currently mounted shares
    void unmount_all();

    /// Mount all shares configured for auto-mount
    void mount_auto_mounts();

    // --- Connection Testing ---

    /// Test if a server is reachable and exports are available
    /// @return true if server responded
    [[nodiscard]] auto test_connection(
        std::string const& server_host,
        std::string const& remote_path = "") const -> bool;

    /// Get available exports from a server
    [[nodiscard]] auto get_server_exports(std::string const& server_host) const
        -> std::vector<std::string>;

    /// Measure connection latency to a mounted share
    /// @return latency in milliseconds, or -1 if not mounted/error
    [[nodiscard]] auto measure_latency(std::string const& id) const -> int;

    // --- Space Information ---

    /// Get available space on a mounted share
    /// @return available bytes, or 0 if not mounted
    [[nodiscard]] auto get_available_space(std::string const& id) const
        -> uint64_t;

    /// Get total space on a mounted share
    /// @return total bytes, or 0 if not mounted
    [[nodiscard]] auto get_total_space(std::string const& id) const -> uint64_t;

    // --- Callbacks ---

    /// Set callback for mount status changes
    void set_mount_status_callback(nfs_mount_status_callback cb);

    /// Set callback for mount errors
    void set_mount_error_callback(nfs_mount_error_callback cb);

    // --- Utilities ---

    /// Generate a unique mount point path for a new mount
    [[nodiscard]] auto generate_mount_point(std::string const& name) const
        -> std::string;

    /// Refresh state for all mounts (check what's actually mounted)
    void refresh_mount_states();

private:
    void load_config();
    void save_config() const;
    bool create_mount_point(std::string const& path);
    bool execute_mount(nfs_mount_config const& config);
    bool execute_unmount(std::string const& mount_point);

    struct impl;
    std::unique_ptr<impl> m_impl;
};

} // namespace piejam::network_manager
