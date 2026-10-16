// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/FilePath.h>
#include <piejam/gui/model/SubscribableModel.h>
#include <piejam/gui/model/fwd.h>

#include <filesystem>

namespace piejam::gui::model
{

class FileDialog : public QObject
{
    Q_OBJECT

    PIEJAM_GUI_MODEL_PIMPL

    PIEJAM_GUI_CONSTANT_PROPERTY(QAbstractListModel*, entries)
    PIEJAM_GUI_PROPERTY(bool, canGoUp, setCanGoUp)

public:
    explicit FileDialog(
        std::filesystem::path root_dir,
        std::string file_extension_filter);

    Q_INVOKABLE void goUp();
    Q_INVOKABLE void changeDirectory(int index);
    Q_INVOKABLE void createDirectory(QString const& name);
    Q_INVOKABLE void remove(int index);

    Q_INVOKABLE piejam::gui::model::FilePath
    selectFile(QString const& name) const;

    Q_INVOKABLE void updateEntries();
};

} // namespace piejam::gui::model
