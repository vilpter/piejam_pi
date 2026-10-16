// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/BoolParameter.h>

#include <piejam/runtime/actions/set_parameter_value.h>
#include <piejam/runtime/selectors.h>

namespace piejam::gui::model
{

BoolParameter::BoolParameter(
    runtime::state_access const& state_access,
    runtime::bool_parameter_id param_id)
    : Parameter(state_access, param_id)
{
}

auto
BoolParameter::paramId() const -> runtime::bool_parameter_id
{
    return std::get<runtime::bool_parameter_id>(Parameter::paramId());
}

void
BoolParameter::onSubscribe()
{
    Parameter::onSubscribe();

    observe(
        runtime::selectors::make_parameter_value_selector(paramId()),
        [this](bool const value) { setValue(value); });
}

void
BoolParameter::changeValue(bool value)
{
    dispatch(runtime::actions::set_bool_parameter(paramId(), value));
}

} // namespace piejam::gui::model
