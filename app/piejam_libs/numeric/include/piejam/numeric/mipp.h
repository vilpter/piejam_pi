// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <mipp.h>

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace piejam::numeric
{

template <class T>
struct is_mipp_integral : std::false_type
{
};

template <>
struct is_mipp_integral<std::int8_t> : std::true_type
{
};

template <>
struct is_mipp_integral<std::int16_t> : std::true_type
{
};

template <>
struct is_mipp_integral<std::int32_t> : std::true_type
{
};

template <>
struct is_mipp_integral<std::int64_t> : std::true_type
{
};

template <>
struct is_mipp_integral<std::uint8_t> : std::true_type
{
};

template <>
struct is_mipp_integral<std::uint16_t> : std::true_type
{
};

template <>
struct is_mipp_integral<std::uint32_t> : std::true_type
{
};

template <>
struct is_mipp_integral<std::uint64_t> : std::true_type
{
};

template <class T>
constexpr bool is_mipp_integral_v =
    is_mipp_integral<std::remove_cvref_t<T>>::value;

template <class T>
concept mipp_integral = is_mipp_integral_v<T>;

template <class T>
concept mipp_signed_integral = mipp_integral<T> && std::is_signed_v<T>;

template <class T>
concept mipp_unsigned_integral = mipp_integral<T> && std::is_unsigned_v<T>;

template <class T>
concept mipp_number = mipp_integral<T> || std::floating_point<T>;

} // namespace piejam::numeric
