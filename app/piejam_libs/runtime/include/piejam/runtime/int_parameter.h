// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/parameter/descriptor.h>
#include <piejam/runtime/parameter/fwd.h>

#include <piejam/numeric/normalize.h>

#include <boost/assert.hpp>

#include <string_view>

namespace piejam::runtime
{

struct int_parameter_tag;
using int_parameter = parameter::descriptor<int_parameter_tag, int>;
using int_parameter_id = parameter::id_t<int_parameter>;

struct int_parameter_args
{
    std::string_view name;
    int default_value;
    int min;
    int max;
};

inline auto
make_int_parameter(int_parameter_args args)
{
    BOOST_ASSERT(args.min < args.max);
    BOOST_ASSERT(args.min <= args.default_value);
    BOOST_ASSERT(args.default_value <= args.max);
    return int_parameter{
        .type = typeid(int),
        .name = box{std::string{args.name}},
        .default_value = args.default_value,
        .min = args.min,
        .max = args.max,
        .value_to_string = [](int x) { return std::to_string(x); },
        .to_normalized = [](auto const& d, int x) noexcept -> float {
            return numeric::to_normalized<float>(x, d.min, d.max);
        },
        .from_normalized = [](auto const& d, float n) noexcept -> int {
            return numeric::from_normalized(n, d.min, d.max);
        },
    };
}

} // namespace piejam::runtime
