// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/numeric/mipp.h>

#include <boost/assert.hpp>
#include <boost/stl_interfaces/iterator_interface.hpp>

#include <ranges>
#include <span>

namespace piejam::numeric
{

namespace detail
{

template <mipp_number T>
struct mipp_iterator_output_proxy
{
    using value_type = std::remove_cv_t<T>;

    constexpr mipp_iterator_output_proxy() noexcept = default;
    constexpr mipp_iterator_output_proxy(T* p) noexcept
        : m_p{p}
    {
        BOOST_ASSERT(mipp::isAligned(m_p));
    }

    operator mipp::Reg<value_type>() const
    {
        return m_p;
    }

    auto
    operator=(mipp::Reg<value_type> const& r) & -> mipp_iterator_output_proxy&
        requires(!std::is_const_v<T>)
    {
        r.store(m_p);
        return *this;
    }

    auto operator=(
        mipp::Reg<value_type> const& r) const&& -> mipp_iterator_output_proxy
        requires(!std::is_const_v<T>)
    {
        r.store(m_p);
        return *this;
    }

    auto
    operator=(mipp::Regx2<value_type> const& r) & -> mipp_iterator_output_proxy&
        requires(!std::is_const_v<T>)
    {
        r[0].store(m_p);
        r[1].store(m_p + mipp::N<T>());
        return *this;
    }

    auto operator=(
        mipp::Regx2<value_type> const& r) const&& -> mipp_iterator_output_proxy
        requires(!std::is_const_v<T>)
    {
        r[0].store(m_p);
        r[1].store(m_p + mipp::N<T>());
        return *this;
    }

private:
    T* m_p{};
};

template <template <class> class Reg>
struct num_regs;

template <>
struct num_regs<mipp::Reg>
{
    static constexpr std::size_t value{1};
};

template <>
struct num_regs<mipp::Regx2>
{
    static constexpr std::size_t value{2};
};

template <template <class> class Reg>
constexpr std::size_t num_regs_v = num_regs<Reg>::value;

} // namespace detail

template <mipp_number T, template <class> class Reg = mipp::Reg>
struct mipp_iterator
    : boost::stl_interfaces::iterator_interface<
          std::contiguous_iterator_tag,
          Reg<std::remove_cv_t<T>>,
          std::conditional_t<
              std::is_const_v<T>,
              Reg<std::remove_cv_t<T>>,
              detail::mipp_iterator_output_proxy<T>>,
          T*>
{
    static constexpr auto N =
        mipp::N<std::remove_cv_t<T>>() * detail::num_regs_v<Reg>;

    constexpr mipp_iterator() noexcept = default;
    constexpr mipp_iterator(T* p) noexcept
        : m_p(p)
    {
        BOOST_ASSERT(mipp::isAligned(m_p));
    }

    [[nodiscard]]
    constexpr auto operator*() const noexcept -> Reg<std::remove_cv_t<T>>
    {
        return m_p;
    }

    [[nodiscard]]
    constexpr auto operator*() noexcept -> detail::mipp_iterator_output_proxy<T>
        requires(!std::is_const_v<T>)
    {
        return m_p;
    }

    constexpr auto operator+=(std::ptrdiff_t n) noexcept -> mipp_iterator&
    {
        m_p += n * N;
        return *this;
    }

    [[nodiscard]]
    constexpr auto operator-(mipp_iterator const& rhs) const noexcept
        -> std::ptrdiff_t
    {
        BOOST_ASSERT((m_p - rhs.m_p) % N == 0);
        return (m_p - rhs.m_p) / N;
    }

    [[nodiscard]]
    constexpr auto operator==(mipp_iterator const& rhs) const noexcept -> bool
    {
        return m_p == rhs.m_p;
    }

    [[nodiscard]]
    constexpr auto operator<(mipp_iterator const& rhs) const noexcept -> bool
    {
        return m_p < rhs.m_p;
    }

private:
    T* m_p{};
};

template <mipp_number T>
mipp_iterator(T*) -> mipp_iterator<T>;

template <mipp_number T>
mipp_iterator(T const*) -> mipp_iterator<T const>;

template <mipp_number T>
[[nodiscard]]
constexpr auto
make_mipp_iterator_x2(T* p)
{
    return mipp_iterator<T, mipp::Regx2>{p};
}

template <mipp_number T>
[[nodiscard]]
constexpr auto
make_mipp_iterator_x2(T const* p)
{
    return mipp_iterator<T const, mipp::Regx2>{p};
}

template <std::ranges::contiguous_range R>
    requires mipp_number<std::ranges::range_value_t<R>>
[[nodiscard]]
constexpr auto
mipp_begin(R&& in)
{
    return mipp_iterator{std::ranges::data(in)};
}

template <std::ranges::contiguous_range R>
    requires mipp_number<std::ranges::range_value_t<R>>
[[nodiscard]]
constexpr auto
mipp_end(R&& in)
{
    return mipp_iterator{std::ranges::data(in) + std::ranges::size(in)};
}

template <std::ranges::contiguous_range R>
    requires mipp_number<std::ranges::range_value_t<R>>
[[nodiscard]]
constexpr auto
mipp_range(R&& in)
{
    auto const data = std::ranges::data(in);

    return std::ranges::subrange{
        mipp_iterator{data},
        mipp_iterator{data + std::ranges::size(in)}};
}

template <std::ranges::contiguous_range R>
    requires mipp_number<std::ranges::range_value_t<R>>
[[nodiscard]]
constexpr auto
mipp_range_split(R&& in)
{
    using T = std::ranges::range_value_t<R>;
    constexpr auto N = mipp::N<T>();

    auto const in_data = std::ranges::data(in);
    auto const in_size = std::ranges::size(in);

    auto const addr = reinterpret_cast<std::uintptr_t>(in_data);
    auto const misalignment = (addr / sizeof(T)) % N;

    auto const pre_size = std::min(in_size, (N - misalignment) % N);
    auto const pre_data = in_data;

    auto const size_without_pre = in_size - pre_size;
    auto const main_size = (size_without_pre / N) * N;
    auto const main_data = pre_data + pre_size;

    auto const post_size = size_without_pre - main_size;
    auto const post_data = main_data + main_size;

    return std::tuple{
        std::span{pre_data, pre_size},
        std::span{main_data, main_size},
        std::span{post_data, post_size},
    };
}

} // namespace piejam::numeric
