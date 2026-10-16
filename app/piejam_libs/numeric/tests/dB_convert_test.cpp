// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/numeric/dB_convert.h>

#include <gtest/gtest.h>

namespace piejam::numeric::test
{

TEST(to_dB, custom_min)
{
    EXPECT_FLOAT_EQ(constants::negative_inf<float>, to_dB(0.f, 0.1f));
    EXPECT_FLOAT_EQ(constants::negative_inf<float>, to_dB(0.05f, 0.1f));
    EXPECT_FLOAT_EQ(constants::negative_inf<float>, to_dB(0.1f, 0.1f));
}

TEST(from_dB, custom_min)
{
    EXPECT_FLOAT_EQ(0.f, from_dB(constants::negative_inf<float>, -20.f));
    EXPECT_FLOAT_EQ(0.f, from_dB(-60.f, -20.f));
    EXPECT_FLOAT_EQ(0.f, from_dB(-20.f, -20.f));
}

} // namespace piejam::numeric::test
