// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/numeric/pos_mod.h>

#include <gtest/gtest.h>

namespace piejam::numeric::test
{

TEST(pos_mod_test, unsigned_inputs_wrap_correctly)
{
    EXPECT_EQ(pos_mod(5u, 3u), 2u);
    EXPECT_EQ(pos_mod(3u, 3u), 0u);
    EXPECT_EQ(pos_mod(0u, 3u), 0u);
    EXPECT_EQ(pos_mod(8u, 5u), 3u);
}

TEST(pos_mod_test, signed_inputs_nonnegative_result)
{
    EXPECT_EQ(pos_mod(5, 3), 2);
    EXPECT_EQ(pos_mod(3, 3), 0);
    EXPECT_EQ(pos_mod(0, 3), 0);

    EXPECT_EQ(pos_mod(-1, 3), 2);
    EXPECT_EQ(pos_mod(-4, 3), 2);
    EXPECT_EQ(pos_mod(-7, 3), 2);
}

TEST(pos_mod_test, signed_inputs_positive_and_negative_y)
{
    EXPECT_EQ(pos_mod(-10, 7), 4);
    EXPECT_EQ(pos_mod(10, 7), 3);

    EXPECT_EQ(pos_mod(-10, -7), 4);
    EXPECT_EQ(pos_mod(10, -7), 3);
}

TEST(pos_mod_test, constexpr_evaluation)
{
    constexpr auto r1 = pos_mod(5, 3);
    constexpr auto r2 = pos_mod(-1, 3);
    constexpr auto r3 = pos_mod(8u, 5u);

    static_assert(r1 == 2);
    static_assert(r2 == 2);
    static_assert(r3 == 3);
}

} // namespace piejam::numeric::test
