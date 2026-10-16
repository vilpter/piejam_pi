// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/actions/session_actions.h>

#include <piejam/runtime/state.h>

namespace piejam::runtime::actions
{

void
switch_startup_session::reduce(state& st) const
{
    st.startup_session = startup_session;
}

void
new_session::reduce(state& st) const
{
    reset_state(st);
}

} // namespace piejam::runtime::actions
