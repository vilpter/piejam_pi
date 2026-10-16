// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/processors/midi_to_parameter_processor.h>

#include <piejam/runtime/bool_parameter.h>
#include <piejam/runtime/enum_parameter.h>
#include <piejam/runtime/float_parameter.h>
#include <piejam/runtime/int_parameter.h>

#include <piejam/audio/engine/event_converter_processor.h>
#include <piejam/midi/event.h>
#include <piejam/range/iota.h>

#include <boost/assert.hpp>

#include <algorithm>
#include <array>
#include <memory>
#include <ranges>

namespace piejam::runtime::processors
{

namespace
{

template <class Parameter>
auto
make_cc_to_value_lut(Parameter const& param)
{
    constexpr std::size_t lut_size = 128;
    using value_type = typename Parameter::value_type;
    std::array<value_type, lut_size> result;

    std::ranges::transform(
        range::iota(std::size_t{lut_size}),
        result.begin(),
        [&param](std::size_t const cc_value) -> value_type {
            return param.from_normalized(param, cc_value / 127.f);
        });

    return result;
}

template <class Parameter>
constexpr auto
cc_processor_name() noexcept -> std::string_view
{
    if constexpr (std::is_same_v<Parameter, bool_parameter>)
    {
        return "cc_to_bool";
    }
    else if constexpr (std::is_same_v<Parameter, enum_parameter>)
    {
        return "cc_to_enum";
    }
    else if constexpr (std::is_same_v<Parameter, float_parameter>)
    {
        return "cc_to_float";
    }
    else if constexpr (std::is_same_v<Parameter, int_parameter>)
    {
        return "cc_to_int";
    }
    else
    {
        static_assert(
            std::is_same_v<Parameter, struct unsupported_parameter_tag>,
            "Unsupported parameter type");
    }
}

template <class Parameter>
auto
make_pitch_bend_to_value_lut(Parameter const& param)
{
    constexpr std::size_t lut_size = 16384;
    using value_type = typename Parameter::value_type;
    std::array<value_type, lut_size> result;

    constexpr std::int16_t pb_min = -8192;
    constexpr std::int16_t pb_max = 8191;

    std::ranges::transform(
        std::views::iota(pb_min, pb_max),
        result.begin(),
        [&param](std::int16_t const cc_value) -> value_type {
            return param.from_normalized(
                param,
                (static_cast<float>(cc_value) + 8192.f) / 16383.f);
        });

    return result;
}

template <class Parameter>
constexpr auto
pitch_bend_processor_name() noexcept -> std::string_view
{
    if constexpr (std::is_same_v<Parameter, bool_parameter>)
    {
        return "pb_to_bool";
    }
    else if constexpr (std::is_same_v<Parameter, enum_parameter>)
    {
        return "pb_to_enum";
    }
    else if constexpr (std::is_same_v<Parameter, float_parameter>)
    {
        return "pb_to_float";
    }
    else if constexpr (std::is_same_v<Parameter, int_parameter>)
    {
        return "pb_to_int";
    }
    else
    {
        static_assert(
            std::is_same_v<Parameter, struct unsupported_parameter_tag>,
            "Unsupported parameter type");
    }
}

} // namespace

template <class Parameter>
auto
make_midi_cc_to_parameter_processor(Parameter const& param)
    -> std::unique_ptr<audio::engine::processor>
{
    using value_type = typename Parameter::value_type;
    constexpr std::array s_input_names{std::string_view("cc in")};
    constexpr std::array s_output_names{std::string_view("out")};
    return audio::engine::make_event_converter_processor(
        [lut = make_cc_to_value_lut(param)](midi::cc_event const& cc_ev)
            -> value_type { return lut[cc_ev.value]; },
        s_input_names,
        s_output_names,
        cc_processor_name<Parameter>());
}

template auto make_midi_cc_to_parameter_processor(bool_parameter const&)
    -> std::unique_ptr<audio::engine::processor>;
template auto make_midi_cc_to_parameter_processor(enum_parameter const&)
    -> std::unique_ptr<audio::engine::processor>;
template auto make_midi_cc_to_parameter_processor(float_parameter const&)
    -> std::unique_ptr<audio::engine::processor>;
template auto make_midi_cc_to_parameter_processor(int_parameter const&)
    -> std::unique_ptr<audio::engine::processor>;

template <class Parameter>
auto
make_midi_pitch_bend_to_parameter_processor(Parameter const& param)
    -> std::unique_ptr<audio::engine::processor>
{
    using value_type = typename Parameter::value_type;
    constexpr std::array s_input_names{std::string_view("pb in")};
    constexpr std::array s_output_names{std::string_view("out")};
    return audio::engine::make_event_converter_processor(
        [lut = make_pitch_bend_to_value_lut(param)](
            midi::pitch_bend_event const& pb_ev) -> value_type {
            return lut[pb_ev.value + 8192];
        },
        s_input_names,
        s_output_names,
        pitch_bend_processor_name<Parameter>());
}

template auto make_midi_pitch_bend_to_parameter_processor(bool_parameter const&)
    -> std::unique_ptr<audio::engine::processor>;
template auto make_midi_pitch_bend_to_parameter_processor(enum_parameter const&)
    -> std::unique_ptr<audio::engine::processor>;
template auto
make_midi_pitch_bend_to_parameter_processor(float_parameter const&)
    -> std::unique_ptr<audio::engine::processor>;
template auto make_midi_pitch_bend_to_parameter_processor(int_parameter const&)
    -> std::unique_ptr<audio::engine::processor>;

} // namespace piejam::runtime::processors
