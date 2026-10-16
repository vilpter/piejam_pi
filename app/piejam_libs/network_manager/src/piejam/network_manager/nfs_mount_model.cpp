// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/network_manager/nfs_mount_model.h>

namespace piejam::network_manager
{

NFSMountModel::NFSMountModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int
NFSMountModel::rowCount(QModelIndex const& parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_mounts.size());
}

QVariant
NFSMountModel::data(QModelIndex const& index, int role) const
{
    if (!index.isValid() || index.row() < 0 ||
        index.row() >= static_cast<int>(m_mounts.size()))
    {
        return {};
    }

    auto const& mount = m_mounts[static_cast<size_t>(index.row())];
    auto const& config = mount.config;
    auto const& state = mount.state;

    switch (role)
    {
    case Qt::DisplayRole:
    case NameRole:
        return QString::fromStdString(config.name);

    case IdRole:
        return QString::fromStdString(config.id);

    case ServerHostRole:
        return QString::fromStdString(config.server_host);

    case RemotePathRole:
        return QString::fromStdString(config.remote_path);

    case LocalMountRole:
        return QString::fromStdString(config.local_mount);

    case VersionRole:
        return static_cast<int>(config.version);

    case VersionStringRole:
        return QString::fromUtf8(nfs_version_to_string(config.version));

    case ReadWriteRole:
        return config.read_write;

    case AutoMountRole:
        return config.auto_mount;

    case StatusRole:
        return static_cast<int>(state.status);

    case StatusStringRole:
        return QString::fromUtf8(nfs_mount_status_to_string(state.status));

    case ErrorMessageRole:
        return QString::fromStdString(state.error_message);

    case AvailableSpaceRole:
        return QVariant::fromValue(state.available_space);

    case TotalSpaceRole:
        return QVariant::fromValue(state.total_space);

    case LatencyMsRole:
        return state.latency_ms;

    default:
        return {};
    }
}

QHash<int, QByteArray>
NFSMountModel::roleNames() const
{
    static QHash<int, QByteArray> const s_roles = {
        {IdRole, "configId"},
        {NameRole, "name"},
        {ServerHostRole, "serverHost"},
        {RemotePathRole, "remotePath"},
        {LocalMountRole, "localMount"},
        {VersionRole, "version"},
        {VersionStringRole, "versionString"},
        {ReadWriteRole, "readWrite"},
        {AutoMountRole, "autoMount"},
        {StatusRole, "status"},
        {StatusStringRole, "statusString"},
        {ErrorMessageRole, "errorMessage"},
        {AvailableSpaceRole, "availableSpace"},
        {TotalSpaceRole, "totalSpace"},
        {LatencyMsRole, "latencyMs"}};
    return s_roles;
}

void
NFSMountModel::setMounts(std::vector<nfs_mount_config> configs)
{
    beginResetModel();
    m_mounts.clear();
    m_mounts.reserve(configs.size());

    for (auto& config : configs)
    {
        MountData data;
        data.config = std::move(config);
        data.state.id = data.config.id;
        m_mounts.push_back(std::move(data));
    }

    endResetModel();
    emit mountsChanged();
}

void
NFSMountModel::setStates(std::vector<nfs_mount_state> states)
{
    for (auto const& state : states)
    {
        int idx = findById(QString::fromStdString(state.id));
        if (idx >= 0)
        {
            m_mounts[static_cast<size_t>(idx)].state = state;
            QModelIndex modelIdx = index(idx);
            emit dataChanged(
                modelIdx,
                modelIdx,
                {StatusRole,
                 StatusStringRole,
                 ErrorMessageRole,
                 AvailableSpaceRole,
                 TotalSpaceRole,
                 LatencyMsRole});
        }
    }
}

void
NFSMountModel::updateState(std::string const& id, nfs_mount_state const& state)
{
    int idx = findById(QString::fromStdString(id));
    if (idx < 0)
        return;

    m_mounts[static_cast<size_t>(idx)].state = state;

    QModelIndex modelIdx = index(idx);
    emit dataChanged(
        modelIdx,
        modelIdx,
        {StatusRole,
         StatusStringRole,
         ErrorMessageRole,
         AvailableSpaceRole,
         TotalSpaceRole,
         LatencyMsRole});
}

auto
NFSMountModel::configAt(int index) const -> nfs_mount_config const*
{
    if (index < 0 || index >= static_cast<int>(m_mounts.size()))
        return nullptr;
    return &m_mounts[static_cast<size_t>(index)].config;
}

auto
NFSMountModel::findById(QString const& id) const -> int
{
    std::string id_str = id.toStdString();
    for (size_t i = 0; i < m_mounts.size(); ++i)
    {
        if (m_mounts[i].config.id == id_str)
            return static_cast<int>(i);
    }
    return -1;
}

} // namespace piejam::network_manager
