// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/Parameter.h>

#include <piejam/gui/model/MidiAssignable.h>

#include <piejam/runtime/actions/set_parameter_value.h>
#include <piejam/runtime/selectors.h>
#include <piejam/runtime/ui/thunk_action.h>

namespace piejam::gui::model
{

Parameter::Parameter(
    runtime::state_access const& state_access,
    runtime::parameter_id const& param_id)
    : CompositeSubscribableModel(state_access)
    , m_midi{observe_once(runtime::selectors::make_parameter_is_midi_assignable_selector(param_id)) ? &addModel<MidiAssignable>(param_id) : nullptr}
    , m_param_id{param_id}
{
    setName(
        QString::fromStdString(observe_once(
            runtime::selectors::make_parameter_name_selector(param_id))));
}

void
Parameter::onSubscribe()
{
    observe(
        runtime::selectors::make_parameter_value_string_selector(m_param_id),
        [this](std::string const& text) {
            setValueString(QString::fromStdString(text));
        });
}

void
Parameter::resetToDefault()
{
    dispatch(runtime::actions::reset_parameter_to_default_value(m_param_id));
}

} // namespace piejam::gui::model
