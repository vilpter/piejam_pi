// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/switch_cast.h>

#include <gtest/gtest.h>

namespace piejam::test
{

TEST(switch_cast, integral_values)
{
    int i = 42;
    long l = -123456;
    unsigned u = 99;

    EXPECT_EQ(switch_cast(i), i);
    EXPECT_EQ(switch_cast(l), l);
    EXPECT_EQ(switch_cast(u), u);
}

TEST(switch_cast, float_bit_cast)
{
    float f = 3.1415926f;
    auto bits = std::bit_cast<std::int32_t>(f);

    EXPECT_EQ(switch_cast(f), bits);
}

TEST(switch_cast, double_bit_cast)
{
    double d = 2.718281828;
    auto bits = std::bit_cast<std::int64_t>(d);

    EXPECT_EQ(switch_cast(d), bits);
}

TEST(switch_cast, constexpr_usage)
{
    constexpr int i = 123;
    constexpr float f = 1.23f;
    constexpr double d = 4.56;

    static_assert(switch_cast(i) == 123, "Integral constexpr failed");
    static_assert(
        switch_cast(f) == std::bit_cast<std::int32_t>(f),
        "Float constexpr failed");
    static_assert(
        switch_cast(d) == std::bit_cast<std::int64_t>(d),
        "Double constexpr failed");
}

} // namespace piejam::test
