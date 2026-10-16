// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/numeric/align.h>

#include <gtest/gtest.h>

namespace piejam::numeric::test
{

static_assert(align_down(10u, 5u) == 10u);
static_assert(align_down(11u, 5u) == 10u);
static_assert(align_down(3u, 5u) == 0u);
static_assert(align_down(123u, 1u) == 123u);
static_assert(align_down(255u, static_cast<unsigned short>(16)) == 240u);

TEST(align_down, rounds_down_exact_multiple)
{
    EXPECT_EQ(align_down(10u, 5u), 10u);
    EXPECT_EQ(align_down(42u, 7u), 42u);
    EXPECT_EQ(align_down(64u, 8u), 64u);
}

TEST(align_down, rounds_down_non_multiple)
{
    EXPECT_EQ(align_down(11u, 5u), 10u);
    EXPECT_EQ(align_down(43u, 7u), 42u);
    EXPECT_EQ(align_down(65u, 8u), 64u);
}

TEST(align_down, rounds_down_to_zero_if_less_than_divisor)
{
    EXPECT_EQ(align_down(3u, 5u), 0u);
    EXPECT_EQ(align_down(1u, 2u), 0u);
}

TEST(align_down, rounds_down_when_divisor_is_one)
{
    EXPECT_EQ(align_down(123u, 1u), 123u);
    EXPECT_EQ(align_down(0u, 1u), 0u);
}

TEST(align_down, large_numbers)
{
    constexpr unsigned long long big = 1ull << 40ull; // 1 TB-ish
    EXPECT_EQ(align_down(big + 123ull, 1024ull), big + 0ull);
    EXPECT_EQ(align_down(big + 1023ull, 1024ull), big + 0ull);
    EXPECT_EQ(align_down(big + 1024ull, 1024ull), big + 1024ull);
}

TEST(align_down, works_with_different_unsigned_types)
{
    EXPECT_EQ(align_down(100ull, 6u), 96ull);
    EXPECT_EQ(align_down(255u, static_cast<unsigned short>(16)), 240u);
}

static_assert(align_up(0u, 4u) == 0u);
static_assert(align_up(1u, 4u) == 4u);
static_assert(align_up(4u, 4u) == 4u);
static_assert(align_up(5u, 4u) == 8u);
static_assert(align_up(15u, 8u) == 16u);

TEST(align_up, zero_aligned)
{
    EXPECT_EQ(align_up(0u, 1u), 0u);
    EXPECT_EQ(align_up(0u, 4u), 0u);
    EXPECT_EQ(align_up(0u, 16u), 0u);
}

TEST(align_up, already_aligned)
{
    EXPECT_EQ(align_up(8u, 4u), 8u);
    EXPECT_EQ(align_up(16u, 8u), 16u);
    EXPECT_EQ(align_up(32u, 16u), 32u);
}

TEST(align_up, not_aligned)
{
    EXPECT_EQ(align_up(5u, 4u), 8u);
    EXPECT_EQ(align_up(7u, 4u), 8u);
    EXPECT_EQ(align_up(9u, 4u), 12u);

    EXPECT_EQ(align_up(17u, 8u), 24u);
    EXPECT_EQ(align_up(19u, 8u), 24u);
}

TEST(align_up, with_large_numbers)
{
    EXPECT_EQ(align_up(1000u, 256u), 1024u);
    EXPECT_EQ(align_up(4097u, 4096u), 8192u);
}

TEST(align_up, with_different_unsigned_types)
{
    EXPECT_EQ(align_up(std::uint8_t{7}, std::uint8_t{4}), std::uint8_t{8});
    EXPECT_EQ(
        align_up(std::uint16_t{1025}, std::uint16_t{256}),
        std::uint16_t{1280});
    EXPECT_EQ(
        align_up(std::uint64_t{123456789}, std::uint64_t{1024}),
        std::uint64_t{123457536});
}

static_assert(align_nearest(3u, 4u) == 4u);
static_assert(align_nearest(5u, 4u) == 4u);
static_assert(align_nearest(6u, 4u) == 8u);
static_assert(align_nearest(7u, 4u) == 8u);
static_assert(align_nearest(2u, 4u) == 4u); // halfway → up
static_assert(align_nearest(uint64_t{33}, uint64_t{10}) == 30u);
static_assert(align_nearest(uint16_t{47}, uint16_t{10}) == 50u);

TEST(align_nearest_test, aligns_down_when_closer_to_lower_multiple)
{
    EXPECT_EQ(align_nearest(3u, 4u), 4u); // 3 is closer to 4 than to 0
    EXPECT_EQ(align_nearest(5u, 4u), 4u); // 5 is closer to 4 than to 8
    EXPECT_EQ(align_nearest(6u, 4u), 8u); // 6 is closer to 8 than to 4
}

TEST(align_nearest_test, aligns_up_when_closer_to_upper_multiple)
{
    EXPECT_EQ(align_nearest(7u, 4u), 8u);
    EXPECT_EQ(align_nearest(9u, 4u), 8u);
    EXPECT_EQ(align_nearest(10u, 4u), 12u);
}

TEST(align_nearest_test, ties_round_up)
{
    EXPECT_EQ(align_nearest(2u, 4u), 4u); // exactly halfway, rounds up
    EXPECT_EQ(align_nearest(6u, 8u), 8u); // halfway case
}

TEST(align_nearest_test, works_with_different_types)
{
    EXPECT_EQ(align_nearest(uint64_t{33}, uint64_t{10}), uint64_t{30});
    EXPECT_EQ(align_nearest(uint16_t{47}, uint16_t{10}), uint16_t{50});
}

} // namespace piejam::numeric::test
