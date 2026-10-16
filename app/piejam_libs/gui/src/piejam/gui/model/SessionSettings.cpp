// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/SessionSettings.h>

#include <piejam/gui/model/FileDialog.h>

#include <piejam/enum.h>
#include <piejam/runtime/actions/session_actions.h>
#include <piejam/runtime/selectors.h>

#include <functional>

namespace piejam::gui::model
{

namespace
{

constexpr auto session_ext = std::string{".pjs"};

auto
sessionFileToQString(std::filesystem::path s) -> QString
{
    if (s.extension() == session_ext)
    {
        s.replace_extension();
    }

    return QString::fromStdString(s.string());
}

} // namespace

struct SessionSettings::Impl
{
    std::filesystem::path sessions_root_dir;
    std::filesystem::path current_session{};
};

SessionSettings::SessionSettings(
    runtime::state_access const& state_access,
    std::filesystem::path sessions_root_dir)
    : CompositeSubscribableModel{state_access}
    , m_impl{make_pimpl<Impl>(sessions_root_dir)}
    , m_sessionFileDialog{
          &addQObject<FileDialog>(std::move(sessions_root_dir), session_ext)}
{
}

void
SessionSettings::onSubscribe()
{
    observe(
        runtime::selectors::select_startup_session,
        [this](runtime::startup_session const startup_session) {
            setStartupSession(bool_enum_to<StartupSession>(startup_session));
        });

    observe(
        runtime::selectors::select_current_session,
        [this](std::filesystem::path const& current_session) {
            setCurrentSession(sessionFileToQString(current_session));
            m_impl->current_session = current_session;
        });

    observe(
        runtime::selectors::select_session_modified,
        std::bind_front(&SessionSettings::setSessionModified, this));
}

void
SessionSettings::newSession()
{
    dispatch(runtime::actions::new_session{});
}

void
SessionSettings::setTemplateFromSession(FilePath const& sessionFile)
{
    runtime::actions::set_session_template action;
    action.session_template = sessionFile.path;
    dispatch(action);
}

void
SessionSettings::setTemplateFromCurrent()
{
    runtime::actions::set_session_template action;
    action.session_template =
        runtime::actions::set_session_template::current_session{};
    dispatch(action);
}

void
SessionSettings::setEmptySessionTemplate()
{
    runtime::actions::set_session_template action;
    action.session_template =
        runtime::actions::set_session_template::empty_session{};
    dispatch(action);
}

void
SessionSettings::openSession(piejam::gui::model::FilePath const& file)
{
    dispatch(runtime::actions::load_session{file.path});
}

void
SessionSettings::saveCurrentSession()
{
    BOOST_ASSERT(!m_impl->current_session.empty());
    dispatch(
        runtime::actions::save_session{
            m_impl->sessions_root_dir / m_impl->current_session});
}

void
SessionSettings::saveSession(piejam::gui::model::FilePath const& file)
{
    dispatch(runtime::actions::save_session{file.path});
}

void
SessionSettings::switchStartupSession(StartupSession startupSession)
{
    runtime::actions::switch_startup_session action;
    action.startup_session =
        bool_enum_to<runtime::startup_session>(startupSession);
    dispatch(action);
}

} // namespace piejam::gui::model
