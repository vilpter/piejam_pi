// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dual_pan_module.h"

#include "dual_pan_internal_id.h"

#include <piejam/runtime/bool_parameter.h>
#include <piejam/runtime/float_parameter.h>
#include <piejam/runtime/fx/module.h>
#include <piejam/runtime/internal_fx_module_factory.h>
#include <piejam/runtime/parameter/map.h>
#include <piejam/runtime/parameter_factory.h>

namespace piejam::fx_modules::dual_pan
{

auto
make_module(runtime::internal_fx_module_factory_args const& args)
    -> runtime::fx::module
{
    using namespace std::string_literals;

    runtime::parameter_factory params_factory{args.params};

    return runtime::fx::module{
        .fx_instance_id = internal_id(),
        .name = box("Dual Pan"s),
        .bus_type = args.bus_type,
        .parameters =
            box(runtime::parameters_map{
                std::in_place_type<parameter_key>,
                {
                    {parameter_key::mute_left,
                     params_factory.make_parameter(
                         runtime::make_bool_parameter({
                             .name = "Mute Left",
                         }))},
                    {parameter_key::pan_left,
                     params_factory.make_parameter(
                         runtime::make_float_parameter(
                             {
                                 .name = "Pan Left",
                                 .default_value = -1.f,
                             },
                             runtime::linear_float_parameter_range<-1.f, 1.f>())
                             .set_flags({runtime::parameter_flags::bipolar}))},
                    {parameter_key::pan_right,
                     params_factory.make_parameter(
                         runtime::make_float_parameter(
                             {
                                 .name = "Pan "
                                         "Right",
                                 .default_value = 1.f,
                             },
                             runtime::linear_float_parameter_range<-1.f, 1.f>())
                             .set_flags({runtime::parameter_flags::bipolar}))},
                    {parameter_key::mute_right,
                     params_factory.make_parameter(
                         runtime::make_bool_parameter({
                             .name = "Mute Right",
                         }))},
                }}),
        .streams = {}};
}

} // namespace piejam::fx_modules::dual_pan
