// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/parameter/flags.h>

#include <piejam/boxed_string.h>

#include <typeindex>

namespace piejam::runtime::parameter
{

template <class Tag, class Value>
struct descriptor
{
    using tag_t = Tag;
    using this_t = descriptor<Tag, Value>;

    using value_type = Value;
    using value_to_string_f = std::string (*)(value_type);
    using normalized_type = float;
    using to_normalized_f = normalized_type (*)(this_t const&, value_type);
    using from_normalized_f = value_type (*)(this_t const&, normalized_type);

    auto operator==(this_t const&) const noexcept -> bool = default;

    std::type_index type;

    boxed_string name;

    value_type default_value;

    value_type min;
    value_type max;

    value_to_string_f value_to_string;

    to_normalized_f to_normalized;
    from_normalized_f from_normalized;

    flags_set flags{};

    [[nodiscard]]
    constexpr auto
    set_value_to_string(value_to_string_f value_to_string) && -> this_t&&
    {
        this->value_to_string = value_to_string;
        return std::move(*this);
    }

    [[nodiscard]]
    constexpr auto
    set_to_normalized(to_normalized_f to_normalized) && -> this_t&&
    {
        this->to_normalized = to_normalized;
        return std::move(*this);
    }

    [[nodiscard]]
    constexpr auto
    set_from_normalized(from_normalized_f from_normalized) && -> this_t&&
    {
        this->from_normalized = from_normalized;
        return std::move(*this);
    }

    [[nodiscard]]
    constexpr auto set_flags(flags_set flags) && -> this_t&&
    {
        this->flags = flags;
        return std::move(*this);
    }
};

} // namespace piejam::runtime::parameter
