// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/EnumParameter.h>

#include <piejam/gui/model/EnumListModel.h>

#include <piejam/runtime/actions/set_parameter_value.h>
#include <piejam/runtime/selectors.h>

namespace piejam::gui::model
{

EnumParameter::EnumParameter(
    runtime::state_access const& state_access,
    runtime::enum_parameter_id param_id)
    : Parameter{state_access, param_id}
    , m_values{&addQObject<EnumListModel>(observe_once(
          runtime::selectors::make_enum_parameter_values_selector(param_id)))}

{
    setMinValue(observe_once(
        runtime::selectors::make_parameter_min_selector(param_id)));

    setMaxValue(observe_once(
        runtime::selectors::make_parameter_max_selector(param_id)));
}

auto
EnumParameter::paramId() const -> runtime::enum_parameter_id
{
    return std::get<runtime::enum_parameter_id>(Parameter::paramId());
}

void
EnumParameter::onSubscribe()
{
    Parameter::onSubscribe();

    observe(
        runtime::selectors::make_parameter_value_selector(paramId()),
        [this](int const value) { setValue(value); });
}

void
EnumParameter::changeValue(int value)
{
    dispatch(runtime::actions::set_enum_parameter(paramId(), value));
}

} // namespace piejam::gui::model
