// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/MidiAssignable.h>

#include <piejam/runtime/actions/control_midi_assignment.h>
#include <piejam/runtime/selectors.h>

#include <boost/assert.hpp>

namespace piejam::gui::model
{

static auto
toQString(runtime::midi_assignment const& ass) -> QString
{
    switch (ass.control_type)
    {
        case runtime::midi_assignment::type::cc:
            return QString("CC %1 @%2")
                .arg(ass.control_id)
                .arg(ass.channel + 1);

        case runtime::midi_assignment::type::pitch_bend:
            return QString("PB @%2").arg(ass.channel + 1);
    }

    [[assume(false)]];
}

MidiAssignable::MidiAssignable(
    runtime::state_access const& state_access,
    runtime::parameter_id const& assignment_id)
    : SubscribableModel(state_access)
    , m_assignment_id(assignment_id)
{
}

void
MidiAssignable::onSubscribe()
{
    observe(
        runtime::selectors::make_is_midi_learning_selector(m_assignment_id),
        [this](bool learning) { setLearning(learning); });

    observe(
        runtime::selectors::make_midi_assignment_selector(m_assignment_id),
        [this](std::optional<runtime::midi_assignment> const& ass) {
            setAssignment(ass ? toQString(*ass) : QString());
        });
}

void
MidiAssignable::startLearn()
{
    runtime::actions::start_midi_learning action;
    action.assignment_id = m_assignment_id;
    dispatch(action);
}

void
MidiAssignable::stopLearn()
{
    dispatch(runtime::actions::stop_midi_learning{});
}

void
MidiAssignable::clearAssignment()
{
    runtime::actions::clear_midi_assignment action;
    action.assignment_id = m_assignment_id;
    dispatch(action);
}

} // namespace piejam::gui::model
