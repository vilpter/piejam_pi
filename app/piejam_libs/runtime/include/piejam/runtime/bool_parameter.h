// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/parameter/descriptor.h>
#include <piejam/runtime/parameter/fwd.h>

#include <boost/assert.hpp>

#include <string_view>

namespace piejam::runtime
{

struct bool_parameter_tag;
using bool_parameter = parameter::descriptor<bool_parameter_tag, bool>;
using bool_parameter_id = parameter::id_t<bool_parameter>;

struct bool_parameter_args
{
    std::string_view name;
    bool default_value{};
};

inline auto
make_bool_parameter(bool_parameter_args args)
{
    return bool_parameter{
        .type = typeid(bool),
        .name = box{std::string{args.name}},
        .default_value = args.default_value,
        .min = false,
        .max = true,
        .value_to_string =
            [](bool x) {
                using namespace std::string_literals;
                return x ? "on"s : "off"s;
            },
        .to_normalized = [](auto const&, bool x) noexcept -> float {
            return x;
        },
        .from_normalized = [](auto const&, float n) noexcept -> bool {
            BOOST_ASSERT(0.f <= n && n <= 1.f);
            return n >= 0.5f;
        }};
}

} // namespace piejam::runtime
