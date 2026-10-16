// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/FxModule.h>
#include <piejam/gui/model/SpectrumSlot.h>
#include <piejam/gui/model/SubscribableModel.h>
#include <piejam/gui/model/Types.h>
#include <piejam/gui/model/fwd.h>

#include <piejam/pimpl.h>
#include <piejam/runtime/fx/fwd.h>

namespace piejam::fx_modules::spectrum::gui
{

class FxSpectrum final : public piejam::gui::model::FxModule
{
    Q_OBJECT

    PIEJAM_GUI_MODEL_PIMPL

    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::SpectrumSlot*, spectrumA)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::SpectrumSlot*, spectrumB)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::BoolParameter*, activeA)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::BoolParameter*, activeB)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::EnumParameter*, channelA)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::EnumParameter*, channelB)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::FloatParameter*, gainA)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::FloatParameter*, gainB)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::BoolParameter*, freeze)

public:
    FxSpectrum(runtime::state_access const&, runtime::fx::module_id);

    auto type() const noexcept -> piejam::gui::model::FxModuleType override;

    Q_INVOKABLE void clear();

private:
    void onSubscribe() override;
};

} // namespace piejam::fx_modules::spectrum::gui
