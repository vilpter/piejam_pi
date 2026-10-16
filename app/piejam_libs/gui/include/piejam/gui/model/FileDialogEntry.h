// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

#include <filesystem>

namespace piejam::gui::model
{

class FileDialogEntry
{
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(Type type MEMBER type)

public:
    FileDialogEntry() = default;
    explicit FileDialogEntry(std::filesystem::path path)
        : path{std::move(path)}
        , type{typeFromPath(this->path)}
        , name{
              type == Type::Directory
                  ? QString::fromStdString(this->path.filename().string())
                  : QString::fromStdString(this->path.stem().string())}
    {
    }

    std::filesystem::path path;

    enum class Type
    {
        File,
        Directory
    };

    Q_ENUM(Type)

    Type type{Type::File};
    QString name{};

    constexpr auto operator==(FileDialogEntry const&) const noexcept
        -> bool = default;

private:
    static auto typeFromPath(std::filesystem::path const& path) -> Type
    {
        std::error_code ec;
        return std::filesystem::is_directory(path, ec) && !ec ? Type::Directory
                                                              : Type::File;
    }
};

} // namespace piejam::gui::model

Q_DECLARE_METATYPE(piejam::gui::model::FileDialogEntry)
