// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "spectrum_module.h"

#include "spectrum_internal_id.h"

#include <piejam/audio/multichannel_buffer.h>
#include <piejam/entity_map.h>
#include <piejam/numeric/dB_convert.h>
#include <piejam/runtime/bool_parameter.h>
#include <piejam/runtime/enum_parameter.h>
#include <piejam/runtime/float_parameter.h>
#include <piejam/runtime/fx/module.h>
#include <piejam/runtime/internal_fx_module_factory.h>
#include <piejam/runtime/parameter/map.h>
#include <piejam/runtime/parameter_factory.h>

namespace piejam::fx_modules::spectrum
{

namespace
{

auto
to_stereo_channel_string(int const n) -> std::string
{
    using namespace std::string_literals;

    switch (n)
    {
        case std::to_underlying(stereo_channel::left):
            return "L"s;

        case std::to_underlying(stereo_channel::right):
            return "R"s;

        case std::to_underlying(stereo_channel::middle):
            return "M"s;

        case std::to_underlying(stereo_channel::side):
            return "S"s;

        default:
            return "ERROR"s;
    }
}

} // namespace

auto
make_module(runtime::internal_fx_module_factory_args const& args)
    -> runtime::fx::module
{
    runtime::parameter_factory params_factory{args.params};

    return runtime::fx::module{
        .fx_instance_id = internal_id(),
        .name = box(std::string{"Spectrum"}),
        .bus_type = args.bus_type,
        .parameters =
            box(runtime::parameters_map{
                std::in_place_type<parameter_key>,
                {
                    {parameter_key::stream_a_active,
                     params_factory.make_parameter(
                         runtime::make_bool_parameter({
                             .name = "Stream A Active",
                             .default_value = true,
                         }))},
                    {parameter_key::stream_b_active,
                     params_factory.make_parameter(
                         runtime::make_bool_parameter({
                             .name = "Stream B Active",
                         }))},
                    {parameter_key::channel_a,
                     params_factory.make_parameter(
                         runtime::make_enum_parameter(
                             "Channel A",
                             stereo_channel::left,
                             &to_stereo_channel_string))},
                    {parameter_key::channel_b,
                     params_factory.make_parameter(
                         runtime::make_enum_parameter(
                             "Channel B",
                             stereo_channel::right,
                             &to_stereo_channel_string))},
                    {parameter_key::gain_a,
                     params_factory.make_parameter(
                         runtime::make_float_parameter(
                             {
                                 .name = "Gain A",
                                 .default_value = 1.f,
                             },
                             runtime::dB_float_parameter_range<-24.f, 24.f>())
                             .set_value_to_string(
                                 &runtime::default_float_to_dB_string))},
                    {parameter_key::gain_b,
                     params_factory.make_parameter(
                         runtime::make_float_parameter(
                             {
                                 .name = "Gain B",
                                 .default_value = 1.f,
                             },
                             runtime::dB_float_parameter_range<-24.f, 24.f>())
                             .set_value_to_string(
                                 &runtime::default_float_to_dB_string))},
                    {parameter_key::freeze,
                     params_factory.make_parameter(
                         runtime::make_bool_parameter({
                             .name = "Freeze",
                         }))},
                }}),
        .streams =
            box(runtime::fx::module_streams{
                {std::to_underlying(stream_key::input),
                 make_stream(args.streams, audio::num_channels(args.bus_type))},
            })};
}

} // namespace piejam::fx_modules::spectrum
