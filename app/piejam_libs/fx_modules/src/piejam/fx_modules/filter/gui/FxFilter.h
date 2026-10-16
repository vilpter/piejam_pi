// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/FxModule.h>
#include <piejam/gui/model/SubscribableModel.h>
#include <piejam/gui/model/fwd.h>

#include <piejam/runtime/fx/fwd.h>

namespace piejam::fx_modules::filter::gui
{

class FxFilter final : public piejam::gui::model::FxModule
{
    Q_OBJECT

    PIEJAM_GUI_MODEL_PIMPL

    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::SpectrumSlot*, spectrumIn)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::SpectrumSlot*, spectrumOut)

    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::EnumParameter*, filterType)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::FloatParameter*, cutoff)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::FloatParameter*, resonance)

public:
    FxFilter(runtime::state_access const&, runtime::fx::module_id);

    auto type() const noexcept -> piejam::gui::model::FxModuleType override;

    Q_INVOKABLE void clear();

private:
    void onSubscribe() override;
};

} // namespace piejam::fx_modules::filter::gui
