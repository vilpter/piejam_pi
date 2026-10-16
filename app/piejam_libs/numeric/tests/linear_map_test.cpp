// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/numeric/linear_map.h>

#include <gtest/gtest.h>

namespace piejam::numeric::test
{

TEST(linear_map, maps_basic_float)
{
    float v = 0.5f;
    float result = linear_map(v, 0.f, 1.f, 0.f, 10.f);
    EXPECT_FLOAT_EQ(result, 5.f);

    result = linear_map(v, 0.f, 1.f, -1.f, 1.f);
    EXPECT_FLOAT_EQ(result, 0.f);
}

TEST(linear_map, maps_basic_double)
{
    double v = 2.0;
    double result = linear_map(v, 0.0, 4.0, 0.0, 8.0);
    EXPECT_DOUBLE_EQ(result, 4.0);

    result = linear_map(v, 0.0, 4.0, -1.0, 1.0);
    EXPECT_DOUBLE_EQ(result, 0.0);
}

TEST(linear_map, curried_float)
{
    auto mapper = linear_map(0.f, 1.f, 0.f, 10.f);
    EXPECT_FLOAT_EQ(mapper(0.f), 0.f);
    EXPECT_FLOAT_EQ(mapper(0.5f), 5.f);
    EXPECT_FLOAT_EQ(mapper(1.f), 10.f);
}

TEST(linear_map, curried_double)
{
    auto mapper = linear_map(0.0, 4.0, -1.0, 1.0);
    EXPECT_DOUBLE_EQ(mapper(0.0), -1.0);
    EXPECT_DOUBLE_EQ(mapper(2.0), 0.0);
    EXPECT_DOUBLE_EQ(mapper(4.0), 1.0);
}

TEST(linear_map, negative_ranges)
{
    auto mapper = linear_map(-2.0, 2.0, 0.0, 1.0);
    EXPECT_DOUBLE_EQ(mapper(-2.0), 0.0);
    EXPECT_DOUBLE_EQ(mapper(0.0), 0.5);
    EXPECT_DOUBLE_EQ(mapper(2.0), 1.0);
}

} // namespace piejam::numeric::test
