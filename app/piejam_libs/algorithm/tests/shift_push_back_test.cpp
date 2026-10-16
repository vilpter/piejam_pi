// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/algorithm/shift_push_back.h>

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <array>

namespace piejam::algorithm::test
{

TEST(shift_push_back, source_is_smaller_than_target)
{
    std::array target{2, 2, 2, 2, 2};
    std::array source{3, 4};

    shift_push_back(target, source);

    EXPECT_TRUE(testing::Matches(testing::ElementsAre(2, 2, 2, 3, 4))(target));
}

TEST(shift_push_back, source_is_bigger_than_target)
{
    std::array target{2, 2, 2};
    std::array source{1, 2, 3, 4, 5};

    shift_push_back(target, source);

    EXPECT_TRUE(testing::Matches(testing::ElementsAre(3, 4, 5))(target));
}

TEST(shift_push_back, does_not_compile_for_move_only)
{
    std::vector<std::unique_ptr<int>> target(3);
    std::vector<std::unique_ptr<int>> source;

    source.push_back(std::make_unique<int>(1));
    source.push_back(std::make_unique<int>(2));

    shift_push_back(target, source);

    ASSERT_EQ(3, target.size());
    EXPECT_EQ(nullptr, target[0]);
    ASSERT_NE(nullptr, target[1]);
    EXPECT_EQ(1, *target[1]);
    ASSERT_NE(nullptr, target[2]);
    EXPECT_EQ(2, *target[2]);
}

} // namespace piejam::algorithm::test
