// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/actions/move_fx_module.h>

#include <piejam/runtime/state.h>

#include <algorithm>

namespace piejam::runtime::actions
{

void
move_fx_module_up::reduce(state& st) const
{
    auto fx_chain = st.mixer_state.fx_chains.at(st.focused_fx_chain_id);

    auto it = std::ranges::find(fx_chain, st.focused_fx_mod_id);
    BOOST_ASSERT(it != fx_chain.end());
    BOOST_ASSERT(it != fx_chain.begin());
    std::iter_swap(it, std::prev(it));

    st.mixer_state.fx_chains.assign(
        st.focused_fx_chain_id,
        std::move(fx_chain));
}

void
move_fx_module_down::reduce(state& st) const
{
    auto fx_chain = st.mixer_state.fx_chains.at(st.focused_fx_chain_id);

    auto it = std::ranges::find(fx_chain, st.focused_fx_mod_id);
    BOOST_ASSERT(it != fx_chain.end());
    BOOST_ASSERT(std::next(it) != fx_chain.end());
    std::iter_swap(it, std::next(it));

    st.mixer_state.fx_chains.assign(
        st.focused_fx_chain_id,
        std::move(fx_chain));
}

} // namespace piejam::runtime::actions
