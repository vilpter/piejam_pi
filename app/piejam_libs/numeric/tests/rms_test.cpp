// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/numeric/generators/sine.h>
#include <piejam/numeric/rms.h>
#include <piejam/numeric/simd/rms.h>

#include <gtest/gtest.h>

#include <algorithm>

namespace piejam::numeric::test
{

// test param: amplitude
struct rms_test : public testing::TestWithParam<float>
{
    rms_test()
        : signal(2048)
    {
        float const amp = GetParam();
        std::ranges::generate(
            signal,
            generators::sine<float>{440.f, 48000.f, amp});
    }

    mipp::vector<float> signal;
};

TEST_P(rms_test, raw_loop)
{
    EXPECT_NEAR(
        GetParam() / std::numbers::sqrt2_v<float>,
        rms(std::span{std::as_const(signal)}),
        0.0003);
}

TEST_P(rms_test, simd)
{
    EXPECT_NEAR(
        GetParam() / std::numbers::sqrt2_v<float>,
        simd::rms(std::span{std::as_const(signal)}),
        0.0003);
}

INSTANTIATE_TEST_SUITE_P(
    all,
    rms_test,
    testing::Values(0.001f, 0.01f, 0.1f, 0.2f, 0.5f, 0.8f, 1.f));

} // namespace piejam::numeric::test
