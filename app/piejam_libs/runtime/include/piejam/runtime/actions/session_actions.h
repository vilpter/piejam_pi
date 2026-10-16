// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/actions/persistence_action.h>
#include <piejam/runtime/fwd.h>
#include <piejam/runtime/ui/cloneable_action.h>

#include <filesystem>

namespace piejam::runtime::actions
{

struct load_session final
    : ui::cloneable_action<load_session, action>
    , visitable_persistence_action<load_session>
{
    explicit load_session(std::filesystem::path file)
        : file(std::move(file))
    {
    }

    std::filesystem::path file;
    bool is_user_session{true};
};

struct save_session final
    : ui::cloneable_action<save_session, action>
    , visitable_persistence_action<save_session>
{
    explicit save_session(std::filesystem::path file)
        : file(std::move(file))
    {
    }

    std::filesystem::path file;
};

struct switch_startup_session final
    : ui::cloneable_action<switch_startup_session, reducible_action>
{
    void reduce(state&) const override;

    runtime::startup_session startup_session;
};

struct initiate_startup_session final
    : ui::cloneable_action<initiate_startup_session, action>
    , visitable_persistence_action<initiate_startup_session>
{
};

struct set_session_template final
    : ui::cloneable_action<set_session_template, action>
    , visitable_persistence_action<set_session_template>
{
    struct current_session
    {
    };

    struct empty_session
    {
    };

    using template_type =
        std::variant<std::filesystem::path, current_session, empty_session>;

    template_type session_template;
};

struct new_session final
    : ui::cloneable_action<new_session, reducible_action>
    , visitable_persistence_action<new_session>
{
    void reduce(state&) const override;
};

} // namespace piejam::runtime::actions
