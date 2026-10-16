// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "scope_component.h"

#include "scope_module.h"

#include <piejam/audio/engine/component.h>
#include <piejam/audio/sample_rate.h>
#include <piejam/runtime/components/stream.h>
#include <piejam/runtime/fx/module.h>
#include <piejam/runtime/internal_fx_component_factory.h>

namespace piejam::fx_modules::scope
{

auto
make_component(runtime::internal_fx_component_factory_args const& args)
    -> std::unique_ptr<audio::engine::component>
{
    return runtime::components::make_stream(
        args.fx_mod.streams->at(std::to_underlying(stream_key::input)),
        args.stream_procs,
        num_channels(args.fx_mod.bus_type),
        args.sample_rate.samples_for_duration(std::chrono::milliseconds(120)),
        "scope");
}

} // namespace piejam::fx_modules::scope
