// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/NetworkSettings.h>

#include <piejam/network_manager/network_controller.h>
#include <piejam/network_manager/nfs_server.h>
#include <piejam/network_manager/wifi_manager.h>
#include <piejam/network_manager/wifi_network_model.h>

#include <piejam/runtime/actions/network_actions.h>
#include <piejam/runtime/selectors.h>

#include <spdlog/spdlog.h>

#include <QPointer>

#include <chrono>

namespace piejam::gui::model
{

struct NetworkSettings::impl
{
    std::shared_ptr<network_manager::network_controller> networkController;
    std::shared_ptr<network_manager::wifi_manager> wifiManager;
    std::shared_ptr<network_manager::nfs_server> nfsServer;

    network_manager::WiFiNetworkModel* availableNetworksModel{};
    network_manager::WiFiSavedNetworkModel* savedNetworksModel{};

    bool connectPending{};
};

NetworkSettings::NetworkSettings(
    runtime::state_access const& state_access,
    std::shared_ptr<network_manager::network_controller> netCtrl,
    std::shared_ptr<network_manager::wifi_manager> wifiMgr,
    std::shared_ptr<network_manager::nfs_server> nfsSrv)
    : CompositeSubscribableModel{state_access}
    , m_impl(std::make_unique<impl>())
{
    m_impl->networkController = std::move(netCtrl);
    m_impl->wifiManager = std::move(wifiMgr);
    m_impl->nfsServer = std::move(nfsSrv);

    m_impl->availableNetworksModel =
        &addQObject<network_manager::WiFiNetworkModel>();
    m_impl->savedNetworksModel =
        &addQObject<network_manager::WiFiSavedNetworkModel>();

    setupCallbacks();
}

NetworkSettings::~NetworkSettings()
{
    // The backends outlive this model; stop them calling back into it.
    if (m_impl->networkController)
    {
        m_impl->networkController->set_state_change_callback({});
        m_impl->networkController->set_error_callback({});
    }

    if (m_impl->wifiManager)
    {
        m_impl->wifiManager->set_scan_complete_callback({});
        m_impl->wifiManager->set_connection_changed_callback({});
        m_impl->wifiManager->set_saved_networks_callback({});
        m_impl->wifiManager->set_error_callback({});
    }

    if (m_impl->nfsServer)
    {
        m_impl->nfsServer->set_status_callback({});
        m_impl->nfsServer->set_error_callback({});
    }
}

void
NetworkSettings::onSubscribe()
{
    observe(
        runtime::selectors::select_wifi_enabled,
        [this](bool enabled) {
            setNetworkEnabled(enabled);
            if (!enabled)
            {
                setWifiConnected(false);
                setCurrentSsid(QString());
                setIpAddress(QString());
                setSignalStrength(0);
                setIsConnecting(false);
                setIsScanning(false);

                if (m_impl->nfsServer && m_impl->nfsServer->is_active())
                {
                    m_impl->nfsServer->disable();
                    setNfsServerActive(false);
                }
            }
        });

    refreshStatus();

    // NetworkManager connects, reconnects and drops WiFi on its own, so
    // follow it while the page is visible.
    requestUpdates(std::chrono::seconds{3}, [this]() { pollStatus(); });
}

void
NetworkSettings::pollStatus()
{
    if (m_impl->networkController)
    {
        m_impl->networkController->refresh();
    }

    if (m_impl->wifiManager)
    {
        m_impl->wifiManager->refresh_status();
    }
}

void
NetworkSettings::setupCallbacks()
{
    // The backends call back on the Qt main thread (they run nmcli through
    // QProcess), so no marshalling is needed.
    if (m_impl->networkController)
    {
        m_impl->networkController->set_state_change_callback(
            [this](bool enabled) {
                runtime::actions::set_wifi_enabled action;
                action.enabled = enabled;
                dispatch(action);
            });

        m_impl->networkController->set_error_callback(
            [this](std::string const& error) {
                emit networkError(QString::fromStdString(error));
            });
    }

    if (m_impl->wifiManager)
    {
        m_impl->wifiManager->set_scan_complete_callback(
            [this](std::vector<network_manager::wifi_network> const& networks) {
                m_impl->availableNetworksModel->setNetworks(networks);
                setIsScanning(false);
                emit scanCompleted();
            });

        m_impl->wifiManager->set_saved_networks_callback(
            [this](
                std::vector<network_manager::wifi_saved_network> const&
                    networks) {
                m_impl->savedNetworksModel->setNetworks(networks);
            });

        m_impl->wifiManager->set_connection_changed_callback(
            [this](
                network_manager::wifi_connection_status status,
                network_manager::wifi_network const* network) {
                bool connected =
                    (status ==
                     network_manager::wifi_connection_status::connected);

                setWifiConnected(connected);
                // An explicit connect attempt keeps isConnecting set
                // until it finishes.
                if (!m_impl->connectPending)
                {
                    setIsConnecting(
                        status ==
                        network_manager::wifi_connection_status::connecting);
                }

                if (connected && network)
                {
                    setCurrentSsid(
                        QString::fromStdString(network->ssid));
                    setSignalStrength(network->signal_percent);
                    m_impl->availableNetworksModel
                        ->setConnectedNetwork(
                            QString::fromStdString(network->ssid));
                    emit connectionSucceeded(
                        QString::fromStdString(network->ssid));

                    auto ip = QString::fromStdString(
                        m_impl->wifiManager->ip_address());
                    setIpAddress(ip);
                }
                else if (!connected)
                {
                    setCurrentSsid(QString());
                    setSignalStrength(0);
                    m_impl->availableNetworksModel
                        ->setConnectedNetwork(QString());

                    if (m_impl->nfsServer &&
                        m_impl->nfsServer->is_active())
                    {
                        m_impl->nfsServer->disable();
                        setNfsServerActive(false);
                    }

                    setIpAddress(QString());
                }
            });

        m_impl->wifiManager->set_error_callback(
            [this](std::string const& error) {
                emit networkError(QString::fromStdString(error));
            });
    }

    if (m_impl->nfsServer)
    {
        m_impl->nfsServer->set_status_callback(
            [this](network_manager::nfs_server_status status) {
                bool active =
                    (status ==
                     network_manager::nfs_server_status::active);
                setNfsServerActive(active);
                emit nfsServerStatusChanged(active);
            });

        m_impl->nfsServer->set_error_callback(
            [this](std::string const& error) {
                emit nfsServerError(QString::fromStdString(error));
            });
    }
}

void
NetworkSettings::updateWifiStatus()
{
    if (!m_impl->wifiManager)
        return;

    auto status = m_impl->wifiManager->status();
    bool connected =
        (status == network_manager::wifi_connection_status::connected);

    setWifiConnected(connected);
    setIsConnecting(
        status == network_manager::wifi_connection_status::connecting);

    if (connected)
    {
        setCurrentSsid(
            QString::fromStdString(m_impl->wifiManager->current_ssid()));
        setIpAddress(
            QString::fromStdString(m_impl->wifiManager->ip_address()));
    }
    else
    {
        setCurrentSsid(QString());
        setIpAddress(QString());
        setSignalStrength(0);
    }
}

void
NetworkSettings::updateNfsServerStatus()
{
    if (!m_impl->nfsServer)
    {
        setNfsServerAvailable(false);
        return;
    }

    setNfsServerAvailable(m_impl->nfsServer->is_available());
    setNfsServerActive(m_impl->nfsServer->is_active());
    setNfsExportPath(
        QString::fromStdString(m_impl->nfsServer->export_path()));
}

void
NetworkSettings::enableNetwork()
{
    if (m_impl->networkController)
    {
        m_impl->networkController->enable();
    }
}

void
NetworkSettings::disableNetwork()
{
    if (m_impl->networkController)
    {
        m_impl->networkController->disable();
    }
}

void
NetworkSettings::toggleNetwork()
{
    // The switch follows networkEnabled, which updates once
    // NetworkManager has switched the radio.
    if (m_impl->networkController)
    {
        if (m_impl->networkController->is_enabled())
        {
            m_impl->networkController->disable();
        }
        else
        {
            m_impl->networkController->enable();
        }
    }
}

void
NetworkSettings::scanNetworks()
{
    if (m_impl->wifiManager && m_impl->wifiManager->is_interface_available())
    {
        setIsScanning(true);
        m_impl->wifiManager->scan_networks();
    }
}

void
NetworkSettings::connectToNetwork(
    QString const& ssid,
    QString const& password,
    bool remember)
{
    if (!m_impl->wifiManager)
        return;

    m_impl->connectPending = true;
    setIsConnecting(true);

    // NetworkManager connects asynchronously; the callback runs on the Qt
    // main thread when it has succeeded or given up.
    m_impl->wifiManager->connect(
        ssid.toStdString(),
        password.toStdString(),
        remember,
        [self = QPointer<NetworkSettings>{this},
         ssid](bool ok, std::string const& error) {
            if (!self)
            {
                return;
            }

            self->m_impl->connectPending = false;
            self->setIsConnecting(false);

            if (!ok)
            {
                emit self->connectionFailed(
                    ssid,
                    QString::fromStdString(error));
            }
        });
}

void
NetworkSettings::disconnectWifi()
{
    if (m_impl->wifiManager)
    {
        m_impl->wifiManager->disconnect();
    }
}

void
NetworkSettings::forgetNetwork(QString const& ssid)
{
    if (m_impl->wifiManager)
    {
        m_impl->wifiManager->forget_network(ssid.toStdString());
    }
}

QObject*
NetworkSettings::availableNetworks() const
{
    return m_impl->availableNetworksModel;
}

QObject*
NetworkSettings::savedNetworks() const
{
    return m_impl->savedNetworksModel;
}

void
NetworkSettings::enableNfsServer()
{
    if (m_impl->nfsServer)
    {
        m_impl->nfsServer->enable();
        updateNfsServerStatus();
    }
}

void
NetworkSettings::disableNfsServer()
{
    if (m_impl->nfsServer)
    {
        m_impl->nfsServer->disable();
        updateNfsServerStatus();
    }
}

void
NetworkSettings::refreshStatus()
{
    // Show what's known now, then ask NetworkManager for fresh state; the
    // callbacks fill in the rest.
    updateWifiStatus();
    updateNfsServerStatus();

    if (m_impl->wifiManager)
    {
        m_impl->availableNetworksModel->setNetworks(
            m_impl->wifiManager->available_networks());
        m_impl->savedNetworksModel->setNetworks(
            m_impl->wifiManager->saved_networks());

        m_impl->wifiManager->list_networks();
        m_impl->wifiManager->refresh_saved_networks();
    }

    pollStatus();
}

} // namespace piejam::gui::model
