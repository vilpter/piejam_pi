// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/FxModule.h>

#include <piejam/runtime/fx/fwd.h>

namespace piejam::gui::model
{

class FxGenericModule : public FxModule
{
    Q_OBJECT

    PIEJAM_GUI_CONSTANT_PROPERTY(QAbstractListModel*, parametersList)

public:
    FxGenericModule(runtime::state_access const&, runtime::fx::module_id);

    auto type() const noexcept -> FxModuleType override
    {
        return {};
    }

private:
    void onSubscribe() override;
};

} // namespace piejam::gui::model
