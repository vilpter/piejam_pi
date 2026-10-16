// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "utility_module.h"

#include "utility_internal_id.h"

#include <piejam/numeric/dB_convert.h>
#include <piejam/runtime/bool_parameter.h>
#include <piejam/runtime/float_parameter.h>
#include <piejam/runtime/fx/module.h>
#include <piejam/runtime/internal_fx_module_factory.h>
#include <piejam/runtime/parameter/map.h>
#include <piejam/runtime/parameter_factory.h>

namespace piejam::fx_modules::utility
{

auto
make_module(runtime::internal_fx_module_factory_args const& args)
    -> runtime::fx::module
{
    runtime::parameter_factory params_factory{args.params};

    runtime::parameters_map parameters{std::in_place_type<parameter_key>, {}};

    parameters.emplace(
        parameter_key::gain,
        params_factory.make_parameter(
            runtime::make_float_parameter(
                {
                    .name = "Gain",
                    .default_value = 1.f,
                },
                runtime::dB_float_parameter_range<-24.f, 24.f>())
                .set_value_to_string(&runtime::default_float_to_dB_string)
                .set_flags({runtime::parameter_flags::bipolar})));

    switch (args.bus_type)
    {
        case audio::bus_type::mono:
            parameters.emplace(
                parameter_key::invert,
                params_factory.make_parameter(
                    runtime::make_bool_parameter({
                        .name = "Invert",
                    })));
            break;

        case audio::bus_type::stereo:
            parameters.emplace(
                parameter_key::invert_left,
                params_factory.make_parameter(
                    runtime::make_bool_parameter({
                        .name = "Invert L",
                    })));
            parameters.emplace(
                parameter_key::invert_right,
                params_factory.make_parameter(
                    runtime::make_bool_parameter({
                        .name = "Invert R",
                    })));
            break;
    }

    return runtime::fx::module{
        .fx_instance_id = internal_id(),
        .name = box(std::string{"Utility"}),
        .bus_type = args.bus_type,
        .parameters = box(std::move(parameters)),
        .streams = {}};
}

} // namespace piejam::fx_modules::utility
