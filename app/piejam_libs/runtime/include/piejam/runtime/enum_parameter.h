// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/parameter/descriptor.h>
#include <piejam/runtime/parameter/fwd.h>

#include <piejam/enum.h>
#include <piejam/numeric/normalize.h>

#include <boost/assert.hpp>

#include <utility>

namespace piejam::runtime
{

struct enum_parameter_tag;
using enum_parameter = parameter::descriptor<enum_parameter_tag, int>;
using enum_parameter_id = parameter::id_t<enum_parameter>;

template <scoped_enum<int> E>
auto
make_enum_parameter(
    std::string_view name,
    E default_value,
    enum_parameter::value_to_string_f value_to_string)
{
    return enum_parameter{
        .type = typeid(E),
        .name = box{std::string{name}},
        .default_value = std::to_underlying(default_value),
        .min = std::to_underlying(E::_min),
        .max = std::to_underlying(E::_max),
        .value_to_string = value_to_string,
        .to_normalized = [](auto const&, int x) noexcept -> float {
            return numeric::to_normalized<
                float,
                int,
                std::to_underlying(E::_min),
                std::to_underlying(E::_max)>(x);
        },
        .from_normalized = [](auto const&, float n) noexcept -> int {
            return numeric::from_normalized<
                float,
                int,
                std::to_underlying(E::_min),
                std::to_underlying(E::_max)>(n);
        },
    };
}

} // namespace piejam::runtime
