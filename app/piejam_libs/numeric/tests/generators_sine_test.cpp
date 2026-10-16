// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/numeric/generators/sine.h>

#include <gtest/gtest.h>

#include <vector>

namespace piejam::numeric::generators::test
{

TEST(sine, generates_correct_initial_value)
{
    sine<double> gen(440.0, 48000.0, 1.0, 0.0);
    double value = gen();
    EXPECT_DOUBLE_EQ(value, 0.0); // sin(0) = 0
}

TEST(sine, respects_initial_phase)
{
    sine<double> gen(440.0, 48000.0, 1.0, M_PI_2);
    double value = gen();
    EXPECT_DOUBLE_EQ(value, 1.0); // sin(π/2) = 1
}

TEST(sine, increments_phase_correctly)
{
    // 1 Hz, 4 samples per second → step = π/2
    sine<double> gen(1.0, 4.0);
    std::vector<double> expected = {0.0, 1.0, 0.0, -1.0};
    for (double exp : expected)
    {
        double value = gen();
        EXPECT_NEAR(value, exp, 1e-10);
    }
}

TEST(sine, wraps_phase)
{
    sine<double> gen(1.0, 4.0);
    for (std::size_t i = 0; i < 10; ++i)
    {
        gen(); // call repeatedly to advance phase
    }
    double value = gen();
    EXPECT_LE(value, 1.0);
    EXPECT_GE(value, -1.0);
}

TEST(sine, respects_amplitude)
{
    sine<double> gen(440.0, 48000.0, 0.7);
    for (std::size_t i = 0; i < 1000; ++i)
    {
        double value = gen();
        EXPECT_LE(value, 0.7);
        EXPECT_GE(value, -0.7);
    }
}

} // namespace piejam::numeric::generators::test
