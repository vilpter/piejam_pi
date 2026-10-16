// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/fwd.h>
#include <piejam/runtime/fx/fwd.h>

namespace piejam::fx_modules::utility
{

enum class parameter_key : runtime::parameter::key
{
    invert,
    invert_left,
    invert_right,
    gain,
};

auto make_module(runtime::internal_fx_module_factory_args const&)
    -> runtime::fx::module;

} // namespace piejam::fx_modules::utility
