// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/actions/insert_fx_module.h>

#include <piejam/runtime/ladspa_fx/ladspa_fx_module.h>
#include <piejam/runtime/parameter/assignment.h>
#include <piejam/runtime/state.h>

#include <boost/assert.hpp>

namespace piejam::runtime::actions
{

void
insert_internal_fx_module::reduce(state& st) const
{
    auto fx_mod_id = runtime::insert_internal_fx_module(
        st,
        fx_chain_id,
        position,
        type,
        initial_values,
        midi_assignments);

    if (show_fx_module)
    {
        st.focused_fx_chain_id = fx_chain_id;
        st.focused_fx_mod_id = fx_mod_id;
        st.root_view_mode = runtime::root_view_mode::fx_module;
    }
}

void
insert_ladspa_fx_module::reduce(state& st) const
{
    auto fx_mod_id = runtime::insert_ladspa_fx_module(
        st,
        fx_chain_id,
        position,
        instance_id,
        plugin_desc,
        control_inputs,
        initial_values,
        midi_assignments);

    if (show_fx_module)
    {
        st.focused_fx_chain_id = fx_chain_id;
        st.focused_fx_mod_id = fx_mod_id;
        st.root_view_mode = runtime::root_view_mode::fx_module;
    }
}

void
insert_missing_ladspa_fx_module::reduce(state& st) const
{
    runtime::insert_missing_ladspa_fx_module(
        st,
        fx_chain_id,
        position,
        unavailable_ladspa,
        name);
}

void
replace_missing_ladspa_fx_module::reduce(state& st) const
{
    for (auto const& [fx_chain_id, replacements] : fx_chain_replacements)
    {
        for (auto const& [pos, ladspa_instance] : replacements)
        {
            BOOST_ASSERT(pos < st.mixer_state.fx_chains.at(fx_chain_id).size());
            auto const prev_fx_mod_id =
                st.mixer_state.fx_chains.at(fx_chain_id)[pos];

            auto unavail_id = std::get<fx::unavailable_ladspa_id>(
                st.fx_state.modules.at(prev_fx_mod_id).fx_instance_id);

            auto const& unavail =
                st.fx_state.unavailable_ladspa_plugins.at(unavail_id);

            auto fx_mod_id = runtime::insert_ladspa_fx_module(
                st,
                fx_chain_id,
                pos,
                ladspa_instance.instance_id,
                ladspa_instance.plugin_desc,
                ladspa_instance.control_inputs,
                unavail.parameter_values,
                unavail.midi_assignments);

            // transfer active state
            st.params.at(st.fx_state.active_modules.at(fx_mod_id))
                .set(st.params.at(st.fx_state.active_modules.at(prev_fx_mod_id))
                         .get());

            runtime::remove_fx_module(st, fx_chain_id, prev_fx_mod_id);
        }
    }
}

} // namespace piejam::runtime::actions
