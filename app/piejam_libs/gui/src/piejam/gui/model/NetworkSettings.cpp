// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/NetworkSettings.h>

#include <piejam/network_manager/network_controller.h>
#include <piejam/network_manager/nfs_client.h>
#include <piejam/network_manager/nfs_mount.h>
#include <piejam/network_manager/nfs_mount_model.h>
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
    std::shared_ptr<network_manager::nfs_client> nfsClient;

    network_manager::WiFiNetworkModel* availableNetworksModel{};
    network_manager::WiFiSavedNetworkModel* savedNetworksModel{};
    network_manager::NFSMountModel* nfsMountModel{};

    bool connectPending{};
};

NetworkSettings::NetworkSettings(
    runtime::state_access const& state_access,
    std::shared_ptr<network_manager::network_controller> netCtrl,
    std::shared_ptr<network_manager::wifi_manager> wifiMgr,
    std::shared_ptr<network_manager::nfs_server> nfsSrv,
    std::shared_ptr<network_manager::nfs_client> nfsCli)
    : CompositeSubscribableModel{state_access}
    , m_impl(std::make_unique<impl>())
{
    m_impl->networkController = std::move(netCtrl);
    m_impl->wifiManager = std::move(wifiMgr);
    m_impl->nfsServer = std::move(nfsSrv);
    m_impl->nfsClient = std::move(nfsCli);

    m_impl->availableNetworksModel =
        &addQObject<network_manager::WiFiNetworkModel>();
    m_impl->savedNetworksModel =
        &addQObject<network_manager::WiFiSavedNetworkModel>();
    m_impl->nfsMountModel =
        &addQObject<network_manager::NFSMountModel>();

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

    observe(
        runtime::selectors::select_nfs_server_enabled,
        [this](bool enabled) {
            setNfsServerEnabled(enabled);
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

    if (m_impl->nfsClient)
    {
        m_impl->nfsClient->set_mount_status_callback(
            [this](
                std::string const& id,
                network_manager::nfs_mount_status status) {
                auto state = m_impl->nfsClient->get_mount_state(id);
                m_impl->nfsMountModel->updateState(id, state);
                emit nfsMountStatusChanged(
                    QString::fromStdString(id),
                    static_cast<int>(status));
            });

        m_impl->nfsClient->set_mount_error_callback(
            [this](
                std::string const& id,
                std::string const& error) {
                auto state = m_impl->nfsClient->get_mount_state(id);
                m_impl->nfsMountModel->updateState(id, state);
                emit nfsMountError(
                    QString::fromStdString(id),
                    QString::fromStdString(error));
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
    setNfsMountCommand(
        QString::fromStdString(m_impl->nfsServer->mount_command()));

    auto const& config = m_impl->nfsServer->export_config();
    setNfsServerReadWrite(config.read_write);

    QStringList ips;
    for (auto const& ip : m_impl->nfsServer->local_ip_addresses())
    {
        ips.append(QString::fromStdString(ip));
    }
    setNfsLocalIpAddresses(ips);
}

void
NetworkSettings::updateNfsClientStatus()
{
    if (!m_impl->nfsClient)
    {
        setNfsClientAvailable(false);
        return;
    }

    setNfsClientAvailable(m_impl->nfsClient->is_available());

    auto mounts = m_impl->nfsClient->saved_mounts();
    m_impl->nfsMountModel->setMounts(std::move(mounts));

    m_impl->nfsClient->refresh_mount_states();
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
    }
}

void
NetworkSettings::disableNfsServer()
{
    if (m_impl->nfsServer)
    {
        m_impl->nfsServer->disable();
    }
}

void
NetworkSettings::toggleNfsServer()
{
    if (m_impl->nfsServer)
    {
        if (m_impl->nfsServer->is_active())
        {
            m_impl->nfsServer->disable();
        }
        else
        {
            m_impl->nfsServer->enable();
        }

        updateNfsServerStatus();
    }
}

void
NetworkSettings::setNfsServerReadWriteMode(bool readWrite)
{
    if (m_impl->nfsServer)
    {
        m_impl->nfsServer->set_read_write(readWrite);
        setNfsServerReadWrite(readWrite);
        setNfsMountCommand(
            QString::fromStdString(m_impl->nfsServer->mount_command()));
    }
}

bool
NetworkSettings::testNfsServer()
{
    if (m_impl->nfsServer)
    {
        return m_impl->nfsServer->test_connection();
    }
    return false;
}

QObject*
NetworkSettings::nfsMounts() const
{
    return m_impl->nfsMountModel;
}

void
NetworkSettings::addNfsMount(
    QString const& name,
    QString const& serverHost,
    QString const& remotePath,
    QString const& localMount,
    bool nfsv4,
    bool readWrite,
    bool autoMount)
{
    if (!m_impl->nfsClient)
        return;

    network_manager::nfs_mount_config config;
    config.name = name.toStdString();
    config.server_host = serverHost.toStdString();
    config.remote_path = remotePath.toStdString();
    config.local_mount = localMount.toStdString();
    config.version = nfsv4 ? network_manager::nfs_version::v4
                           : network_manager::nfs_version::v3;
    config.read_write = readWrite;
    config.auto_mount = autoMount;

    m_impl->nfsClient->add_mount_config(std::move(config));

    auto mounts = m_impl->nfsClient->saved_mounts();
    m_impl->nfsMountModel->setMounts(std::move(mounts));
}

void
NetworkSettings::updateNfsMount(
    QString const& id,
    QString const& name,
    QString const& serverHost,
    QString const& remotePath,
    QString const& localMount,
    bool nfsv4,
    bool readWrite,
    bool autoMount)
{
    if (!m_impl->nfsClient)
        return;

    network_manager::nfs_mount_config config;
    config.id = id.toStdString();
    config.name = name.toStdString();
    config.server_host = serverHost.toStdString();
    config.remote_path = remotePath.toStdString();
    config.local_mount = localMount.toStdString();
    config.version = nfsv4 ? network_manager::nfs_version::v4
                           : network_manager::nfs_version::v3;
    config.read_write = readWrite;
    config.auto_mount = autoMount;

    m_impl->nfsClient->update_mount_config(id.toStdString(), config);

    auto mounts = m_impl->nfsClient->saved_mounts();
    m_impl->nfsMountModel->setMounts(std::move(mounts));
}

void
NetworkSettings::removeNfsMount(QString const& id)
{
    if (!m_impl->nfsClient)
        return;

    m_impl->nfsClient->remove_mount_config(id.toStdString());

    auto mounts = m_impl->nfsClient->saved_mounts();
    m_impl->nfsMountModel->setMounts(std::move(mounts));
}

void
NetworkSettings::mountNfs(QString const& id)
{
    if (m_impl->nfsClient)
    {
        m_impl->nfsClient->mount(id.toStdString());
    }
}

void
NetworkSettings::unmountNfs(QString const& id)
{
    if (m_impl->nfsClient)
    {
        m_impl->nfsClient->unmount(id.toStdString());
    }
}

void
NetworkSettings::unmountAllNfs()
{
    if (m_impl->nfsClient)
    {
        m_impl->nfsClient->unmount_all();
    }
}

bool
NetworkSettings::testNfsConnection(
    QString const& serverHost,
    QString const& remotePath)
{
    if (m_impl->nfsClient)
    {
        return m_impl->nfsClient->test_connection(
            serverHost.toStdString(),
            remotePath.toStdString());
    }
    return false;
}

QStringList
NetworkSettings::getNfsServerExports(QString const& serverHost)
{
    QStringList result;
    if (m_impl->nfsClient)
    {
        auto exports =
            m_impl->nfsClient->get_server_exports(serverHost.toStdString());
        for (auto const& exp : exports)
        {
            result.append(QString::fromStdString(exp));
        }
    }
    return result;
}

void
NetworkSettings::refreshStatus()
{
    // Show what's known now, then ask NetworkManager for fresh state; the
    // callbacks fill in the rest.
    updateWifiStatus();
    updateNfsServerStatus();
    updateNfsClientStatus();

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
