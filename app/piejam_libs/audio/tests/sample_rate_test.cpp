// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/audio/sample_rate.h>

#include <gtest/gtest.h>

namespace piejam::audio::test
{

TEST(sample_rate, default_constructed_is_invalid)
{
    sample_rate sr;
    EXPECT_FALSE(sr.valid());
    EXPECT_TRUE(sr.invalid());
    EXPECT_EQ(sr.value(), 0u);
}

TEST(sample_rate, construct_with_value_is_valid)
{
    sample_rate sr(44100u);
    EXPECT_TRUE(sr.valid());
    EXPECT_FALSE(sr.invalid());
    EXPECT_EQ(sr.value(), 44100u);
}

TEST(sample_rate, as_int_and_as_float)
{
    sample_rate sr(48000u);
    EXPECT_EQ(sr.as<int>(), 48000);
    EXPECT_FLOAT_EQ(sr.as<float>(), 48000.0f);
    EXPECT_DOUBLE_EQ(sr.as<double>(), 48000.0);
}

TEST(sample_rate, equality_operator)
{
    sample_rate sr1(44100u);
    sample_rate sr2(44100u);
    sample_rate sr3(48000u);

    EXPECT_TRUE(sr1 == sr2);
    EXPECT_FALSE(sr1 == sr3);
}

TEST(sample_rate, duration_for_samples)
{
    using namespace std::chrono_literals;

    sample_rate sr(48000u);

    // 48000 samples at 48000 Hz = 1 second = 1e9 nanoseconds
    auto dur = sr.duration_for_samples(48000);
    EXPECT_EQ(dur, 1s);

    // 24000 samples at 48000 Hz = 0.5 seconds = 5e8 nanoseconds
    auto dur2 = sr.duration_for_samples(24000);
    EXPECT_EQ(dur2, 500ms);
}

TEST(sample_rate, samples_for_duration)
{
    using namespace std::chrono_literals;

    sample_rate sr(48000u);

    EXPECT_EQ(sr.samples_for_duration(1s), 48000u);
    EXPECT_EQ(sr.samples_for_duration(500ms), 24000u);
    EXPECT_EQ(sr.samples_for_duration(2s), 96000u);
    EXPECT_EQ(sr.samples_for_duration(1ms), 48u);
}

} // namespace piejam::audio::test
