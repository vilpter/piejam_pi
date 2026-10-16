// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tuner_internal_id.h"

#include "gui/FxTuner.h"
#include "tuner_component.h"
#include "tuner_module.h"

#include "../module_registration.h"

namespace piejam::fx_modules::tuner
{

void
init()
{
    static std::once_flag s_init;
    std::call_once(s_init, []() {
        PIEJAM_FX_MODULES_MODEL(gui::FxTuner, "FxTuner");
        internal_id();
    });
}

auto
internal_id() -> runtime::fx::internal_id
{
    using namespace std::string_literals;

    static auto const id = register_module(
        module_registration{
            .available_for_mono = true,
            .persistence_name = "tuner"s,
            .fx_module_factory = &make_module,
            .fx_component_factory = &make_component,
            .fx_browser_entry_name = "Tuner",
            .fx_browser_entry_description = "Detect pitch of an audio signal.",
            .fx_module_content_factory =
                &piejam::gui::model::makeFxModule<gui::FxTuner>,
            .viewSource = "/PieJam/FxModules/TunerView.qml"});
    return id;
}

} // namespace piejam::fx_modules::tuner
