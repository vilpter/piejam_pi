// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/numeric/normalize.h>

#include <gtest/gtest.h>

namespace piejam::numeric::test
{

TEST(normalize, to_normalized_float_zero_to_one)
{
    float value = 0.0f;
    EXPECT_FLOAT_EQ(to_normalized<float>(value, 0.0f, 1.0f), 0.0f);
    value = 1.0f;
    EXPECT_FLOAT_EQ(to_normalized<float>(value, 0.0f, 1.0f), 1.0f);
}

TEST(normalize, to_normalized_float_mid_value)
{
    float value = 0.5f;
    EXPECT_FLOAT_EQ(to_normalized<float>(value, 0.0f, 1.0f), 0.5f);
}

TEST(normalize, to_normalized_int_min_max)
{
    int min = 0;
    int max = 127;
    EXPECT_FLOAT_EQ(to_normalized<float>(min, min, max), 0.0f);
    EXPECT_FLOAT_EQ(to_normalized<float>(max, min, max), 1.0f);
}

TEST(normalize, from_normalized_float_identity)
{
    float min = -10.0f;
    float max = 20.0f;
    float norm = 0.25f;
    float expected = min + norm * (max - min);
    EXPECT_FLOAT_EQ(from_normalized(norm, min, max), expected);
}

TEST(normalize, int_roundtrip_mid_value)
{
    int min = 0;
    int max = 127;
    int value = 64;
    float norm = to_normalized<float>(value, min, max);
    int roundtrip = from_normalized<float>(norm, min, max);
    // Roundtrip may differ by 1 due to rounding
    EXPECT_NEAR(roundtrip, value, 1);
}

TEST(normalize, to_normalized_float_ct_zero_to_one)
{
    constexpr float min = 0.0f;
    constexpr float max = 1.0f;
    EXPECT_FLOAT_EQ((to_normalized<float, float, min, max>(min)), 0.0f);
    EXPECT_FLOAT_EQ((to_normalized<float, float, min, max>(max)), 1.0f);
}

TEST(normalize, to_normalized_float_ct_mid_value)
{
    constexpr float min = -10.0f;
    constexpr float max = 20.0f;
    float mid = 5.0f;
    EXPECT_FLOAT_EQ(
        (to_normalized<float, float, min, max>(mid)),
        (mid - min) / (max - min));
}

TEST(normalize, to_normalized_float_ct_min_max)
{
    constexpr int min = 0;
    constexpr int max = 127;
    EXPECT_FLOAT_EQ((to_normalized<float, int, min, max>(min)), 0.0f);
    EXPECT_FLOAT_EQ((to_normalized<float, int, min, max>(max)), 1.0f);
}

TEST(normalize, int_ct_roundtrip)
{
    constexpr int min = 0;
    constexpr int max = 127;
    int value = 64;
    float norm = to_normalized<float, int, min, max>(value);
    int roundtrip = from_normalized<float, int, min, max>(norm);
    EXPECT_NEAR(roundtrip, value, 1);
}

} // namespace piejam::numeric::test
