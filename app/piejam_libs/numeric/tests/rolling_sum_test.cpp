// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/numeric/simd/rolling_sum.h>

#include <gtest/gtest.h>

#include <numeric>
#include <vector>

namespace piejam::numeric::simd::test
{

template <typename T>
class rolling_sum_test : public ::testing::Test
{
};

using test_types = ::testing::Types<float, double>;
TYPED_TEST_SUITE(rolling_sum_test, test_types);

TYPED_TEST(rolling_sum_test, empty_sum_initially_zero)
{
    rolling_sum<std::identity, TypeParam> rs(4 * mipp::N<TypeParam>());
    EXPECT_EQ(rs.sum(), TypeParam{0});
}

TYPED_TEST(rolling_sum_test, full_window_update_computes_correct_sum)
{
    constexpr std::size_t window = 4 * mipp::N<TypeParam>();
    rolling_sum<std::identity, TypeParam> rs(window);

    std::vector<TypeParam> samples(window);
    std::iota(samples.begin(), samples.end(), TypeParam{1}); // 1..N

    auto s = rs.update(samples);
    TypeParam expected =
        std::accumulate(samples.begin(), samples.end(), TypeParam{0});

    EXPECT_NEAR(s, expected, TypeParam(1e-5));
    EXPECT_NEAR(rs.sum(), expected, TypeParam(1e-5));
}

TYPED_TEST(rolling_sum_test, rolling_update_sliding_window)
{
    constexpr std::size_t window = 2 * mipp::N<TypeParam>();
    rolling_sum<std::identity, TypeParam> rs(window);

    // fill with initial [1,2,...,window]
    std::vector<TypeParam> samples(window);
    std::iota(samples.begin(), samples.end(), TypeParam{1});
    rs.update(samples);

    // next update with [window+1, window+2]
    std::vector<TypeParam> next(mipp::N<TypeParam>()); // one SIMD chunk
    std::iota(next.begin(), next.end(), TypeParam(window + 1));
    auto s = rs.update(next);

    // expected: sum of last "window" samples
    std::vector<TypeParam> all(window + mipp::N<TypeParam>());
    std::iota(all.begin(), all.end(), TypeParam{1});
    TypeParam expected =
        std::accumulate(all.end() - window, all.end(), TypeParam{0});

    EXPECT_NEAR(s, expected, TypeParam(1e-5));
}

TYPED_TEST(rolling_sum_test, reset_sets_sum_to_zero)
{
    constexpr std::size_t window = 2 * mipp::N<TypeParam>();
    rolling_sum<std::identity, TypeParam> rs(window);

    std::vector<TypeParam> samples(window);
    std::iota(samples.begin(), samples.end(), TypeParam{1});
    rs.update(samples);

    EXPECT_GT(rs.sum(), TypeParam{0});

    rs.reset();
    EXPECT_EQ(rs.sum(), TypeParam{0});
}

TYPED_TEST(rolling_sum_test, multiple_partial_updates_cover_window)
{
    constexpr std::size_t window = 4 * mipp::N<TypeParam>();
    rolling_sum<std::identity, TypeParam> rs(window);

    // First half of window
    std::vector<TypeParam> part1(2 * mipp::N<TypeParam>());
    std::iota(part1.begin(), part1.end(), TypeParam{1});
    rs.update(part1);

    // Second half
    std::vector<TypeParam> part2(2 * mipp::N<TypeParam>());
    std::iota(part2.begin(), part2.end(), TypeParam(part1.back() + 1));
    auto s = rs.update(part2);

    // expected sum of [1..window]
    std::vector<TypeParam> all(window);
    std::iota(all.begin(), all.end(), TypeParam{1});
    TypeParam expected = std::accumulate(all.begin(), all.end(), TypeParam{0});

    EXPECT_NEAR(s, expected, TypeParam(1e-5));
}

} // namespace piejam::numeric::simd::test
