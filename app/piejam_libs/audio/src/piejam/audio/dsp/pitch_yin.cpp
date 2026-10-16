// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/audio/dsp/pitch_yin.h>

#include <piejam/numeric/mipp_iterator.h>
#include <piejam/numeric/pow_n.h>
#include <piejam/numeric/simd/fsqradd.h>
#include <piejam/numeric/simd/lrot_n.h>

#include <mipp.h>

#include <boost/hof/compose.hpp>

#include <array>
#include <numeric>
#include <utility>

namespace piejam::audio::dsp
{

namespace
{

template <std::size_t N>
std::array<mipp::Msk<N>, N> precomputed_masks =
    []<std::size_t... Offset>(std::index_sequence<Offset...>) {
        return std::array{[]<std::size_t... I>(
                              std::index_sequence<I...>,
                              std::size_t offset) {
            return mipp::Msk<N>{I < (N - offset)...};
        }(std::make_index_sequence<N>{}, Offset)...};
    }(std::make_index_sequence<N>{});

template <std::floating_point T>
[[nodiscard]]
constexpr auto
sqr_difference_sum(
    std::span<T const> const in,
    std::size_t const e,
    std::size_t const tau) -> T
{
    constexpr auto N = mipp::N<T>();
    auto const data = in.data();
    auto const offset = tau % N;
    if (offset == 0)
    {
        return mipp::sum(
            std::transform_reduce(
                numeric::mipp_iterator{data},
                numeric::mipp_iterator{data + e},
                numeric::mipp_iterator{data + tau},
                mipp::Reg<T>(T{}),
                std::plus<>{},
                boost::hof::compose(numeric::pow_n<2>, std::minus<>{})));
    }
    else
    {
        mipp::Reg<T> sums(T{});

        auto mask = precomputed_masks<N>[offset];

        numeric::mipp_iterator it_tau{data + tau - offset};
        mipp::Reg<T> reg_lo = numeric::simd::lrot_n(*it_tau, offset);

        for (auto reg_i : std::ranges::subrange(
                 numeric::mipp_iterator{data},
                 numeric::mipp_iterator{data + e}))
        {
            ++it_tau;
            mipp::Reg<T> reg_hi = numeric::simd::lrot_n(*it_tau, offset);

            sums = numeric::simd::fsqradd(
                reg_i - mipp::select(mask, reg_lo, reg_hi),
                sums);

            reg_lo = reg_hi;
        }

        return mipp::sum(sums);
    }
}

} // namespace

template <std::floating_point T>
[[nodiscard]]
auto
pitch_yin(std::span<T const> const in, sample_rate const sr) -> T
{
    constexpr T threshold{0.1};

    std::size_t const e = in.size() / 2;
    T prev_cmn = T{1};
    T curr_cmn = T{1};
    T cumulative_sum{};
    for (std::size_t tau = 1; tau < e; ++tau)
    {
        T const sum = sqr_difference_sum(in, e, tau);

        cumulative_sum += sum;
        T const next_cmn = tau * sum / cumulative_sum;

        if (curr_cmn < threshold && curr_cmn < next_cmn)
        {
            // quadratic interpolation
            T const denom = T{2} * (prev_cmn - T{2} * curr_cmn + next_cmn);
            BOOST_ASSERT(std::abs(denom) > std::numeric_limits<T>::epsilon());
            T const betterTau = (tau - 1) + (prev_cmn - next_cmn) / denom;
            return sr.as<T>() / betterTau;
        }

        prev_cmn = curr_cmn;
        curr_cmn = next_cmn;
    }

    return T{};
}

template auto pitch_yin<float>(std::span<float const>, sample_rate) -> float;

} // namespace piejam::audio::dsp
