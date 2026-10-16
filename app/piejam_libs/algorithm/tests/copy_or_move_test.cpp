// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/algorithm/copy_or_move.h>

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <vector>

namespace piejam::algorithm::test
{

TEST(copy_or_move, works_with_copyable_range)
{
    std::vector<int> src{1, 2, 3, 4};
    std::vector<int> dst(4, 0);

    copy_or_move(src, dst.begin());

    EXPECT_EQ(dst, src);                            // copied correctly
    EXPECT_EQ(src, (std::vector<int>{1, 2, 3, 4})); // src unchanged
}

TEST(copy_or_move, works_with_copyable_iterators)
{
    std::vector<int> src{5, 6, 7};
    std::vector<int> dst(3, 0);

    copy_or_move(src.begin(), src.end(), dst.begin());

    EXPECT_EQ(dst, src);
    EXPECT_EQ(src, (std::vector<int>{5, 6, 7}));
}

TEST(copy_or_move, works_with_move_only_range)
{
    std::vector<std::unique_ptr<int>> src;
    src.push_back(std::make_unique<int>(10));
    src.push_back(std::make_unique<int>(20));

    std::vector<std::unique_ptr<int>> dst(2);

    copy_or_move(src, dst.begin());

    EXPECT_EQ(*dst[0], 10);
    EXPECT_EQ(*dst[1], 20);

    EXPECT_EQ(src[0], nullptr);
    EXPECT_EQ(src[1], nullptr);
}

TEST(copy_or_move, works_with_move_only_iterators)
{
    std::vector<std::unique_ptr<int>> src;
    src.push_back(std::make_unique<int>(42));
    src.push_back(std::make_unique<int>(99));

    std::vector<std::unique_ptr<int>> dst(2);

    copy_or_move(src.begin(), src.end(), dst.begin());

    EXPECT_EQ(*dst[0], 42);
    EXPECT_EQ(*dst[1], 99);

    EXPECT_EQ(src[0], nullptr);
    EXPECT_EQ(src[1], nullptr);
}

TEST(copy_or_move, works_with_empty_range)
{
    std::vector<int> src;
    std::vector<int> dst(3, 0);

    copy_or_move(src, dst.begin());

    EXPECT_EQ(dst, (std::vector<int>{0, 0, 0})); // unchanged
}

TEST(copy_or_move, works_with_empty_iterators)
{
    std::vector<int> src;
    std::vector<int> dst(2, 7);

    copy_or_move(src.begin(), src.end(), dst.begin());

    EXPECT_EQ(dst, (std::vector<int>{7, 7})); // unchanged
}

TEST(copy_or_move, works_with_partial_iterators)
{
    std::vector<int> src{1, 2, 3, 4, 5};
    std::vector<int> dst(3, 0);

    copy_or_move(src.begin() + 1, src.begin() + 4, dst.begin());

    EXPECT_EQ(dst, (std::vector<int>{2, 3, 4}));
}

TEST(copy_or_move, works_with_strings)
{
    std::vector<std::string> src{"foo", "bar", "baz"};
    std::vector<std::string> dst(3);

    copy_or_move(src, dst.begin());

    EXPECT_EQ(dst, src);
    EXPECT_EQ(
        src,
        (std::vector<std::string>{"foo", "bar", "baz"})); // copy semantics
}

} // namespace piejam::algorithm::test
