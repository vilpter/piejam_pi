// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/network_manager/nfs_client.h>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStorageInfo>
#include <QUuid>

#include <chrono>
#include <fstream>
#include <map>

namespace piejam::network_manager
{

struct nfs_client::impl
{
    std::string config_path;
    std::map<std::string, nfs_mount_config> configs;
    std::map<std::string, nfs_mount_state> states;

    nfs_mount_status_callback status_callback;
    nfs_mount_error_callback error_callback;

    void notify_status(std::string const& id, nfs_mount_status status)
    {
        states[id].status = status;
        if (status_callback)
            status_callback(id, status);
    }

    void notify_error(std::string const& id, std::string const& error)
    {
        states[id].error_message = error;
        states[id].status = nfs_mount_status::error;
        if (error_callback)
            error_callback(id, error);
    }
};

nfs_client::nfs_client(std::string config_path)
    : m_impl(std::make_unique<impl>())
{
    m_impl->config_path = std::move(config_path);
    load_config();
    refresh_mount_states();
}

nfs_client::~nfs_client() = default;

nfs_client::nfs_client(nfs_client&&) noexcept = default;
nfs_client& nfs_client::operator=(nfs_client&&) noexcept = default;

auto
nfs_client::is_available() const -> bool
{
    QProcess proc;
    proc.start("which", {"mount.nfs"});
    proc.waitForFinished(5000);
    return proc.exitCode() == 0;
}

auto
nfs_client::saved_mounts() const -> std::vector<nfs_mount_config>
{
    std::vector<nfs_mount_config> result;
    result.reserve(m_impl->configs.size());

    for (auto const& [id, config] : m_impl->configs)
    {
        result.push_back(config);
    }

    return result;
}

auto
nfs_client::get_mount_config(std::string const& id) const
    -> nfs_mount_config const*
{
    auto it = m_impl->configs.find(id);
    if (it != m_impl->configs.end())
    {
        return &it->second;
    }
    return nullptr;
}

auto
nfs_client::add_mount_config(nfs_mount_config config) -> std::string
{
    // Generate UUID if not provided
    if (config.id.empty())
    {
        config.id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    }

    // Generate mount point if not provided
    if (config.local_mount.empty())
    {
        config.local_mount = generate_mount_point(config.name);
    }

    m_impl->configs[config.id] = config;
    m_impl->states[config.id] = nfs_mount_state{config.id, nfs_mount_status::unmounted, {}, 0, 0, -1};

    save_config();

    return config.id;
}

bool
nfs_client::update_mount_config(
    std::string const& id,
    nfs_mount_config const& config)
{
    auto it = m_impl->configs.find(id);
    if (it == m_impl->configs.end())
    {
        return false;
    }

    // Don't allow changing ID
    nfs_mount_config updated = config;
    updated.id = id;

    // If mount point changed and currently mounted, need to unmount first
    if (it->second.local_mount != updated.local_mount && is_mounted(id))
    {
        unmount(id);
    }

    m_impl->configs[id] = updated;
    save_config();

    return true;
}

bool
nfs_client::remove_mount_config(std::string const& id)
{
    auto it = m_impl->configs.find(id);
    if (it == m_impl->configs.end())
    {
        return false;
    }

    // Unmount if currently mounted
    if (is_mounted(id))
    {
        unmount(id);
    }

    m_impl->configs.erase(it);
    m_impl->states.erase(id);
    save_config();

    return true;
}

auto
nfs_client::get_mount_state(std::string const& id) const -> nfs_mount_state
{
    auto it = m_impl->states.find(id);
    if (it != m_impl->states.end())
    {
        return it->second;
    }
    return nfs_mount_state{id, nfs_mount_status::unmounted, {}, 0, 0, -1};
}

auto
nfs_client::is_mounted(std::string const& id) const -> bool
{
    auto it = m_impl->states.find(id);
    if (it == m_impl->states.end())
    {
        return false;
    }
    return it->second.status == nfs_mount_status::mounted;
}

bool
nfs_client::mount(std::string const& id)
{
    auto it = m_impl->configs.find(id);
    if (it == m_impl->configs.end())
    {
        return false;
    }

    if (is_mounted(id))
    {
        return true; // Already mounted
    }

    m_impl->notify_status(id, nfs_mount_status::mounting);

    // Create mount point if needed
    if (!create_mount_point(it->second.local_mount))
    {
        m_impl->notify_error(id, "Failed to create mount point");
        return false;
    }

    // Execute mount
    if (!execute_mount(it->second))
    {
        return false;
    }

    // Update state
    m_impl->notify_status(id, nfs_mount_status::mounted);

    // Update space info
    auto& state = m_impl->states[id];
    state.available_space = get_available_space(id);
    state.total_space = get_total_space(id);

    return true;
}

bool
nfs_client::unmount(std::string const& id)
{
    auto it = m_impl->configs.find(id);
    if (it == m_impl->configs.end())
    {
        return false;
    }

    if (!is_mounted(id))
    {
        return true; // Already unmounted
    }

    m_impl->notify_status(id, nfs_mount_status::unmounting);

    if (!execute_unmount(it->second.local_mount))
    {
        return false;
    }

    m_impl->notify_status(id, nfs_mount_status::unmounted);
    m_impl->states[id].available_space = 0;
    m_impl->states[id].total_space = 0;

    return true;
}

void
nfs_client::unmount_all()
{
    for (auto const& [id, config] : m_impl->configs)
    {
        if (is_mounted(id))
        {
            unmount(id);
        }
    }
}

void
nfs_client::mount_auto_mounts()
{
    for (auto const& [id, config] : m_impl->configs)
    {
        if (config.auto_mount && !is_mounted(id))
        {
            mount(id);
        }
    }
}

auto
nfs_client::test_connection(
    std::string const& server_host,
    std::string const& remote_path) const -> bool
{
    QProcess proc;
    proc.start("showmount", {"-e", QString::fromStdString(server_host)});
    proc.waitForFinished(10000);

    if (proc.exitCode() != 0)
    {
        return false;
    }

    if (remote_path.empty())
    {
        return true; // Just check server is reachable
    }

    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    return output.contains(QString::fromStdString(remote_path));
}

auto
nfs_client::get_server_exports(std::string const& server_host) const
    -> std::vector<std::string>
{
    std::vector<std::string> exports;

    QProcess proc;
    proc.start("showmount", {"-e", QString::fromStdString(server_host)});
    proc.waitForFinished(10000);

    if (proc.exitCode() != 0)
    {
        return exports;
    }

    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    // Skip header line
    for (int i = 1; i < lines.size(); ++i)
    {
        QString line = lines[i].trimmed();
        // Format: "/export/path host(s)"
        int space_pos = line.indexOf(' ');
        if (space_pos > 0)
        {
            exports.push_back(line.left(space_pos).toStdString());
        }
        else if (!line.isEmpty())
        {
            exports.push_back(line.toStdString());
        }
    }

    return exports;
}

auto
nfs_client::measure_latency(std::string const& id) const -> int
{
    auto it = m_impl->configs.find(id);
    if (it == m_impl->configs.end() || !is_mounted(id))
    {
        return -1;
    }

    // Measure time to stat the mount point
    auto start = std::chrono::steady_clock::now();

    QStorageInfo info(QString::fromStdString(it->second.local_mount));
    if (!info.isValid())
    {
        return -1;
    }

    auto end = std::chrono::steady_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    return static_cast<int>(duration.count());
}

auto
nfs_client::get_available_space(std::string const& id) const -> uint64_t
{
    auto it = m_impl->configs.find(id);
    if (it == m_impl->configs.end() || !is_mounted(id))
    {
        return 0;
    }

    QStorageInfo info(QString::fromStdString(it->second.local_mount));
    return info.isValid() ? static_cast<uint64_t>(info.bytesAvailable()) : 0;
}

auto
nfs_client::get_total_space(std::string const& id) const -> uint64_t
{
    auto it = m_impl->configs.find(id);
    if (it == m_impl->configs.end() || !is_mounted(id))
    {
        return 0;
    }

    QStorageInfo info(QString::fromStdString(it->second.local_mount));
    return info.isValid() ? static_cast<uint64_t>(info.bytesTotal()) : 0;
}

void
nfs_client::set_mount_status_callback(nfs_mount_status_callback cb)
{
    m_impl->status_callback = std::move(cb);
}

void
nfs_client::set_mount_error_callback(nfs_mount_error_callback cb)
{
    m_impl->error_callback = std::move(cb);
}

auto
nfs_client::generate_mount_point(std::string const& name) const -> std::string
{
    // Sanitize name for filesystem
    QString sanitized = QString::fromStdString(name);
    sanitized = sanitized.replace(QRegularExpression("[^a-zA-Z0-9_-]"), "_");
    sanitized = sanitized.toLower();

    if (sanitized.isEmpty())
    {
        sanitized = "nfs_mount";
    }

    return std::string(default_mount_base) + "/" + sanitized.toStdString();
}

void
nfs_client::refresh_mount_states()
{
    // Read /proc/mounts to see what's actually mounted
    QFile mounts_file("/proc/mounts");
    if (!mounts_file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }

    QTextStream in(&mounts_file);
    QStringList mounted_paths;

    while (!in.atEnd())
    {
        QString line = in.readLine();
        QStringList parts = line.split(' ');
        if (parts.size() >= 3 && (parts[2] == "nfs" || parts[2] == "nfs4"))
        {
            mounted_paths.append(parts[1]); // Mount point
        }
    }

    // Update states for all configs
    for (auto& [id, state] : m_impl->states)
    {
        auto config_it = m_impl->configs.find(id);
        if (config_it == m_impl->configs.end())
            continue;

        QString mount_point = QString::fromStdString(config_it->second.local_mount);
        bool is_currently_mounted = mounted_paths.contains(mount_point);

        if (is_currently_mounted &&
            state.status != nfs_mount_status::mounted)
        {
            state.status = nfs_mount_status::mounted;
            state.available_space = get_available_space(id);
            state.total_space = get_total_space(id);
        }
        else if (!is_currently_mounted &&
                 state.status == nfs_mount_status::mounted)
        {
            state.status = nfs_mount_status::unmounted;
            state.available_space = 0;
            state.total_space = 0;
        }
    }
}

void
nfs_client::load_config()
{
    QFile file(QString::fromStdString(m_impl->config_path));
    if (!file.open(QIODevice::ReadOnly))
    {
        return; // No config file yet, that's OK
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
    {
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray mounts = root["mounts"].toArray();

    for (auto const& mount_val : mounts)
    {
        QJsonObject mount = mount_val.toObject();
        nfs_mount_config config;

        config.id = mount["id"].toString().toStdString();
        config.name = mount["name"].toString().toStdString();
        config.server_host = mount["server_host"].toString().toStdString();
        config.remote_path = mount["remote_path"].toString().toStdString();
        config.local_mount = mount["local_mount"].toString().toStdString();
        config.version =
            mount["nfsv4"].toBool(true) ? nfs_version::v4 : nfs_version::v3;
        config.read_write = mount["read_write"].toBool(true);
        config.auto_mount = mount["auto_mount"].toBool(false);
        config.timeout_decisecs = mount["timeout_decisecs"].toInt(30);
        config.retrans = mount["retrans"].toInt(3);

        if (!config.id.empty())
        {
            m_impl->configs[config.id] = config;
            m_impl->states[config.id] = nfs_mount_state{config.id, nfs_mount_status::unmounted, {}, 0, 0, -1};
        }
    }
}

void
nfs_client::save_config() const
{
    // Ensure directory exists
    QFileInfo file_info(QString::fromStdString(m_impl->config_path));
    QDir().mkpath(file_info.path());

    QJsonArray mounts;
    for (auto const& [id, config] : m_impl->configs)
    {
        QJsonObject mount;
        mount["id"] = QString::fromStdString(config.id);
        mount["name"] = QString::fromStdString(config.name);
        mount["server_host"] = QString::fromStdString(config.server_host);
        mount["remote_path"] = QString::fromStdString(config.remote_path);
        mount["local_mount"] = QString::fromStdString(config.local_mount);
        mount["nfsv4"] = (config.version == nfs_version::v4);
        mount["read_write"] = config.read_write;
        mount["auto_mount"] = config.auto_mount;
        mount["timeout_decisecs"] = config.timeout_decisecs;
        mount["retrans"] = config.retrans;
        mounts.append(mount);
    }

    QJsonObject root;
    root["mounts"] = mounts;

    QFile file(QString::fromStdString(m_impl->config_path));
    if (file.open(QIODevice::WriteOnly))
    {
        file.write(QJsonDocument(root).toJson());
    }
}

bool
nfs_client::create_mount_point(std::string const& path)
{
    QDir dir;
    if (dir.exists(QString::fromStdString(path)))
    {
        return true;
    }

    return dir.mkpath(QString::fromStdString(path));
}

bool
nfs_client::execute_mount(nfs_mount_config const& config)
{
    QStringList args;

    args << "-t" << "nfs";
    args << "-o";

    QString options = "soft";
    options += ",timeo=" + QString::number(config.timeout_decisecs);
    options += ",retrans=" + QString::number(config.retrans);
    options += config.read_write ? ",rw" : ",ro";
    options += ",nfsvers=" + QString(config.version == nfs_version::v4 ? "4" : "3");

    args << options;
    args << QString::fromStdString(config.server_host + ":" + config.remote_path);
    args << QString::fromStdString(config.local_mount);

    QProcess proc;
    proc.start("mount", args);
    proc.waitForFinished(30000);

    if (proc.exitCode() != 0)
    {
        QString error = QString::fromUtf8(proc.readAllStandardError());
        m_impl->notify_error(config.id, "Mount failed: " + error.toStdString());
        return false;
    }

    return true;
}

bool
nfs_client::execute_unmount(std::string const& mount_point)
{
    QProcess proc;
    proc.start("umount", {QString::fromStdString(mount_point)});
    proc.waitForFinished(10000);

    return proc.exitCode() == 0;
}

} // namespace piejam::network_manager
