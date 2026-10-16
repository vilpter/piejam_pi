// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/Filesystem.h>

#include <spdlog/spdlog.h>

namespace piejam::gui::model
{

bool
Filesystem::fileExists(piejam::gui::model::FilePath const& file) const
{
    try
    {
        return std::filesystem::exists(file.path);
    }
    catch (std::filesystem::filesystem_error const& err)
    {
        spdlog::error(
            "Filesystem::fileExists() - error accessing {}: {}",
            file.path.string(),
            err.what());

        return false;
    }
}

bool
Filesystem::isRegularFile(piejam::gui::model::FilePath const& file) const
{
    try
    {
        return std::filesystem::is_regular_file(file.path);
    }
    catch (std::filesystem::filesystem_error const& err)
    {
        spdlog::error(
            "Filesystem::isRegularFile() - error accessing {}: {}",
            file.path.string(),
            err.what());

        return false;
    }
}

bool
Filesystem::isDirectory(piejam::gui::model::FilePath const& file) const
{
    try
    {
        return std::filesystem::is_directory(file.path);
    }
    catch (std::filesystem::filesystem_error const& err)
    {
        spdlog::error(
            "Filesystem::isDirectory() - error accessing {}: {}",
            file.path.string(),
            err.what());

        return false;
    }
}

auto
filesystemSingleton() -> Filesystem&
{
    static Filesystem s_instance;
    return s_instance;
}

} // namespace piejam::gui::model
