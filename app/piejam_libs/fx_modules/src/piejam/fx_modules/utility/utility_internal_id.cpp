// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "utility_internal_id.h"

#include "gui/FxUtility.h"
#include "utility_component.h"
#include "utility_module.h"

#include "../module_registration.h"

namespace piejam::fx_modules::utility
{

void
init()
{
    static std::once_flag s_init;
    std::call_once(s_init, []() {
        PIEJAM_FX_MODULES_MODEL(gui::FxUtility, "FxUtility");
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
            .persistence_name = "utility"s,
            .fx_module_factory = &make_module,
            .fx_component_factory = &make_component,
            .fx_browser_entry_name = "Utility",
            .fx_browser_entry_description =
                "Amplify or attenuate an audio signal.",
            .fx_module_content_factory =
                &piejam::gui::model::makeFxModule<gui::FxUtility>,
            .viewSource = "/PieJam/FxChainControls/GenericFxModuleView.qml"});
    return id;
}

} // namespace piejam::fx_modules::utility
