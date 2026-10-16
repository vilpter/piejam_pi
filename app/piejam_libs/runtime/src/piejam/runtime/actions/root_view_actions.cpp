// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/actions/root_view_actions.h>

#include <piejam/runtime/state.h>

namespace piejam::runtime::actions
{

void
set_root_view_mode::reduce(state& st) const
{
    st.root_view_mode = mode;
}

void
show_fx_browser::reduce(state& st) const
{
    st.fx_browser_fx_chain_id = fx_chain_id;
    st.root_view_mode = root_view_mode::fx_browser;
}

} // namespace piejam::runtime::actions
