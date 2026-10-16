// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/FxModule.h>
#include <piejam/gui/model/SubscribableModel.h>
#include <piejam/gui/model/Types.h>
#include <piejam/gui/model/fwd.h>

#include <piejam/runtime/fx/fwd.h>

namespace piejam::fx_modules::scope::gui
{

class FxScope final : public piejam::gui::model::FxModule
{
    Q_OBJECT

    PIEJAM_GUI_MODEL_PIMPL

    PIEJAM_GUI_PROPERTY(double, sampleRate, setSampleRate)
    PIEJAM_GUI_WRITABLE_PROPERTY(int, viewSize, setViewSize)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::EnumParameter*, mode)
    PIEJAM_GUI_CONSTANT_PROPERTY(
        piejam::gui::model::EnumParameter*,
        triggerSlope)
    PIEJAM_GUI_CONSTANT_PROPERTY(
        piejam::gui::model::FloatParameter*,
        triggerLevel)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::FloatParameter*, holdTime)
    PIEJAM_GUI_CONSTANT_PROPERTY(
        piejam::gui::model::EnumParameter*,
        waveformWindowSize)
    PIEJAM_GUI_CONSTANT_PROPERTY(
        piejam::gui::model::EnumParameter*,
        scopeWindowSize)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::BoolParameter*, activeA)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::BoolParameter*, activeB)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::EnumParameter*, channelA)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::EnumParameter*, channelB)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::FloatParameter*, gainA)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::FloatParameter*, gainB)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::BoolParameter*, freeze)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::WaveformSlot*, waveformA)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::WaveformSlot*, waveformB)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::ScopeSlot*, scopeA)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::ScopeSlot*, scopeB)

public:
    FxScope(runtime::state_access const&, runtime::fx::module_id);

    auto type() const noexcept -> piejam::gui::model::FxModuleType override;

    enum class Mode
    {
        Free,
        TriggerA,
        TriggerB,
    };

    Q_ENUM(Mode)

    Q_INVOKABLE void clear();

private:
    void onSubscribe() override;
};

} // namespace piejam::fx_modules::scope::gui
