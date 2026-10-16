// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/numeric/clamp.h>
#include <piejam/numeric/simd/clamp.h>

#include <gtest/gtest.h>

namespace piejam::numeric::test
{

TEST(clamp, int_)
{
    EXPECT_EQ(clamp(5, 2, 4), 4);
    EXPECT_EQ(clamp(5, 6, 10), 6);
    EXPECT_EQ(clamp(5, 2, 10), 5);

    static_assert(clamp(5, 2, 4) == 4);
    static_assert(clamp(5, 6, 10) == 6);
    static_assert(clamp(5, 2, 10) == 5);
}

TEST(clamp, float_)
{
    EXPECT_FLOAT_EQ(clamp(2.5f, 1.0f, 2.0f), 2.0f);
    EXPECT_FLOAT_EQ(clamp(2.5f, 3.0f, 4.0f), 3.0f);
    EXPECT_FLOAT_EQ(clamp(2.5f, 1.0f, 4.0f), 2.5f);

    static_assert(clamp(2.5f, 1.0f, 2.0f) == 2.0f);
    static_assert(clamp(2.5f, 3.0f, 4.0f) == 3.0f);
    static_assert(clamp(2.5f, 1.0f, 4.0f) == 2.5f);
}

TEST(simd_clamp, float_)
{
    mipp::Reg<float> min(4.f);
    mipp::Reg<float> max(7.f);

    mipp::Reg<float> value({2.f, 5.f, 6.f, 9.f});

    auto result = simd::clamp(value, min, max);

    EXPECT_FLOAT_EQ(result[0], 4.f);
    EXPECT_FLOAT_EQ(result[1], 5.f);
    EXPECT_FLOAT_EQ(result[2], 6.f);
    EXPECT_FLOAT_EQ(result[3], 7.f);
}

} // namespace piejam::numeric::test
