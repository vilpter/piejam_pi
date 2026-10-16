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

/// NFS server status
enum class nfs_server_status
{
    inactive,
    starting,
    active,
    stopping,
    error
};

/// Callback types
using nfs_server_status_callback = std::function<void(nfs_server_status)>;
using nfs_server_error_callback = std::function<void(std::string const&)>;

/// NFS server manager for exporting recordings directory
///
/// Manages /etc/exports and NFS server daemon (nfs-kernel-server)
class nfs_server
{
public:
    explicit nfs_server(std::string export_path = "/home/piejam/recordings");
    ~nfs_server();

    nfs_server(nfs_server const&) = delete;
    nfs_server& operator=(nfs_server const&) = delete;
    nfs_server(nfs_server&&) noexcept;
    nfs_server& operator=(nfs_server&&) noexcept;

    /// Get the export path
    [[nodiscard]] auto export_path() const noexcept -> std::string const&;

    /// Get current server status
    [[nodiscard]] auto status() const noexcept -> nfs_server_status;

    /// Check if server is active
    [[nodiscard]] auto is_active() const noexcept -> bool;

    /// Check if NFS server packages are installed
    [[nodiscard]] auto is_available() const -> bool;

    /// Get current export configuration
    [[nodiscard]] auto export_config() const noexcept -> nfs_export_config const&;

    /// Get mount command for display to user
    [[nodiscard]] auto mount_command() const -> std::string;

    /// Get local IP address(es) for display
    [[nodiscard]] auto local_ip_addresses() const -> std::vector<std::string>;

    /// Enable NFS server
    /// @return true if operation was started successfully
    bool enable();

    /// Disable NFS server
    /// @return true if operation was started successfully
    bool disable();

    /// Restart NFS server
    /// @return true if operation was started successfully
    bool restart();

    /// Update export configuration
    void set_export_config(nfs_export_config const& config);

    /// Set read-write mode for export
    void set_read_write(bool read_write);

    /// Set allowed hosts pattern
    void set_allowed_hosts(std::string const& hosts);

    /// Test if the server is accessible from localhost
    /// @return true if mount test succeeds
    [[nodiscard]] auto test_connection() -> bool;

    /// Set callback for status changes
    void set_status_callback(nfs_server_status_callback cb);

    /// Set callback for errors
    void set_error_callback(nfs_server_error_callback cb);

private:
    void update_exports_file();
    void execute_systemctl(std::string const& action);
    void refresh_status();

    struct impl;
    std::unique_ptr<impl> m_impl;
};

} // namespace piejam::network_manager
