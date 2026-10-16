// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/io_direction.h>

#include <array>
#include <tuple>
#include <type_traits>
#include <utility>

namespace piejam
{

template <class T>
struct io_pair
{
    using value_type = T;

    constexpr io_pair() noexcept(std::is_nothrow_default_constructible_v<T>) =
        default;

    template <std::convertible_to<T> I, std::convertible_to<T> O>
    constexpr io_pair(I&& i, O&& o) noexcept(
        noexcept(T{std::forward<I>(i)}) && noexcept(T{std::forward<O>(o)}))
        : m_p{std::forward<I>(i), std::forward<O>(o)}
    {
    }

    constexpr io_pair(io_pair const&) = default;
    constexpr io_pair(io_pair&&) noexcept = default;

    constexpr auto operator=(io_pair const&) -> io_pair& = default;
    constexpr auto operator=(io_pair&&) noexcept -> io_pair& = default;

    auto operator==(io_pair const&) const noexcept -> bool = default;

    [[nodiscard]]
    constexpr auto operator[](io_direction d) const noexcept -> T const&
    {
        return m_p[std::to_underlying(d)];
    }

    [[nodiscard]]
    constexpr auto operator[](io_direction d) noexcept -> T&
    {
        return m_p[std::to_underlying(d)];
    }

    [[nodiscard]]
    constexpr auto in() & noexcept -> T&
    {
        return m_p[0];
    }

    [[nodiscard]]
    constexpr auto in() const& noexcept -> T const&
    {
        return m_p[0];
    }

    [[nodiscard]]
    constexpr auto out() & noexcept -> T&
    {
        return m_p[1];
    }

    [[nodiscard]]
    constexpr auto out() const& noexcept -> T const&
    {
        return m_p[1];
    }

    [[nodiscard]]
    constexpr auto begin() const noexcept
    {
        return m_p.begin();
    }

    [[nodiscard]]
    constexpr auto begin() noexcept
    {
        return m_p.begin();
    }

    [[nodiscard]]
    constexpr auto end() const noexcept
    {
        return m_p.end();
    }

    [[nodiscard]]
    constexpr auto end() noexcept
    {
        return m_p.end();
    }

private:
    std::array<T, 2> m_p{};
};

// get<0> -> in(), get<1> -> out()

template <std::size_t N, class T>
[[nodiscard]]
constexpr auto
get(io_pair<T>& p) noexcept -> T&
{
    static_assert(N < 2);
    return N == 0 ? p.in() : p.out();
}

template <std::size_t N, class T>
[[nodiscard]]
constexpr auto
get(io_pair<T> const& p) noexcept -> T const&
{
    static_assert(N < 2);
    return N == 0 ? p.in() : p.out();
}

} // namespace piejam

namespace std
{

template <class T>
struct tuple_size<piejam::io_pair<T>> : integral_constant<size_t, 2>
{
};

template <class T>
struct tuple_element<0, piejam::io_pair<T>>
{
    using type = T;
};

template <class T>
struct tuple_element<1, piejam::io_pair<T>>
{
    using type = T;
};

} // namespace std
