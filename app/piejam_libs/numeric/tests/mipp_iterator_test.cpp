// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/numeric/mipp_iterator.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace piejam::numeric::test
{

TEST(mipp_iterator, write)
{
    mipp::vector<float> buffer{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};

    auto it = mipp_iterator{buffer.data()};

    mipp::Reg<float> reg = *it;

    *it = reg * 2;

    EXPECT_THAT(
        buffer,
        testing::ElementsAre(2.0f, 4.0f, 6.0f, 8.0f, 5.0f, 6.0f, 7.0f, 8.0f));
}

TEST(mipp_iterator, concepts)
{
    static_assert(std::input_iterator<mipp_iterator<float const>>);
    static_assert(std::output_iterator<mipp_iterator<float>, mipp::Reg<float>>);
}

TEST(mipp_range_split, on_empty)
{
    mipp::vector<float> samples;
    auto [pre, main, post] = mipp_range_split(std::span{samples});

    EXPECT_EQ(pre.size(), 0u);
    EXPECT_EQ(main.size(), 0u);
    EXPECT_EQ(post.size(), 0u);
}

TEST(mipp_range_split, unaligned_pre)
{
    mipp::vector<float> samples(8);

    auto [pre, main, post] =
        mipp_range_split(std::span(std::next(samples.begin()), samples.end()));

    EXPECT_EQ(pre.size(), 3u);
    EXPECT_EQ(main.size(), 4u);
    EXPECT_TRUE(mipp::isAligned(main.data()));
    EXPECT_EQ(post.size(), 0u);
}

TEST(mipp_range_split, with_post)
{
    mipp::vector<float> samples(7);

    auto [pre, main, post] = mipp_range_split(std::span(samples));

    EXPECT_EQ(pre.size(), 0u);
    EXPECT_EQ(main.size(), 4u);
    EXPECT_TRUE(mipp::isAligned(main.data()));
    EXPECT_EQ(post.size(), 3u);
    EXPECT_TRUE(mipp::isAligned(post.data()));
}

TEST(mipp_range_split, unaligned_pre_with_post)
{
    mipp::vector<float> samples(7);

    auto [pre, main, post] =
        mipp_range_split(std::span(std::next(samples.begin()), samples.end()));

    EXPECT_EQ(pre.size(), 3u);
    EXPECT_EQ(main.size(), 0u);
    EXPECT_EQ(post.size(), 3u);
    EXPECT_TRUE(mipp::isAligned(post.data()));
}

} // namespace piejam::numeric::test
