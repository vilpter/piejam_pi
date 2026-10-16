// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/network_manager/wifi_network_model.h>

namespace piejam::network_manager
{

// --- WiFiNetworkModel ---

WiFiNetworkModel::WiFiNetworkModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int
WiFiNetworkModel::rowCount(QModelIndex const& parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_networks.size());
}

QVariant
WiFiNetworkModel::data(QModelIndex const& index, int role) const
{
    if (!index.isValid() || index.row() < 0 ||
        index.row() >= static_cast<int>(m_networks.size()))
    {
        return {};
    }

    auto const& net = m_networks[static_cast<size_t>(index.row())];

    switch (role)
    {
    case Qt::DisplayRole:
    case SsidRole:
        return QString::fromStdString(net.ssid);

    case BssidRole:
        return QString::fromStdString(net.bssid);

    case SignalStrengthRole:
        return net.signal_strength;

    case SignalPercentRole:
        return net.signal_percent;

    case SecurityRole:
        return QString::fromUtf8(security_to_string(net.security));

    case SecurityTypeRole:
        return static_cast<int>(net.security);

    case BandRole:
        return QString::fromUtf8(band_to_string(net.band));

    case FrequencyMhzRole:
        return net.frequency_mhz;

    case IsConnectedRole:
        return net.is_connected;

    default:
        return {};
    }
}

QHash<int, QByteArray>
WiFiNetworkModel::roleNames() const
{
    static QHash<int, QByteArray> const s_roles = {
        {SsidRole, "ssid"},
        {BssidRole, "bssid"},
        {SignalStrengthRole, "signalStrength"},
        {SignalPercentRole, "signalPercent"},
        {SecurityRole, "security"},
        {SecurityTypeRole, "securityType"},
        {BandRole, "band"},
        {FrequencyMhzRole, "frequencyMhz"},
        {IsConnectedRole, "isConnected"}};
    return s_roles;
}

void
WiFiNetworkModel::setNetworks(std::vector<wifi_network> networks)
{
    beginResetModel();
    m_networks = std::move(networks);
    endResetModel();
    emit networksChanged();
}

void
WiFiNetworkModel::setConnectedNetwork(QString const& ssid)
{
    std::string ssid_str = ssid.toStdString();
    for (size_t i = 0; i < m_networks.size(); ++i)
    {
        bool was_connected = m_networks[i].is_connected;
        m_networks[i].is_connected = (m_networks[i].ssid == ssid_str);

        if (was_connected != m_networks[i].is_connected)
        {
            QModelIndex idx = index(static_cast<int>(i));
            emit dataChanged(idx, idx, {IsConnectedRole});
        }
    }
}

wifi_network const*
WiFiNetworkModel::networkAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_networks.size()))
        return nullptr;
    return &m_networks[static_cast<size_t>(index)];
}

int
WiFiNetworkModel::findBySsid(QString const& ssid) const
{
    std::string ssid_str = ssid.toStdString();
    for (size_t i = 0; i < m_networks.size(); ++i)
    {
        if (m_networks[i].ssid == ssid_str)
            return static_cast<int>(i);
    }
    return -1;
}

// --- WiFiSavedNetworkModel ---

WiFiSavedNetworkModel::WiFiSavedNetworkModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int
WiFiSavedNetworkModel::rowCount(QModelIndex const& parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_networks.size());
}

QVariant
WiFiSavedNetworkModel::data(QModelIndex const& index, int role) const
{
    if (!index.isValid() || index.row() < 0 ||
        index.row() >= static_cast<int>(m_networks.size()))
    {
        return {};
    }

    auto const& net = m_networks[static_cast<size_t>(index.row())];

    switch (role)
    {
    case Qt::DisplayRole:
    case SsidRole:
        return QString::fromStdString(net.ssid);

    case PriorityRole:
        return net.priority;

    case AutoConnectRole:
        return net.auto_connect;

    case HiddenRole:
        return net.hidden;

    case SecurityRole:
        return QString::fromUtf8(security_to_string(net.security));

    default:
        return {};
    }
}

QHash<int, QByteArray>
WiFiSavedNetworkModel::roleNames() const
{
    static QHash<int, QByteArray> const s_roles = {
        {SsidRole, "ssid"},
        {PriorityRole, "priority"},
        {AutoConnectRole, "autoConnect"},
        {HiddenRole, "hidden"},
        {SecurityRole, "security"}};
    return s_roles;
}

void
WiFiSavedNetworkModel::setNetworks(std::vector<wifi_saved_network> networks)
{
    beginResetModel();
    m_networks = std::move(networks);
    endResetModel();
    emit networksChanged();
}

} // namespace piejam::network_manager
