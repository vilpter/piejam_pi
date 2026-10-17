// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/network_manager/nfs_export.h>

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

/// Shares PieJam's recordings directory over NFS, on demand.
///
/// The server is never enabled at boot (the network is never a boot
/// dependency, ADR 0005). Enabling starts `nfs-server` through polkit and
/// exports the recordings to the Pi's current subnet with `exportfs`;
/// disabling unexports and stops the service. Nothing is written to
/// /etc/exports, so the share is gone after a reboot. The recordings are
/// exposed under a short path (`/srv/rec`, a bind mount) so the mount command
/// clients type stays short. The privileged steps run through a restricted
/// polkit rule and a validated root helper (`piejam-nfs-export`); the app
/// runs unprivileged (ADR 0004).
///
/// The path passed here is the exported path shown in the UI; the helper owns
/// the real recordings location and the bind mount.
class nfs_server
{
public:
    explicit nfs_server(std::string export_path = "/srv/rec");
    ~nfs_server();

    nfs_server(nfs_server const&) = delete;
    nfs_server& operator=(nfs_server const&) = delete;
    nfs_server(nfs_server&&) noexcept;
    nfs_server& operator=(nfs_server&&) noexcept;

    /// The directory that gets exported.
    [[nodiscard]] auto export_path() const noexcept -> std::string const&;

    /// Current server status.
    [[nodiscard]] auto status() const noexcept -> nfs_server_status;

    /// Whether the server is active.
    [[nodiscard]] auto is_active() const noexcept -> bool;

    /// Whether the NFS server tools are installed (exportfs present).
    [[nodiscard]] auto is_available() const -> bool;

    /// The export configuration last applied (path, subnet, access mode).
    [[nodiscard]] auto export_config() const noexcept
        -> nfs_export_config const&;

    /// The Pi's current non-loopback IPv4 address(es), for display.
    [[nodiscard]] auto local_ip_addresses() const -> std::vector<std::string>;

    /// Start the server and export the recordings directory, read-write, to
    /// the Pi's current subnet. @return true if the server ended up active.
    bool enable();

    /// Unexport and stop the server. @return true if it ended up inactive.
    bool disable();

    /// Re-read the service state and notify the status callback.
    void refresh_status();

    void set_status_callback(nfs_server_status_callback cb);
    void set_error_callback(nfs_server_error_callback cb);

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

} // namespace piejam::network_manager
