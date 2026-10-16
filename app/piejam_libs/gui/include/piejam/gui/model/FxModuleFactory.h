// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/model/fwd.h>

#include <piejam/entity_id.h>
#include <piejam/registry_map.h>
#include <piejam/runtime/fwd.h>
#include <piejam/runtime/fx/fwd.h>
#include <piejam/runtime/state_access.h>
#include <piejam/runtime/subscriber.h>

#include <functional>
#include <memory>

namespace piejam::gui::model
{

using FxModuleFactory = std::function<std::unique_ptr<FxModule>(
    runtime::state_access const&,
    runtime::fx::module_id)>;

using FxModuleFactories =
    registry_map<runtime::fx::internal_id, FxModuleFactory>;

template <class T>
auto
makeFxModule(
    runtime::state_access const& state_access,
    runtime::fx::module_id fx_mod_id) -> std::unique_ptr<FxModule>
{
    return std::make_unique<T>(state_access, fx_mod_id);
}

} // namespace piejam::gui::model
