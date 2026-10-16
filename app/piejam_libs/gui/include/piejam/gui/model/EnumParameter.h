// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/Parameter.h>
#include <piejam/gui/model/fwd.h>

namespace piejam::gui::model
{

class EnumParameter final : public Parameter
{
    Q_OBJECT

    PIEJAM_GUI_PROPERTY(int, value, setValue)
    PIEJAM_GUI_PROPERTY(int, minValue, setMinValue)
    PIEJAM_GUI_PROPERTY(int, maxValue, setMaxValue)
    PIEJAM_GUI_CONSTANT_PROPERTY(QAbstractListModel*, values)

public:
    EnumParameter(runtime::state_access const&, runtime::enum_parameter_id);

    static constexpr auto StaticType = Type::Enum;

    auto type() const noexcept -> Type override
    {
        return EnumParameter::StaticType;
    }

    template <class E>
        requires(
            std::is_enum_v<E> && std::is_same_v<std::underlying_type_t<E>, int>)
    auto as() const noexcept -> E
    {
        return static_cast<E>(value());
    }

    Q_INVOKABLE void changeValue(int);

private:
    void onSubscribe() override;

    auto paramId() const -> runtime::enum_parameter_id;
};

} // namespace piejam::gui::model
