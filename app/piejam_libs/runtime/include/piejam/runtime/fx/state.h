// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/fx/fwd.h>
#include <piejam/runtime/fx/module.h>

#include <piejam/boxed_map.h>
#include <piejam/entity_id_hash.h>
#include <piejam/entity_map.h>

namespace piejam::runtime::fx
{

struct state
{
    modules_t modules;
    active_modules_t active_modules;

    ladspa_instances_t ladspa_instances;
    unavailable_ladspa_plugins_t unavailable_ladspa_plugins;
};

} // namespace piejam::runtime::fx
