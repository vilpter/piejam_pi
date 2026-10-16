// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/numeric/mipp_iterator.h>

#include <boost/assert.hpp>

#include <algorithm>
#include <concepts>
#include <functional>
#include <span>
#include <tuple>

namespace piejam::numeric::simd
{

template <class Tr = std::identity, std::floating_point T = float>
class rolling_sum
{

public:
    using value_type = mipp::Reg<T>;

    explicit constexpr rolling_sum(std::size_t window_size)
        : m_window(window_size)
        , m_sum(T{})
    {
        BOOST_ASSERT(window_size % mipp::N<T>() == 0);
    }

    [[nodiscard]]
    constexpr auto sum() const noexcept -> T
    {
        return mipp::sum(m_sum);
    }

    [[nodiscard]]
    constexpr auto window_size() const noexcept -> std::size_t
    {
        return m_window.size();
    }

    template <std::ranges::contiguous_range R>
        requires std::same_as<std::ranges::range_value_t<R>, T>
    constexpr auto update(R const& samples) noexcept -> T
    {
        auto const samples_data = std::ranges::data(samples);
        BOOST_ASSERT(mipp::isAligned(samples_data));

        std::size_t const samples_size = std::ranges::size(samples);
        BOOST_ASSERT(samples_size % mipp::N<T>() == 0);

        std::size_t const window_size = m_window.size();
        BOOST_ASSERT(m_pos % mipp::N<T>() == 0);

        if (samples_size < window_size)
        {
            auto [lo, hi] = ring_buffer_split(samples_size);
            auto mid_it = mipp_iterator{samples_data + lo.size()};

            update_segment(mipp_begin(samples), mid_it, mipp_begin(lo));
            update_segment(mid_it, mipp_end(samples), mipp_begin(hi));

            advance_position(samples_size);
        }
        else
        {
            auto samples_first =
                mipp_iterator{samples_data + samples_size - window_size};
            auto samples_last = mipp_iterator{samples_data + samples_size};

            mipp::Reg<T> acc(T{});
            for (auto sum_it = mipp_begin(m_window);
                 samples_first != samples_last;
                 ++samples_first, ++sum_it)
            {
                auto t = Tr{}(*samples_first);
                acc += t;
                *sum_it = t;
            }

            m_sum = acc;
            m_pos = 0;
        }

        return mipp::sum(m_sum);
    }

    constexpr void reset()
    {
        std::ranges::fill(m_window, T{0});
        m_pos = 0;
        m_sum = mipp::Reg<T>(T{});
    }

private:
    template <class SamplesIterator, class SumIterator>
    constexpr void update_segment(
        SamplesIterator samples_first,
        SamplesIterator samples_last,
        SumIterator sum_first) noexcept
    {
        mipp::Reg<T> sub(T{});
        mipp::Reg<T> add(T{});

        for (; samples_first != samples_last; ++samples_first, ++sum_first)
        {
            sub += *sum_first;
            auto new_summand = Tr{}(*samples_first);
            add += new_summand;
            *sum_first = new_summand;
        }

        m_sum = m_sum - sub + add;
    }

    constexpr void advance_position(std::size_t num_samples) noexcept
    {
        m_pos += num_samples;

        std::size_t const window_size = m_window.size();
        if (m_pos >= window_size)
        {
            m_pos -= window_size;
        }
    }

    constexpr auto ring_buffer_split(std::size_t num_samples) noexcept
        -> std::tuple<std::span<T>, std::span<T>>
    {
        auto const window_data = m_window.data();
        auto const window_size = m_window.size();
        auto const first = window_data + m_pos;

        if (m_pos + num_samples < window_size)
        {
            auto last = first + num_samples;
            return std::tuple{std::span{first, last}, std::span{last, last}};
        }
        else
        {
            auto size = window_size - m_pos;
            return std::tuple{
                std::span{first, size},
                std::span{window_data, num_samples - size}};
        }
    }

    mipp::vector<T> m_window;
    std::size_t m_pos{};

    mipp::Reg<T> m_sum;
};

} // namespace piejam::numeric::simd
