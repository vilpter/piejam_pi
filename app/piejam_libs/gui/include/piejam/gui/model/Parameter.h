// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/SubscribableModel.h>
#include <piejam/gui/model/fwd.h>

#include <piejam/runtime/parameters.h>

namespace piejam::gui::model
{

class Parameter : public CompositeSubscribableModel
{
    Q_OBJECT

    Q_PROPERTY(Type type READ type CONSTANT FINAL)
    PIEJAM_GUI_PROPERTY(QString, name, setName)
    PIEJAM_GUI_PROPERTY(QString, valueString, setValueString)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::MidiAssignable*, midi)

public:
    Parameter(runtime::state_access const&, runtime::parameter_id const&);

    auto paramId() const noexcept -> runtime::parameter_id
    {
        return m_param_id;
    }

    enum class Type
    {
        Float,
        Int,
        Bool,
        Enum,
    };

    Q_ENUM(Type)

    virtual auto type() const noexcept -> Type = 0;

    Q_INVOKABLE void resetToDefault();

protected:
    void onSubscribe() override;

private:
    runtime::parameter_id m_param_id;
};

} // namespace piejam::gui::model
