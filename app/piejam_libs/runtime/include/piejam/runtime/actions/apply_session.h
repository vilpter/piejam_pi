// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/fwd.h>
#include <piejam/runtime/persistence/session.h>
#include <piejam/runtime/ui/cloneable_action.h>

#include <piejam/box.h>

#include <filesystem>

namespace piejam::runtime::actions
{

struct apply_session final
    : ui::cloneable_action<apply_session, reducible_action>
{
    box<persistence::session> session;
    box<std::filesystem::path> current_session;

    void reduce(state&) const override;
};

} // namespace piejam::runtime::actions
