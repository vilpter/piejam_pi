// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace piejam::network_manager
{

class network_controller;
class wifi_manager;
class WiFiNetworkModel;
class WiFiSavedNetworkModel;

struct wifi_network;
struct wifi_saved_network;
enum class wifi_security;
enum class wifi_band;
enum class wifi_connection_status;

// NFS types
class nfs_server;
class nfs_client;
class NFSMountModel;

struct nfs_mount_config;
struct nfs_mount_state;
struct nfs_export_config;

enum class nfs_version;
enum class nfs_mount_status;
enum class nfs_server_status;

} // namespace piejam::network_manager
