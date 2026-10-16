// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/FileDialog.h>

#include <piejam/gui/ListModelEditScriptProcessor.h>
#include <piejam/gui/model/FileDialogEntry.h>
#include <piejam/gui/model/ValueListModel.h>

#include <spdlog/spdlog.h>

namespace piejam::gui::model
{

namespace
{

struct FileDialogEntryList : public ValueListModel<FileDialogEntry>
{
    using ValueListModel<FileDialogEntry>::ValueListModel;

    auto itemToString(FileDialogEntry const& entry) const -> QString override
    {
        return entry.name;
    }
};

} // namespace

struct FileDialog::Impl
{
    std::filesystem::path root_dir;
    std::string file_extension_filter;

    std::filesystem::path current_dir{root_dir};
    std::vector<std::filesystem::path> entries{};

    FileDialogEntryList entriesList{};
};

FileDialog::FileDialog(
    std::filesystem::path root_dir,
    std::string file_extension_filter)
    : m_impl{make_pimpl<Impl>(
          std::move(root_dir),
          std::move(file_extension_filter))}
    , m_entries{&m_impl->entriesList}
{
}

void
FileDialog::goUp()
{
    BOOST_ASSERT(m_canGoUp);

    m_impl->current_dir = m_impl->current_dir.parent_path();
    updateEntries();
}

void
FileDialog::changeDirectory(int index)
{
    BOOST_ASSERT(
        0 <= index && static_cast<size_t>(index) < m_impl->entries.size());

    auto const& dir_path = m_impl->entries[static_cast<size_t>(index)];

    try
    {
        if (std::filesystem::is_directory(dir_path))
        {
            m_impl->current_dir = dir_path;
        }

        updateEntries();
    }
    catch (std::filesystem::filesystem_error const& err)
    {
        spdlog::error(
            "FileDialog::changeDirectory() - error accessing {}: {}",
            dir_path.string(),
            err.what());
    }
}

void
FileDialog::createDirectory(QString const& name)
{
    BOOST_ASSERT(!name.isEmpty());

    auto dir_path = m_impl->current_dir / name.toStdString();

    try
    {
        std::filesystem::create_directory(dir_path);
    }
    catch (std::filesystem::filesystem_error const& err)
    {
        spdlog::error(
            "FileDialog::createDirectory() - error creating directory {}: {}",
            dir_path.string(),
            err.what());
    }

    updateEntries();
}

void
FileDialog::remove(int index)
{
    BOOST_ASSERT(
        0 <= index && static_cast<size_t>(index) < m_impl->entries.size());

    auto const& path = m_impl->entries[static_cast<size_t>(index)];

    try
    {
        std::filesystem::remove_all(path);
    }
    catch (std::filesystem::filesystem_error const& err)
    {
        spdlog::error(
            "FileDialog::remove() - error removing {}: {}",
            path.string(),
            err.what());
    }

    updateEntries();
}

auto
FileDialog::selectFile(QString const& name) const
    -> piejam::gui::model::FilePath
{
    BOOST_ASSERT(!name.isEmpty());

    auto file_path = m_impl->current_dir / name.toStdString();
    file_path.replace_extension(m_impl->file_extension_filter);
    return FilePath{std::move(file_path)};
}

void
FileDialog::updateEntries()
{
    std::vector<std::filesystem::path> entries;
    std::error_code ec;

    if (!std::filesystem::exists(m_impl->current_dir, ec))
    {
        m_impl->current_dir = m_impl->root_dir;
    }

    setCanGoUp(m_impl->current_dir != m_impl->root_dir);

    if (std::filesystem::exists(m_impl->current_dir, ec))
    {
        BOOST_ASSERT(!ec);

        for (auto const& entry :
             std::filesystem::directory_iterator(m_impl->current_dir, ec))
        {
            if ((!ec && entry.is_directory(ec)) ||
                (!ec && entry.is_regular_file(ec) && !ec &&
                 entry.path().extension().string().ends_with(
                     m_impl->file_extension_filter)))
            {
                entries.push_back(entry.path());
            }
        }

        std::ranges::sort(entries);
    }
    else if (ec)
    {
        spdlog::error(
            "FileDialog: error accessing directory {}: {}",
            m_impl->current_dir.string(),
            ec.message());
    }

    algorithm::apply_edit_script(
        algorithm::edit_script(m_impl->entries, entries),
        ListModelEditScriptProcessor{
            m_impl->entriesList,
            [](std::filesystem::path const& path) {
                return FileDialogEntry{path};
            }});
    m_impl->entries = std::move(entries);
}

} // namespace piejam::gui::model
