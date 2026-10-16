// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/Parameter.h>

namespace piejam::gui::model
{

class BoolParameter final : public Parameter
{
    Q_OBJECT

    PIEJAM_GUI_PROPERTY(bool, value, setValue)

public:
    BoolParameter(runtime::state_access const&, runtime::bool_parameter_id);

    static constexpr auto StaticType = Type::Bool;

    auto type() const noexcept -> Type override
    {
        return BoolParameter::StaticType;
    }

    Q_INVOKABLE void changeValue(bool);

private:
    void onSubscribe() override;

    auto paramId() const -> runtime::bool_parameter_id;
};

} // namespace piejam::gui::model
