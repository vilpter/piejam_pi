// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/SubscribableModel.h>
#include <piejam/gui/model/fwd.h>
#include <piejam/network_manager/fwd.h>

#include <QString>

#include <memory>

namespace piejam::gui::model
{

class NetworkSettings final : public CompositeSubscribableModel
{
    Q_OBJECT

    // Redux-backed properties (observed via selectors)
    PIEJAM_GUI_PROPERTY(bool, networkEnabled, setNetworkEnabled)

    // Backend-backed properties (updated via callbacks)
    PIEJAM_GUI_PROPERTY(bool, wifiConnected, setWifiConnected)
    PIEJAM_GUI_PROPERTY(QString, currentSsid, setCurrentSsid)
    PIEJAM_GUI_PROPERTY(QString, ipAddress, setIpAddress)
    PIEJAM_GUI_PROPERTY(int, signalStrength, setSignalStrength)
    PIEJAM_GUI_PROPERTY(bool, isScanning, setIsScanning)
    PIEJAM_GUI_PROPERTY(bool, isConnecting, setIsConnecting)

    // NFS server properties
    PIEJAM_GUI_PROPERTY(bool, nfsServerAvailable, setNfsServerAvailable)
    PIEJAM_GUI_PROPERTY(bool, nfsServerActive, setNfsServerActive)
    PIEJAM_GUI_PROPERTY(QString, nfsExportPath, setNfsExportPath)

public:
    explicit NetworkSettings(
        runtime::state_access const&,
        std::shared_ptr<network_manager::network_controller>,
        std::shared_ptr<network_manager::wifi_manager>,
        std::shared_ptr<network_manager::nfs_server>);

    ~NetworkSettings() override;

    // Network control
    Q_INVOKABLE void enableNetwork();
    Q_INVOKABLE void disableNetwork();
    Q_INVOKABLE void toggleNetwork();

    // WiFi operations
    Q_INVOKABLE void scanNetworks();
    Q_INVOKABLE void connectToNetwork(
        QString const& ssid,
        QString const& password,
        bool remember);
    Q_INVOKABLE void disconnectWifi();
    Q_INVOKABLE void forgetNetwork(QString const& ssid);

    // Models for QML
    Q_INVOKABLE QObject* availableNetworks() const;
    Q_INVOKABLE QObject* savedNetworks() const;

    // NFS server operations. The recordings directory is shared read-write to
    // the whole local subnet, so the UI warns before calling enableNfsServer.
    Q_INVOKABLE void enableNfsServer();
    Q_INVOKABLE void disableNfsServer();

    // Refresh status
    Q_INVOKABLE void refreshStatus();

signals:
    void connectionSucceeded(QString ssid);
    void connectionFailed(QString ssid, QString error);
    void scanCompleted();
    void networkError(QString message);

    void nfsServerStatusChanged(bool active);
    void nfsServerError(QString message);

private:
    void onSubscribe() override;
    void setupCallbacks();
    void pollStatus();
    void updateWifiStatus();
    void updateNfsServerStatus();

    struct impl;
    std::unique_ptr<impl> m_impl;
};

} // namespace piejam::gui::model
