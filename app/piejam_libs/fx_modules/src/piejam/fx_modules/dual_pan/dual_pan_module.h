// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/fwd.h>
#include <piejam/runtime/fx/fwd.h>

namespace piejam::fx_modules::dual_pan
{

enum class parameter_key : runtime::parameter::key
{
    mute_left,
    pan_left,
    pan_right,
    mute_right,
};

auto make_module(runtime::internal_fx_module_factory_args const&)
    -> runtime::fx::module;

} // namespace piejam::fx_modules::dual_pan
