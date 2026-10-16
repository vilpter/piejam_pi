// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/IntParameter.h>

#include <piejam/runtime/actions/set_parameter_value.h>
#include <piejam/runtime/selectors.h>

namespace piejam::gui::model
{

IntParameter::IntParameter(
    runtime::state_access const& state_access,
    runtime::int_parameter_id param_id)
    : Parameter{state_access, param_id}

{
    setMinValue(observe_once(
        runtime::selectors::make_parameter_min_selector(param_id)));

    setMaxValue(observe_once(
        runtime::selectors::make_parameter_max_selector(param_id)));
}

auto
IntParameter::paramId() const -> runtime::int_parameter_id
{
    return std::get<runtime::int_parameter_id>(Parameter::paramId());
}

void
IntParameter::onSubscribe()
{
    Parameter::onSubscribe();

    observe(
        runtime::selectors::make_parameter_value_selector(paramId()),
        [this](int const value) { setValue(value); });
}

void
IntParameter::changeValue(int value)
{
    dispatch(runtime::actions::set_int_parameter(paramId(), value));
}

} // namespace piejam::gui::model
