// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "scope_internal_id.h"

#include "gui/FxScope.h"
#include "scope_component.h"
#include "scope_module.h"

#include "../module_registration.h"

namespace piejam::fx_modules::scope
{

void
init()
{
    static std::once_flag s_init;
    std::call_once(s_init, []() {
        PIEJAM_FX_MODULES_MODEL(gui::FxScope, "FxScope");
        internal_id();
    });
}

auto
internal_id() -> runtime::fx::internal_id
{
    using namespace std::string_literals;

    static auto const s_id = register_module(
        module_registration{
            .available_for_mono = true,
            .persistence_name = "scope"s,
            .fx_module_factory = &make_module,
            .fx_component_factory = &make_component,
            .fx_browser_entry_name = "Oscilloscope",
            .fx_browser_entry_description =
                "Observe waveform of an audio signal.",
            .fx_module_content_factory =
                &piejam::gui::model::makeFxModule<gui::FxScope>,
            .viewSource = "/PieJam/FxModules/ScopeView.qml"});

    return s_id;
}

} // namespace piejam::fx_modules::scope
