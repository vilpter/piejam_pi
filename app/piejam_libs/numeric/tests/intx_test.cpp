// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/numeric/intx.h>

#include <gtest/gtest.h>

namespace piejam::numeric::test
{

TEST(intx_io, default_constructor)
{
    int24_io_t x;
    EXPECT_EQ(static_cast<int>(x), 0);
}

TEST(intx_io, construct_from_integer)
{
    int24_io_t x{123456};
    EXPECT_EQ(static_cast<int>(x), 123456);

    int24_io_t y{-1000};
    EXPECT_EQ(static_cast<int>(y), -1000);
}

TEST(intx_io, construct_from_other_intx_io)
{
    int24_io_t x{5000};
    int24_io_t y{x};
    EXPECT_EQ(static_cast<int>(y), 5000);
}

TEST(intx_io, construct_from_float)
{
    float f = 1234.0f;
    int24_io_t x{f};
    EXPECT_EQ(static_cast<int>(x), 1234);
}

TEST(intx_io, comparison_operators)
{
    int24_io_t a{10};
    int24_io_t b{10};
    int24_io_t c{20};

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a <=> c == std::strong_ordering::less);
    EXPECT_TRUE(c <=> a == std::strong_ordering::greater);
}

TEST(intx_io, bitwise_xor)
{
    int24_io_t a{0b1010};
    int24_io_t b{0b0110};
    int24_io_t c = a ^ b;
    EXPECT_EQ(static_cast<int>(c), 0b1100);
}

TEST(intx_io, shift_left)
{
    int24_io_t a{0b1};
    int24_io_t b = a << 3;
    EXPECT_EQ(static_cast<int>(b), 0b1000);
}

TEST(intx_io, numeric_limits_signed)
{
    using limits = std::numeric_limits<int24_io_t>;
    EXPECT_TRUE(limits::is_integer);
    EXPECT_TRUE(limits::is_bounded);
    EXPECT_EQ(limits::digits, 23); // 24 bits - 1 sign bit
    EXPECT_EQ(static_cast<int>(limits::min()), -(1 << 23));
    EXPECT_EQ(static_cast<int>(limits::max()), (1 << 23) - 1);
}

TEST(intx_io, numeric_limits_unsigned)
{
    using limits = std::numeric_limits<uint24_io_t>;
    EXPECT_TRUE(limits::is_integer);
    EXPECT_TRUE(limits::is_bounded);
    EXPECT_EQ(limits::digits, 24);
    EXPECT_EQ(static_cast<int>(limits::min()), 0);
    EXPECT_EQ(static_cast<int>(limits::max()), (1 << 24) - 1);
}

TEST(intx_io, overflow_signed)
{
    int24_io_t max_val{(1 << 23) - 1};
    int24_io_t min_val{-(1 << 23)};

    // Overflow addition (wraps around in underlying storage)
    int24_io_t overflowed = max_val ^ int24_io_t { -1 };
    EXPECT_EQ(static_cast<int>(overflowed), ((1 << 23) - 1) ^ -1);

    int24_io_t underflowed = min_val ^ int24_io_t { -1 };
    EXPECT_EQ(static_cast<int>(underflowed), (-(1 << 23)) ^ -1);
}

TEST(intx_io, overflow_unsigned)
{
    uint24_io_t max_val{(1u << 24) - 1}; // 0xFFFFFF
    uint24_io_t shift_result = max_val << 1;

    // Only the lower 24 bits are stored
    unsigned expected = ((1u << 24) - 1) << 1 & 0xFFFFFF;

    EXPECT_EQ(static_cast<unsigned>(shift_result), expected);
}

TEST(intx_io, float_to_signed)
{
    // Arbitrary float values beyond signed 24-bit range
    float over = 9000000.f;   // > 2^23 - 1
    float under = -9000000.f; // < -2^23

    int24_io_t val_over{over};
    int24_io_t val_under{under};

    // Expect only the lower 24 bits are kept, interpreted as signed
    EXPECT_EQ(static_cast<int>(val_over), -7777216);
    EXPECT_EQ(static_cast<int>(val_under), 7777216);
}

TEST(intx_io, float_to_unsigned)
{
    // Arbitrary float values beyond 24-bit range
    float over = 25000000.f; // > 2^24
    float under = -1234.f;   // negative value

    uint24_io_t val_over{over};
    uint24_io_t val_under{under};

    // Expect only the lower 24 bits are kept
    EXPECT_EQ(static_cast<unsigned>(val_over), 8222784);
    EXPECT_EQ(static_cast<unsigned>(val_under), 16775982);
}

} // namespace piejam::numeric::test
