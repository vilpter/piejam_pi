// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/actions/fx_chain_actions.h>

#include <piejam/bool.h>
#include <piejam/runtime/state.h>

namespace piejam::runtime::actions
{

void
focus_fx_module::reduce(state& st) const
{
    st.focused_fx_chain_id = fx_chain_id;
    st.focused_fx_mod_id = fx_mod_id;
}

void
show_fx_module::reduce(state& st) const
{
    st.root_view_mode = runtime::root_view_mode::fx_module;
}

void
show_prev_fx_module::reduce(state& st) const
{
    auto const& fx_chain = st.mixer_state.fx_chains.at(st.focused_fx_chain_id);
    auto it = std::find(fx_chain.begin(), fx_chain.end(), st.focused_fx_mod_id);
    BOOST_ASSERT(it != fx_chain.end());
    BOOST_ASSERT(it != fx_chain.begin());
    st.focused_fx_mod_id = *std::prev(it);
}

void
show_next_fx_module::reduce(state& st) const
{
    auto const& fx_chain = st.mixer_state.fx_chains.at(st.focused_fx_chain_id);
    auto it = std::find(fx_chain.begin(), fx_chain.end(), st.focused_fx_mod_id);
    BOOST_ASSERT(it != fx_chain.end());
    ++it;
    BOOST_ASSERT(it != fx_chain.end());
    st.focused_fx_mod_id = *it;
}

} // namespace piejam::runtime::actions
