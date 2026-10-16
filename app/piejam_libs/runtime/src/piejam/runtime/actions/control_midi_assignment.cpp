// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/actions/control_midi_assignment.h>

#include <piejam/runtime/state.h>

namespace piejam::runtime::actions
{

void
start_midi_learning::reduce(state& st) const
{
    st.midi_learning = assignment_id;
}

void
stop_midi_learning::reduce(state& st) const
{
    BOOST_ASSERT(st.midi_learning);

    if (learned)
    {
        update_midi_assignments(
            st,
            midi_assignments_map{{*st.midi_learning, *learned}});
    }

    st.midi_learning.reset();
}

void
clear_midi_assignment::reduce(state& st) const
{
    st.midi_assignments.lock()->erase(assignment_id);
    st.midi_learning.reset();
}

} // namespace piejam::runtime::actions
