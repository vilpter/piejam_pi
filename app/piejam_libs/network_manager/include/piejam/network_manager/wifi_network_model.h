// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/network_manager/wifi_network.h>

#include <QAbstractListModel>

#include <vector>

namespace piejam::network_manager
{

/// Qt list model for WiFi networks, suitable for QML ListView.
///
/// Exposes wifi_network data with the following roles:
/// - ssid: Network name (QString)
/// - bssid: MAC address (QString)
/// - signalStrength: Signal in dBm (int)
/// - signalPercent: Signal as 0-100 (int)
/// - security: Security type string (QString)
/// - securityType: Security enum value (int)
/// - band: Frequency band string (QString)
/// - frequencyMhz: Frequency in MHz (int)
/// - isConnected: Currently connected (bool)
class WiFiNetworkModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles : int
    {
        SsidRole = Qt::UserRole + 1,
        BssidRole,
        SignalStrengthRole,
        SignalPercentRole,
        SecurityRole,
        SecurityTypeRole,
        BandRole,
        FrequencyMhzRole,
        IsConnectedRole
    };
    Q_ENUM(Roles)

    explicit WiFiNetworkModel(QObject* parent = nullptr);

    // QAbstractListModel interface
    [[nodiscard]] int rowCount(QModelIndex const& parent = QModelIndex()) const override;

    [[nodiscard]] QVariant
    data(QModelIndex const& index, int role = Qt::DisplayRole) const override;

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    // Update model data
    void setNetworks(std::vector<wifi_network> networks);

    // Mark a network as connected/disconnected
    void setConnectedNetwork(QString const& ssid);

    // Get network at index
    [[nodiscard]] wifi_network const* networkAt(int index) const;

    // Find network by SSID
    [[nodiscard]] int findBySsid(QString const& ssid) const;

signals:
    void networksChanged();

private:
    std::vector<wifi_network> m_networks;
};

/// Qt list model for saved WiFi networks.
class WiFiSavedNetworkModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles : int
    {
        SsidRole = Qt::UserRole + 1,
        PriorityRole,
        AutoConnectRole,
        HiddenRole,
        SecurityRole
    };
    Q_ENUM(Roles)

    explicit WiFiSavedNetworkModel(QObject* parent = nullptr);

    // QAbstractListModel interface
    [[nodiscard]] int rowCount(QModelIndex const& parent = QModelIndex()) const override;

    [[nodiscard]] QVariant
    data(QModelIndex const& index, int role = Qt::DisplayRole) const override;

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    // Update model data
    void setNetworks(std::vector<wifi_saved_network> networks);

signals:
    void networksChanged();

private:
    std::vector<wifi_saved_network> m_networks;
};

} // namespace piejam::network_manager
