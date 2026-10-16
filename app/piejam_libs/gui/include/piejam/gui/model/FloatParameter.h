// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/Parameter.h>

namespace piejam::gui::model
{

class FloatParameter : public Parameter
{
    Q_OBJECT

    PIEJAM_GUI_PROPERTY(double, value, setValue)
    PIEJAM_GUI_PROPERTY(double, normalizedValue, setNormalizedValue)
    PIEJAM_GUI_PROPERTY(bool, bipolar, setBipolar)

public:
    FloatParameter(runtime::state_access const&, runtime::float_parameter_id);

    static constexpr auto StaticType = Type::Float;

    auto type() const noexcept -> Type override
    {
        return FloatParameter::StaticType;
    }

    Q_INVOKABLE void changeValue(double);
    Q_INVOKABLE void changeNormalizedValue(double);

    auto valueF() const noexcept -> float
    {
        return static_cast<float>(m_value);
    }

    auto normalizedValueF() const noexcept -> float
    {
        return static_cast<float>(m_normalizedValue);
    }

private:
    void onSubscribe() override;

    auto paramId() const -> runtime::float_parameter_id;
};

} // namespace piejam::gui::model
