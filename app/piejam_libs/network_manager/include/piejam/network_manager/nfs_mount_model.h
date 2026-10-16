// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/network_manager/nfs_mount.h>

#include <QAbstractListModel>

#include <vector>

namespace piejam::network_manager
{

/// Qt model exposing NFS mount configurations to QML
class NFSMountModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles : int
    {
        IdRole = Qt::UserRole + 1,
        NameRole,
        ServerHostRole,
        RemotePathRole,
        LocalMountRole,
        VersionRole,
        VersionStringRole,
        ReadWriteRole,
        AutoMountRole,
        StatusRole,
        StatusStringRole,
        ErrorMessageRole,
        AvailableSpaceRole,
        TotalSpaceRole,
        LatencyMsRole
    };
    Q_ENUM(Roles)

    explicit NFSMountModel(QObject* parent = nullptr);

    [[nodiscard]] auto
    rowCount(QModelIndex const& parent = QModelIndex()) const -> int override;

    [[nodiscard]] auto data(QModelIndex const& index, int role) const
        -> QVariant override;

    [[nodiscard]] auto roleNames() const -> QHash<int, QByteArray> override;

    /// Update the entire list of mounts
    void setMounts(std::vector<nfs_mount_config> configs);

    /// Update states for all mounts
    void setStates(std::vector<nfs_mount_state> states);

    /// Update state for a specific mount
    void updateState(std::string const& id, nfs_mount_state const& state);

    /// Get config at index
    [[nodiscard]] auto configAt(int index) const -> nfs_mount_config const*;

    /// Find index by ID
    [[nodiscard]] auto findById(QString const& id) const -> int;

signals:
    void mountsChanged();

private:
    struct MountData
    {
        nfs_mount_config config;
        nfs_mount_state state;
    };

    std::vector<MountData> m_mounts;
};

} // namespace piejam::network_manager
