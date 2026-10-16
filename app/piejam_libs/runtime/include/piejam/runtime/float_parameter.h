// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/parameter/descriptor.h>
#include <piejam/runtime/parameter/fwd.h>

#include <piejam/numeric/clamp.h>
#include <piejam/numeric/dB_convert.h>
#include <piejam/numeric/normalize.h>

#include <boost/assert.hpp>

#include <format>
#include <string_view>

namespace piejam::runtime
{

struct float_parameter_tag;
using float_parameter = parameter::descriptor<float_parameter_tag, float>;
using float_parameter_id = parameter::id_t<float_parameter>;

inline auto
default_float_to_string(float x) -> std::string
{
    return std::format("{:.2f}", x);
}

inline auto
default_float_to_dB_string(float x) -> std::string
{
    return std::format("{:.1f} dB", numeric::to_dB(x));
}

struct float_parameter_args
{
    std::string_view name;
    float default_value;
};

struct float_parameter_range
{
    float min;
    float max;
    float_parameter::to_normalized_f to_normalized;
    float_parameter::from_normalized_f from_normalized;
};

constexpr auto
linear_float_parameter_range(float min, float max) noexcept
{
    return float_parameter_range{
        .min = min,
        .max = max,
        .to_normalized = [](auto const& d, float x) noexcept -> float {
            return numeric::clamp(
                numeric::to_normalized<float>(x, d.min, d.max),
                0.f,
                1.f);
        },
        .from_normalized = [](auto const& d, float n) noexcept -> float {
            return numeric::clamp(
                numeric::from_normalized(n, d.min, d.max),
                d.min,
                d.max);
        },
    };
}

template <float Min, float Max>
constexpr auto
linear_float_parameter_range() noexcept
{
    static_assert(Min < Max);
    return float_parameter_range{
        .min = Min,
        .max = Max,
        .to_normalized = [](auto const&, float x) noexcept -> float {
            return numeric::clamp(
                numeric::to_normalized<float, float, Min, Max>(x),
                0.f,
                1.f);
        },
        .from_normalized = [](auto const&, float n) noexcept -> float {
            return numeric::clamp(
                numeric::from_normalized<float, float, Min, Max>(n),
                Min,
                Max);
        },
    };
}

constexpr auto
logarithmic_float_parameter_range(float min, float max) noexcept
{
    BOOST_ASSERT(min > 0.f);
    BOOST_ASSERT(max > 0.f);
    return float_parameter_range{
        .min = min,
        .max = max,
        .to_normalized = [](auto const& d, float x) noexcept -> float {
            return numeric::clamp(
                numeric::to_normalized<float>(
                    std::log(x),
                    std::log(d.min),
                    std::log(d.max)),
                0.f,
                1.f);
        },
        .from_normalized = [](auto const& d, float n) noexcept -> float {
            return numeric::clamp(
                std::exp(
                    numeric::from_normalized(
                        n,
                        std::log(d.min),
                        std::log(d.max))),
                d.min,
                d.max);
        },
    };
}

template <float Min, float Max>
constexpr auto
logarithmic_float_parameter_range() noexcept
{
    static_assert(Min > 0.f);
    static_assert(Max > 0.f);
    static_assert(Min < Max);
    return float_parameter_range{
        .min = Min,
        .max = Max,
        .to_normalized = [](auto const&, float x) noexcept -> float {
            return numeric::clamp(
                numeric::
                    to_normalized<float, float, std::log(Min), std::log(Max)>(
                        std::log(x)),
                0.f,
                1.f);
        },
        .from_normalized = [](auto const&, float n) noexcept -> float {
            return numeric::clamp(
                std::exp(
                    numeric::from_normalized<
                        float,
                        float,
                        std::log(Min),
                        std::log(Max)>(n)),
                Min,
                Max);
        },
    };
}

template <float Min, float Max>
constexpr auto
dB_float_parameter_range() noexcept
{
    return logarithmic_float_parameter_range<
        numeric::from_dB(Min),
        numeric::from_dB(Max)>();
}

inline auto
make_float_parameter(float_parameter_args args, float_parameter_range rng)
{
    BOOST_ASSERT(rng.min < rng.max);
    BOOST_ASSERT(rng.min <= args.default_value);
    BOOST_ASSERT(args.default_value <= rng.max);
    return float_parameter{
        .type = typeid(float),
        .name = box{std::string{args.name}},
        .default_value = args.default_value,
        .min = rng.min,
        .max = rng.max,
        .value_to_string = &default_float_to_string,
        .to_normalized = rng.to_normalized,
        .from_normalized = rng.from_normalized,
    };
}

} // namespace piejam::runtime
