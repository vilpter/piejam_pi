// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filter_module.h"

#include "filter_internal_id.h"

#include <piejam/audio/multichannel_buffer.h>
#include <piejam/entity_map.h>
#include <piejam/runtime/enum_parameter.h>
#include <piejam/runtime/float_parameter.h>
#include <piejam/runtime/fx/module.h>
#include <piejam/runtime/internal_fx_module_factory.h>
#include <piejam/runtime/parameter/map.h>
#include <piejam/runtime/parameter_factory.h>

#include <format>

namespace piejam::fx_modules::filter
{

namespace
{

auto
to_type_string(int const n) -> std::string
{
    switch (n)
    {
        case std::to_underlying(type::lp2):
            return "LP2";
        case std::to_underlying(type::lp4):
            return "LP4";
        case std::to_underlying(type::bp2):
            return "BP2";
        case std::to_underlying(type::bp4):
            return "BP4";
        case std::to_underlying(type::hp2):
            return "HP2";
        case std::to_underlying(type::hp4):
            return "HP4";
        case std::to_underlying(type::br):
            return "BR";

        default:
            return "Pass";
    }
}

auto
to_cutoff_string(float const f)
{
    return f < 1000.f ? std::format("{:.2f} Hz", f)
                      : std::format("{:.2f} kHz", f / 1000.f);
}

auto
to_resonance_string(float const r)
{
    auto p = r * 100;
    return p < 10.f ? std::format("{:.2f}%", p)
                    : (p < 100.f ? std::format("{:.1f}%", p)
                                 : std::format("{:.0f}%", p));
}

} // namespace

auto
make_module(runtime::internal_fx_module_factory_args const& args)
    -> runtime::fx::module
{
    using namespace std::string_literals;

    runtime::parameter_factory params_factory{args.params};

    return runtime::fx::module{
        .fx_instance_id = internal_id(),
        .name = box("Filter"s),
        .bus_type = args.bus_type,
        .parameters =
            box(runtime::parameters_map{
                std::in_place_type<parameter_key>,
                {
                    {parameter_key::type,
                     params_factory.make_parameter(
                         runtime::make_enum_parameter(
                             "Type"s,
                             type::lp2,
                             &to_type_string))},
                    {parameter_key::cutoff,
                     params_factory.make_parameter(
                         runtime::make_float_parameter(
                             {
                                 .name = "Cutoff",
                                 .default_value = 440.f,
                             },
                             runtime::logarithmic_float_parameter_range<
                                 10.f,
                                 20000.f>())
                             .set_value_to_string(&to_cutoff_string))},
                    {parameter_key::resonance,
                     params_factory.make_parameter(
                         runtime::make_float_parameter(
                             {
                                 .name = "Resonance",
                                 .default_value = 0.f,
                             },
                             runtime::linear_float_parameter_range<0.f, 1.f>())
                             .set_value_to_string(&to_resonance_string))},
                }}),
        .streams =
            box(runtime::fx::module_streams{
                {std::to_underlying(stream_key::in_out),
                 make_stream(args.streams, num_channels(args.bus_type) * 2)},
            })};
}

} // namespace piejam::fx_modules::filter
