// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/FloatParameter.h>

#include <piejam/runtime/actions/set_parameter_value.h>
#include <piejam/runtime/selectors.h>
#include <piejam/runtime/ui/thunk_action.h>

namespace piejam::gui::model
{

FloatParameter::FloatParameter(
    runtime::state_access const& state_access,
    runtime::float_parameter_id param_id)
    : Parameter{state_access, param_id}
{
    setBipolar(observe_once(
        runtime::selectors::make_float_parameter_bipolar_selector(param_id)));
}

auto
FloatParameter::paramId() const -> runtime::float_parameter_id
{
    return std::get<runtime::float_parameter_id>(Parameter::paramId());
}

void
FloatParameter::onSubscribe()
{
    Parameter::onSubscribe();

    auto const float_param_id = paramId();

    observe(
        runtime::selectors::make_parameter_value_selector(float_param_id),
        [this](float const value) { setValue(static_cast<double>(value)); });

    observe(
        runtime::selectors::make_parameter_normalized_value_selector(
            float_param_id),
        [this](float const value) {
            setNormalizedValue(static_cast<double>(value));
        });
}

void
FloatParameter::changeValue(double value)
{
    dispatch(
        runtime::actions::set_float_parameter(
            paramId(),
            static_cast<float>(value)));
}

void
FloatParameter::changeNormalizedValue(double value)
{
    dispatch(
        runtime::actions::set_float_parameter_normalized(
            paramId(),
            static_cast<float>(value)));
}

} // namespace piejam::gui::model
