// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/network_manager/nfs_server.h>

#include <QHostAddress>
#include <QNetworkInterface>
#include <QProcess>

#include <optional>

namespace piejam::network_manager
{

namespace
{

// The validated root helper that applies the export (see provisioning). It
// fixes the export path and checks the subnet, so the app only ever hands it
// a computed CIDR and an access mode.
constexpr char const* nfs_export_helper = "/usr/local/sbin/piejam-nfs-export";

// The systemd unit for the kernel NFS server on Raspberry Pi OS.
constexpr char const* nfs_unit = "nfs-server.service";

struct process_result
{
    bool started{};
    int exit_code{-1};
    QString std_err;
};

// Run a command with an explicit argument list and no shell. User input is
// never part of the program name or interpreted by a shell.
auto
run(QString const& program, QStringList const& args, int timeout_ms = 15000)
    -> process_result
{
    QProcess proc;
    proc.start(program, args);
    if (!proc.waitForStarted(5000))
    {
        return {.started = false, .exit_code = -1, .std_err = {}};
    }
    if (!proc.waitForFinished(timeout_ms))
    {
        proc.kill();
        proc.waitForFinished(2000);
        return {.started = true, .exit_code = -1, .std_err = {}};
    }
    return {
        .started = true,
        .exit_code = proc.exitCode(),
        .std_err = QString::fromUtf8(proc.readAllStandardError()).trimmed()};
}

} // namespace

struct nfs_server::impl
{
    nfs_export_config config;
    nfs_server_status status{nfs_server_status::inactive};

    nfs_server_status_callback status_callback;
    nfs_server_error_callback error_callback;

    void notify_status(nfs_server_status s)
    {
        status = s;
        if (status_callback)
            status_callback(s);
    }

    void notify_error(std::string const& err)
    {
        if (error_callback)
            error_callback(err);
    }

    // systemctl start|stop nfs-server, authorized for the piejam user by a
    // polkit rule limited to this unit. No "enable": never a boot dependency.
    bool systemctl(QString const& action)
    {
        auto const r = run("systemctl", {action, nfs_unit});
        if (!r.started || r.exit_code != 0)
        {
            notify_error(
                "NFS server " + action.toStdString() + " failed" +
                (r.std_err.isEmpty() ? "" : ": " + r.std_err.toStdString()));
            return false;
        }
        return true;
    }

    // Apply or clear the export through the validated root helper.
    bool run_helper(QStringList const& helper_args)
    {
        QStringList args;
        args << "-n" << nfs_export_helper << helper_args;
        auto const r = run("sudo", args);
        if (!r.started || r.exit_code != 0)
        {
            notify_error(
                "NFS export failed" +
                (r.std_err.isEmpty() ? "" : ": " + r.std_err.toStdString()));
            return false;
        }
        return true;
    }

    [[nodiscard]] bool service_active() const
    {
        auto const r = run("systemctl", {"is-active", nfs_unit}, 5000);
        return r.started && r.exit_code == 0;
    }
};

nfs_server::nfs_server(std::string export_path)
    : m_impl(std::make_unique<impl>())
{
    m_impl->config.export_path = std::move(export_path);
    m_impl->config.read_write = true;

    refresh_status();
}

nfs_server::~nfs_server() = default;

nfs_server::nfs_server(nfs_server&&) noexcept = default;
nfs_server& nfs_server::operator=(nfs_server&&) noexcept = default;

auto
nfs_server::export_path() const noexcept -> std::string const&
{
    return m_impl->config.export_path;
}

auto
nfs_server::status() const noexcept -> nfs_server_status
{
    return m_impl->status;
}

auto
nfs_server::is_active() const noexcept -> bool
{
    return m_impl->status == nfs_server_status::active;
}

auto
nfs_server::is_available() const -> bool
{
    // exportfs ships with nfs-kernel-server; its presence means the server
    // tools are installed.
    auto const r = run("which", {"exportfs"}, 5000);
    return r.started && r.exit_code == 0;
}

auto
nfs_server::export_config() const noexcept -> nfs_export_config const&
{
    return m_impl->config;
}

auto
nfs_server::local_ip_addresses() const -> std::vector<std::string>
{
    std::vector<std::string> addresses;

    for (auto const& iface : QNetworkInterface::allInterfaces())
    {
        auto const flags = iface.flags();
        if (!flags.testFlag(QNetworkInterface::IsUp) ||
            !flags.testFlag(QNetworkInterface::IsRunning) ||
            flags.testFlag(QNetworkInterface::IsLoopBack))
        {
            continue;
        }

        for (auto const& entry : iface.addressEntries())
        {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
            {
                addresses.push_back(entry.ip().toString().toStdString());
            }
        }
    }

    return addresses;
}

namespace
{

// The CIDR of the private subnet the Pi is currently on, host bits masked
// off (e.g. "192.168.30.0/24"). nullopt if there's no usable address yet.
auto
current_subnet_cidr() -> std::optional<std::string>
{
    for (auto const& iface : QNetworkInterface::allInterfaces())
    {
        auto const flags = iface.flags();
        if (!flags.testFlag(QNetworkInterface::IsUp) ||
            !flags.testFlag(QNetworkInterface::IsRunning) ||
            flags.testFlag(QNetworkInterface::IsLoopBack))
        {
            continue;
        }

        for (auto const& entry : iface.addressEntries())
        {
            if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol)
                continue;

            auto const ip = entry.ip().toString().toStdString();
            auto const octets = parse_ipv4(ip);
            if (!octets || !is_private_ipv4(*octets))
                continue;

            if (auto cidr = ipv4_network_cidr(ip, entry.prefixLength()))
                return cidr;
        }
    }

    return std::nullopt;
}

} // namespace

bool
nfs_server::enable()
{
    if (!is_available())
    {
        m_impl->notify_error("NFS server packages not installed");
        return false;
    }

    auto const cidr = current_subnet_cidr();
    if (!cidr)
    {
        m_impl->notify_error("no local network to share on");
        return false;
    }

    m_impl->notify_status(nfs_server_status::starting);

    // Start the daemon first so the export is served as soon as it's added.
    if (!m_impl->systemctl("start"))
    {
        m_impl->notify_status(nfs_server_status::error);
        return false;
    }

    // Share read-write to the current subnet. The subnet is recomputed on
    // every enable, so moving the Pi to another network just works.
    if (!m_impl->run_helper({"export", QString::fromStdString(*cidr), "rw"}))
    {
        m_impl->systemctl("stop");
        m_impl->notify_status(nfs_server_status::error);
        return false;
    }

    m_impl->config.allowed_hosts = *cidr;
    m_impl->config.read_write = true;

    refresh_status();
    return is_active();
}

bool
nfs_server::disable()
{
    m_impl->notify_status(nfs_server_status::stopping);

    // Stop the daemon first, then unexport and drop the bind mount: a bind
    // mount that nfsd still holds can't be unmounted.
    m_impl->systemctl("stop");
    m_impl->run_helper({"unexport"});

    m_impl->config.allowed_hosts.clear();

    refresh_status();
    return !is_active();
}

void
nfs_server::refresh_status()
{
    m_impl->notify_status(
        m_impl->service_active() ? nfs_server_status::active
                                 : nfs_server_status::inactive);
}

void
nfs_server::set_status_callback(nfs_server_status_callback cb)
{
    m_impl->status_callback = std::move(cb);
}

void
nfs_server::set_error_callback(nfs_server_error_callback cb)
{
    m_impl->error_callback = std::move(cb);
}

} // namespace piejam::network_manager
