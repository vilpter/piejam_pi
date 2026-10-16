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

class SessionSettings final : public CompositeSubscribableModel
{
    Q_OBJECT

    PIEJAM_GUI_MODEL_PIMPL

public:
    enum class NewSessionType : bool
    {
        Empty,
        Template,
    };

    Q_ENUM(NewSessionType)

    enum class StartupSession : bool
    {
        New,
        Last,
    };

    Q_ENUM(StartupSession)

private:
    PIEJAM_GUI_PROPERTY(NewSessionType, newSessionType, setRotation)
    PIEJAM_GUI_PROPERTY(QString, currentSession, setCurrentSession)
    PIEJAM_GUI_PROPERTY(StartupSession, startupSession, setStartupSession)
    PIEJAM_GUI_CONSTANT_PROPERTY(
        piejam::gui::model::FileDialog*,
        sessionFileDialog)
    PIEJAM_GUI_PROPERTY(bool, isSessionModified, setSessionModified)

public:
    explicit SessionSettings(
        runtime::state_access const&,
        std::filesystem::path sessions_root_dir);

    Q_INVOKABLE void newSession();
    Q_INVOKABLE void
    setTemplateFromSession(piejam::gui::model::FilePath const&);
    Q_INVOKABLE void setTemplateFromCurrent();
    Q_INVOKABLE void setEmptySessionTemplate();

    Q_INVOKABLE void saveCurrentSession();
    Q_INVOKABLE void openSession(piejam::gui::model::FilePath const&);
    Q_INVOKABLE void saveSession(piejam::gui::model::FilePath const&);

    Q_INVOKABLE void switchStartupSession(StartupSession);

private:
    void onSubscribe() override;
};

} // namespace piejam::gui::model
