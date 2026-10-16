// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/persistence_middleware.h>

#include <piejam/runtime/persistence/access.h>
#include <piejam/runtime/persistence/session.h>

#include <piejam/runtime/actions/apply_app_config.h>
#include <piejam/runtime/actions/apply_session.h>
#include <piejam/runtime/actions/load_app_config.h>
#include <piejam/runtime/actions/save_app_config.h>
#include <piejam/runtime/actions/session_actions.h>
#include <piejam/runtime/actions/shutdown.h>
#include <piejam/runtime/middleware_functors.h>
#include <piejam/runtime/state.h>
#include <piejam/runtime/ui/update_state_action.h>

#include <spdlog/spdlog.h>

#include <boost/assert.hpp>
#include <boost/hof/match.hpp>

#include <fstream>

namespace piejam::runtime
{

namespace
{

auto
strip_prefix(std::filesystem::path p, std::filesystem::path const& prefix)
    -> std::filesystem::path
{
    // strip prefix if applicable
    auto rel = p.lexically_relative(prefix);
    if (!rel.empty() && !rel.native().starts_with(".."))
    {
        p = rel;
    }

    return p;
}

} // namespace

persistence_middleware::persistence_middleware(
    std::filesystem::path home_dir,
    std::filesystem::path sessions_dir)
    : m_home_dir{std::move(home_dir)}
    , m_sessions_dir{std::move(sessions_dir)}
    , m_template_file{m_home_dir / "template.pjs"}
    , m_last_session_file{m_home_dir / "last.pjs"}
{
}

void
persistence_middleware::operator()(
    middleware_functors const& mw_fs,
    action const& a)
{
    if (auto action = dynamic_cast<actions::persistence_action const*>(&a))
    {
        auto v = ui::make_action_visitor<actions::persistence_action_visitor>(
            [this, &mw_fs](auto const& a) {
                process_persistence_action(mw_fs, a);
            });

        action->visit(v);
    }
    else
    {
        mw_fs.next(a);
    }
}

template <>
void
persistence_middleware::process_persistence_action(
    middleware_functors const& mw_fs,
    actions::load_app_config const& a)
{
    try
    {
        if (!std::filesystem::exists(a.file))
        {
            return;
        }

        std::ifstream in(a.file);
        if (!in.is_open())
        {
            spdlog::error("Could not open config file: {}", a.file.string());
            return;
        }

        actions::apply_app_config action;
        action.conf = persistence::load_app_config(in);
        mw_fs.dispatch(action);
    }
    catch (std::exception const& err)
    {
        spdlog::error("Could not load config file: {}", err.what());
    }
}

template <>
void
persistence_middleware::process_persistence_action(
    middleware_functors const& mw_fs,
    actions::save_app_config const& a)
{
    persistence::save_app_config(
        a.file,
        a.enabled_midi_devices,
        mw_fs.get_state());
}

template <>
void
persistence_middleware::process_persistence_action(
    middleware_functors const& mw_fs,
    actions::load_session const& a)
{
    try
    {
        if (!std::filesystem::exists(a.file))
        {
            spdlog::error("Session file doesn't exist:  {}", a.file.string());
            return;
        }

        std::ifstream in(a.file);
        if (!in.is_open())
        {
            spdlog::error("Could not open session file: {}", a.file.string());
            return;
        }

        actions::apply_session action;
        action.session = persistence::load_session(in);

        if (a.is_user_session)
        {
            action.current_session = box<std::filesystem::path>{
                strip_prefix(a.file, m_sessions_dir)};
        }

        mw_fs.dispatch(action);
    }
    catch (std::exception const& err)
    {
        spdlog::error("Could not load session file: {}", err.what());
    }
}

template <>
void
persistence_middleware::process_persistence_action(
    middleware_functors const& mw_fs,
    actions::save_session const& a)
{
    persistence::save_session(a.file, mw_fs.get_state());

    mw_fs.next(
        update_state_action{
            [session = strip_prefix(a.file, m_sessions_dir)](state& st) {
                st.current_session = box<std::filesystem::path>{session};
                st.session_modified = false;
            }});
}

template <>
void
persistence_middleware::process_persistence_action(
    middleware_functors const& mw_fs,
    actions::initiate_startup_session const&)
{
    auto const& st = mw_fs.get_state();

    try
    {
        switch (st.startup_session)
        {
            case startup_session::new_:
            {
                if (std::filesystem::exists(m_template_file))
                {
                    actions::load_session action{m_template_file};
                    action.is_user_session = false;
                    mw_fs.dispatch(action);
                }
                // in case there is no template file, just start with an empty
                // session
                break;
            }

            case startup_session::last:
            {
                if (st.current_session->empty())
                {
                    if (std::filesystem::exists(m_last_session_file))
                    {
                        actions::load_session action{m_last_session_file};
                        action.is_user_session = false;
                        mw_fs.dispatch(action);
                    }
                }
                else
                {
                    auto const session_file =
                        m_sessions_dir / st.current_session;

                    if (std::filesystem::exists(session_file))
                    {
                        mw_fs.dispatch(actions::load_session{session_file});
                    }
                    else
                    {
                        spdlog::error(
                            "Missing session file '{}'",
                            session_file.string());

                        mw_fs.next(update_state_action{[](state& st) {
                            st.current_session = box<std::filesystem::path>{};
                        }});
                    }
                }

                break;
            }
        }
    }
    catch (std::filesystem::filesystem_error const& err)
    {
        spdlog::error("Could not initiate startup session: {}", err.what());
    }
}

template <>
void
persistence_middleware::process_persistence_action(
    middleware_functors const& mw_fs,
    actions::shutdown const&)
{
    auto const& st = mw_fs.get_state();

    if (st.current_session->empty())
    {
        persistence::save_session(m_last_session_file, st);
    }
    else
    {
        auto const session_file = m_sessions_dir / st.current_session;
        persistence::save_session(session_file, st);
    }
}

template <>
void
persistence_middleware::process_persistence_action(
    middleware_functors const& mw_fs,
    actions::set_session_template const& a)
{
    auto const& st = mw_fs.get_state();

    std::visit(
        boost::hof::match(
            [&](std::filesystem::path const& session_file) {
                try
                {
                    std::filesystem::copy_file(session_file, m_template_file);
                }
                catch (std::exception const& err)
                {
                    spdlog::error(
                        "Could not create session template file: {}",
                        err.what());
                }
            },
            [&](actions::set_session_template::current_session) {
                persistence::save_session(m_template_file, st);
            },
            [&](actions::set_session_template::empty_session) {
                try
                {
                    std::filesystem::remove(m_template_file);
                }
                catch (std::filesystem::filesystem_error const& err)
                {
                    spdlog::error(
                        "Could not remove session template file: {}",
                        err.what());
                }
            }),
        a.session_template);
}

template <>
void
persistence_middleware::process_persistence_action(
    middleware_functors const& mw_fs,
    actions::new_session const& a)
{
    try
    {
        if (std::filesystem::exists(m_template_file))
        {
            actions::load_session action{m_template_file};
            action.is_user_session = false;
            mw_fs.dispatch(action);
        }
        else
        {
            mw_fs.next(a);
        }
    }
    catch (std::filesystem::filesystem_error const& err)
    {
        spdlog::error("Could not create new session: {}", err.what());
    }
}

} // namespace piejam::runtime
