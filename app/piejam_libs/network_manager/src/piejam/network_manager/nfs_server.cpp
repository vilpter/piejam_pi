// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/network_manager/nfs_server.h>

#include <QFile>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QProcess>
#include <QTextStream>

#include <fstream>
#include <regex>

namespace piejam::network_manager
{

struct nfs_server::impl
{
    std::string export_path;
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
};

nfs_server::nfs_server(std::string export_path)
    : m_impl(std::make_unique<impl>())
{
    m_impl->export_path = std::move(export_path);
    m_impl->config.export_path = m_impl->export_path;
    m_impl->config.allowed_hosts = "192.168.0.0/16";
    m_impl->config.read_write = true;
    m_impl->config.sync = true;
    m_impl->config.no_subtree_check = true;
    m_impl->config.no_root_squash = false;

    refresh_status();
}

nfs_server::~nfs_server() = default;

nfs_server::nfs_server(nfs_server&&) noexcept = default;
nfs_server& nfs_server::operator=(nfs_server&&) noexcept = default;

auto
nfs_server::export_path() const noexcept -> std::string const&
{
    return m_impl->export_path;
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
    // Check if nfs-kernel-server is installed
    QProcess proc;
    proc.start("which", {"exportfs"});
    proc.waitForFinished(5000);
    return proc.exitCode() == 0;
}

auto
nfs_server::export_config() const noexcept -> nfs_export_config const&
{
    return m_impl->config;
}

auto
nfs_server::mount_command() const -> std::string
{
    auto ips = local_ip_addresses();
    std::string ip = ips.empty() ? "<pi_ip>" : ips.front();

    return "mount -t nfs " + ip + ":" + m_impl->export_path +
           " /mnt/piejam_recordings";
}

auto
nfs_server::local_ip_addresses() const -> std::vector<std::string>
{
    std::vector<std::string> addresses;

    for (auto const& iface : QNetworkInterface::allInterfaces())
    {
        if (iface.flags().testFlag(QNetworkInterface::IsUp) &&
            iface.flags().testFlag(QNetworkInterface::IsRunning) &&
            !iface.flags().testFlag(QNetworkInterface::IsLoopBack))
        {
            for (auto const& entry : iface.addressEntries())
            {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
                {
                    addresses.push_back(entry.ip().toString().toStdString());
                }
            }
        }
    }

    return addresses;
}

bool
nfs_server::enable()
{
    if (!is_available())
    {
        m_impl->notify_error("NFS server packages not installed");
        return false;
    }

    m_impl->notify_status(nfs_server_status::starting);

    // Update exports file first
    update_exports_file();

    // Start NFS server
    execute_systemctl("start");

    // Also enable auto-start
    execute_systemctl("enable");

    // Refresh exports
    QProcess proc;
    proc.start("exportfs", {"-ra"});
    proc.waitForFinished(5000);

    refresh_status();
    return m_impl->status == nfs_server_status::active;
}

bool
nfs_server::disable()
{
    m_impl->notify_status(nfs_server_status::stopping);

    execute_systemctl("stop");

    // Optionally disable auto-start
    // execute_systemctl("disable");

    refresh_status();
    return m_impl->status == nfs_server_status::inactive;
}

bool
nfs_server::restart()
{
    m_impl->notify_status(nfs_server_status::starting);

    update_exports_file();
    execute_systemctl("restart");

    // Refresh exports
    QProcess proc;
    proc.start("exportfs", {"-ra"});
    proc.waitForFinished(5000);

    refresh_status();
    return m_impl->status == nfs_server_status::active;
}

void
nfs_server::set_export_config(nfs_export_config const& config)
{
    m_impl->config = config;
    m_impl->config.export_path = m_impl->export_path; // Keep original export path

    if (is_active())
    {
        restart();
    }
}

void
nfs_server::set_read_write(bool read_write)
{
    m_impl->config.read_write = read_write;

    if (is_active())
    {
        update_exports_file();

        QProcess proc;
        proc.start("exportfs", {"-ra"});
        proc.waitForFinished(5000);
    }
}

void
nfs_server::set_allowed_hosts(std::string const& hosts)
{
    m_impl->config.allowed_hosts = hosts;

    if (is_active())
    {
        update_exports_file();

        QProcess proc;
        proc.start("exportfs", {"-ra"});
        proc.waitForFinished(5000);
    }
}

auto
nfs_server::test_connection() -> bool
{
    // Try to use showmount to verify exports are visible
    QProcess proc;
    proc.start("showmount", {"-e", "localhost"});
    proc.waitForFinished(5000);

    if (proc.exitCode() != 0)
    {
        return false;
    }

    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    return output.contains(QString::fromStdString(m_impl->export_path));
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

void
nfs_server::update_exports_file()
{
    // Generate new export line
    std::string export_line = generate_export_line(m_impl->config);

    // Read current exports file
    std::string exports_path = "/etc/exports";
    std::ifstream infile(exports_path);
    std::string content;
    std::string line;
    bool found = false;

    while (std::getline(infile, line))
    {
        // Check if this line is our export (starts with our path)
        if (line.find(m_impl->export_path) == 0)
        {
            // Replace with new config
            content += export_line + "\n";
            found = true;
        }
        else
        {
            content += line + "\n";
        }
    }
    infile.close();

    // If export wasn't found, append it
    if (!found)
    {
        content += "\n# PieJam recordings export\n";
        content += export_line + "\n";
    }

    // Write back
    // Note: This requires root privileges. In practice, you'd use a helper
    // or polkit
    std::ofstream outfile(exports_path);
    if (outfile.is_open())
    {
        outfile << content;
        outfile.close();
    }
    else
    {
        m_impl->notify_error(
            "Failed to write /etc/exports - permission denied");
    }
}

void
nfs_server::execute_systemctl(std::string const& action)
{
    // Buildroot uses busybox init scripts instead of systemd.
    // Try init script first, then fall back to direct daemon management.
    QProcess proc;

    // Try buildroot-style init script
    static char const* const init_scripts[] = {
        "/etc/init.d/S60nfs",
        "/etc/init.d/nfs",
    };

    for (auto const* script : init_scripts)
    {
        if (QFile::exists(script))
        {
            proc.start(script, {QString::fromStdString(action)});
            proc.waitForFinished(10000);
            if (proc.exitCode() == 0)
                return;
        }
    }

    // Fallback: try systemctl in case it exists (e.g., if systemd is available)
    proc.start("systemctl", {QString::fromStdString(action), "nfs-kernel-server"});
    proc.waitForFinished(10000);

    if (proc.exitCode() != 0)
    {
        QString stderr_output = QString::fromUtf8(proc.readAllStandardError());
        m_impl->notify_error(
            "NFS service " + action + " failed: " + stderr_output.toStdString());
        m_impl->notify_status(nfs_server_status::error);
    }
}

void
nfs_server::refresh_status()
{
    // Check if nfsd is running by looking for the process
    QProcess proc;
    proc.start("pidof", {"nfsd"});
    proc.waitForFinished(5000);

    if (proc.exitCode() == 0)
    {
        m_impl->notify_status(nfs_server_status::active);
    }
    else
    {
        m_impl->notify_status(nfs_server_status::inactive);
    }
}

} // namespace piejam::network_manager
