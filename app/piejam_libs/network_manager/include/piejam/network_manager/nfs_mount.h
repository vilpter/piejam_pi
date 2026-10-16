// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

#include <string>
#include <chrono>

namespace piejam::network_manager
{

/// NFS protocol version
enum class nfs_version
{
    v3,
    v4
};

/// Mount status
enum class nfs_mount_status
{
    unmounted,
    mounting,
    mounted,
    unmounting,
    error
};

/// Convert nfs_version to string
inline auto
nfs_version_to_string(nfs_version v) noexcept -> char const*
{
    switch (v)
    {
    case nfs_version::v3:
        return "NFSv3";
    case nfs_version::v4:
        return "NFSv4";
    default:
        return "Unknown";
    }
}

/// Convert nfs_mount_status to string
inline auto
nfs_mount_status_to_string(nfs_mount_status s) noexcept -> char const*
{
    switch (s)
    {
    case nfs_mount_status::unmounted:
        return "Unmounted";
    case nfs_mount_status::mounting:
        return "Mounting...";
    case nfs_mount_status::mounted:
        return "Mounted";
    case nfs_mount_status::unmounting:
        return "Unmounting...";
    case nfs_mount_status::error:
        return "Error";
    default:
        return "Unknown";
    }
}

/// Configuration for an NFS client mount
struct nfs_mount_config
{
    std::string id;             ///< Unique identifier (UUID)
    std::string name;           ///< User-friendly name
    std::string server_host;    ///< Server IP or hostname
    std::string remote_path;    ///< Remote export path (e.g., "/export/audio")
    std::string local_mount;    ///< Local mount point (e.g., "/mnt/remote_recordings")
    nfs_version version{nfs_version::v4};
    bool read_write{true};      ///< Mount read-write (false = read-only)
    bool auto_mount{false};     ///< Auto-mount on boot
    int timeout_decisecs{30};   ///< Timeout in deciseconds (30 = 3 seconds)
    int retrans{3};             ///< Number of retries
};

/// Runtime state for an NFS mount
struct nfs_mount_state
{
    std::string id;             ///< Config ID this state belongs to
    nfs_mount_status status{nfs_mount_status::unmounted};
    std::string error_message;  ///< Last error message if status is error
    uint64_t available_space{0}; ///< Available space in bytes (if mounted)
    uint64_t total_space{0};    ///< Total space in bytes (if mounted)
    int latency_ms{-1};         ///< Connection latency in ms (-1 if unknown)
};

/// NFS server export configuration
struct nfs_export_config
{
    std::string export_path;    ///< Path to export (e.g., "/home/piejam/recordings")
    std::string allowed_hosts;  ///< Allowed hosts pattern (e.g., "192.168.1.0/24", "*")
    bool read_write{true};      ///< Export read-write (false = read-only)
    bool sync{true};            ///< Synchronous writes
    bool no_subtree_check{true}; ///< No subtree checking (recommended for home dirs)
    bool no_root_squash{false}; ///< Allow root access (security risk)
};

/// Generate mount command string for display
inline auto
generate_mount_command(nfs_mount_config const& config) -> std::string
{
    std::string cmd = "mount -t nfs";

    // Options
    cmd += " -o ";
    cmd += "soft";
    cmd += ",timeo=" + std::to_string(config.timeout_decisecs);
    cmd += ",retrans=" + std::to_string(config.retrans);
    cmd += config.read_write ? ",rw" : ",ro";
    cmd += ",nfsvers=" + std::string(config.version == nfs_version::v4 ? "4" : "3");

    // Source
    cmd += " " + config.server_host + ":" + config.remote_path;

    // Target
    cmd += " " + config.local_mount;

    return cmd;
}

/// Generate export line for /etc/exports
inline auto
generate_export_line(nfs_export_config const& config) -> std::string
{
    std::string line = config.export_path + " " + config.allowed_hosts + "(";

    line += config.read_write ? "rw" : "ro";
    line += config.sync ? ",sync" : ",async";
    if (config.no_subtree_check)
        line += ",no_subtree_check";
    if (config.no_root_squash)
        line += ",no_root_squash";

    line += ")";
    return line;
}

} // namespace piejam::network_manager
