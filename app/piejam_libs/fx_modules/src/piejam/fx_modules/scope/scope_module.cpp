// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "scope_module.h"

#include "scope_internal_id.h"

#include <piejam/audio/multichannel_buffer.h>
#include <piejam/entity_map.h>
#include <piejam/runtime/bool_parameter.h>
#include <piejam/runtime/enum_parameter.h>
#include <piejam/runtime/float_parameter.h>
#include <piejam/runtime/fx/module.h>
#include <piejam/runtime/internal_fx_module_factory.h>
#include <piejam/runtime/parameter/map.h>
#include <piejam/runtime/parameter_factory.h>

#include <format>

namespace piejam::fx_modules::scope
{

namespace
{

auto
to_mode_mono_string(int const n) -> std::string
{
    using namespace std::string_literals;

    switch (n)
    {
        case std::to_underlying(mode_mono::free):
            return "Free"s;
        case std::to_underlying(mode_mono::trigger):
            return "Trigger"s;

        default:
            return "ERROR"s;
    }
}

auto
to_mode_stereo_string(int const n) -> std::string
{
    using namespace std::string_literals;

    switch (n)
    {
        case std::to_underlying(mode_stereo::free):
            return "Free"s;
        case std::to_underlying(mode_stereo::trigger_a):
            return "Trigger A"s;
        case std::to_underlying(mode_stereo::trigger_b):
            return "Trigger B"s;

        default:
            return "ERROR"s;
    }
}

auto
to_trigger_slope_string(int const n) -> std::string
{
    using namespace std::string_literals;

    switch (n)
    {
        case std::to_underlying(trigger_slope::rising_edge):
            return "Rising Edge"s;

        case std::to_underlying(trigger_slope::falling_edge):
            return "Falling Edge"s;

        default:
            return "ERROR"s;
    }
}

auto
to_hold_time_string(float const time) -> std::string
{
    return time > 1000.f ? std::format("{:1.2f} s", time / 1000.f)
                         : std::format("{:.0f} ms", time);
}

auto
to_window_size_string(int const n) -> std::string
{
    using namespace std::string_literals;

    switch (n)
    {
        case std::to_underlying(window_size::very_small):
            return "Very Small"s;

        case std::to_underlying(window_size::small):
            return "Small"s;

        case std::to_underlying(window_size::medium):
            return "Medium"s;

        case std::to_underlying(window_size::large):
            return "Large"s;

        case std::to_underlying(window_size::very_large):
            return "Very Large"s;

        default:
            return "ERROR"s;
    }
}

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
        .name = box(std::string{"Scope"}),
        .bus_type = args.bus_type,
        .parameters =
            box(runtime::parameters_map{
                std::in_place_type<parameter_key>,
                {
                    {parameter_key::mode,
                     params_factory.make_parameter(
                         args.bus_type == audio::bus_type::mono
                             ? runtime::make_enum_parameter(
                                   "Mode",
                                   mode_mono::trigger,
                                   &to_mode_mono_string)
                             : runtime::make_enum_parameter(
                                   "Mode",
                                   mode_stereo::trigger_a,
                                   &to_mode_stereo_string))},
                    {parameter_key::trigger_slope,
                     params_factory.make_parameter(
                         runtime::make_enum_parameter(
                             "Slope",
                             trigger_slope::rising_edge,
                             &to_trigger_slope_string))},
                    {parameter_key::trigger_level,
                     params_factory.make_parameter(
                         runtime::make_float_parameter(
                             {
                                 .name = "Trigger "
                                         "Level",
                                 .default_value = 0.f,
                             },
                             runtime::
                                 linear_float_parameter_range<-1.f, 1.f>()))},
                    {parameter_key::hold_time,
                     params_factory.make_parameter(
                         runtime::make_float_parameter(
                             {
                                 .name = "Hold "
                                         "Time",
                                 .default_value = 80.f,
                             },
                             runtime::
                                 linear_float_parameter_range<16.f, 1600.f>())
                             .set_value_to_string(&to_hold_time_string))},
                    {parameter_key::waveform_window_size,
                     params_factory.make_parameter(
                         runtime::make_enum_parameter(
                             "Window Size",
                             window_size::large,
                             &to_window_size_string))},
                    {parameter_key::scope_window_size,
                     params_factory.make_parameter(
                         runtime::make_enum_parameter(
                             "Window Size",
                             window_size::very_small,
                             &to_window_size_string))},
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
                 make_stream(args.streams, num_channels(args.bus_type))},
            })};
}

} // namespace piejam::fx_modules::scope
