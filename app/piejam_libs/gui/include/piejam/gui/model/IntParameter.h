// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/Parameter.h>

namespace piejam::gui::model
{

class IntParameter : public Parameter
{
    Q_OBJECT

    PIEJAM_GUI_PROPERTY(int, value, setValue)
    PIEJAM_GUI_PROPERTY(int, minValue, setMinValue)
    PIEJAM_GUI_PROPERTY(int, maxValue, setMaxValue)

public:
    IntParameter(runtime::state_access const&, runtime::int_parameter_id);

    static constexpr auto StaticType = Type::Int;

    auto type() const noexcept -> Type override
    {
        return IntParameter::StaticType;
    }

    Q_INVOKABLE void changeValue(int);

private:
    void onSubscribe() override;

    auto paramId() const -> runtime::int_parameter_id;
};

} // namespace piejam::gui::model
