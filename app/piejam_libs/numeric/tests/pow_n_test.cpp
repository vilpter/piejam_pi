// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/numeric/pow_n.h>

#include <gtest/gtest.h>

namespace piejam::numeric::test
{

TEST(pow_n, integer_powers)
{
    EXPECT_EQ(pow_n<0>(5), 1);
    EXPECT_EQ(pow_n<1>(5), 5);
    EXPECT_EQ(pow_n<2>(5), 25);
    EXPECT_EQ(pow_n<3>(2), 8);
    EXPECT_EQ(pow_n<4>(3), 81);
}

TEST(pow_n, floating_point_powers)
{
    EXPECT_FLOAT_EQ(pow_n<0>(2.5f), 1.0f);
    EXPECT_FLOAT_EQ(pow_n<1>(2.5f), 2.5f);
    EXPECT_FLOAT_EQ(pow_n<2>(2.0f), 4.0f);
    EXPECT_FLOAT_EQ(pow_n<3>(1.5f), 3.375f);
    EXPECT_FLOAT_EQ(pow_n<4>(1.5f), 5.0625f);
}

TEST(pow_n, constexpr_evaluation)
{
    constexpr auto v1 = pow_n<3>(2);
    constexpr auto v2 = pow_n<4>(1.5);
    constexpr auto v3 = pow_n<0>(42);

    static_assert(v1 == 8);
    static_assert(std::abs(v2 - 5.0625) < 1e-10);
    static_assert(v3 == 1);

    EXPECT_EQ(v1, 8);
    EXPECT_NEAR(v2, 5.0625, 1e-12);
    EXPECT_EQ(v3, 1);
}

} // namespace piejam::numeric::test
