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

namespace piejam::fx_modules::tuner::gui
{

class FxTuner final : public piejam::gui::model::FxModule
{
    Q_OBJECT

    PIEJAM_GUI_MODEL_PIMPL

    PIEJAM_GUI_PROPERTY(float, detectedFrequency, setDetectedFrequency)
    PIEJAM_GUI_PROPERTY(QString, detectedPitch, setDetectedPitch)
    PIEJAM_GUI_PROPERTY(int, detectedCents, setDetectedCents)

public:
    FxTuner(runtime::state_access const&, runtime::fx::module_id);

    auto type() const noexcept -> piejam::gui::model::FxModuleType override;

private:
    void onSubscribe() override;

    void onChannelChanged();
};

} // namespace piejam::fx_modules::tuner::gui
