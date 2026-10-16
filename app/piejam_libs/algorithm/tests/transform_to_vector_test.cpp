// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/algorithm/transform_to_vector.h>

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <list>
#include <ranges>
#include <string>

namespace piejam::algorithm::test
{

TEST(transform_to_vector, works_with_vector)
{
    std::vector<int> input{1, 2, 3, 4};
    auto result = transform_to_vector(input, [](int x) { return x * 2; });

    ASSERT_EQ(result.size(), 4u);
    EXPECT_EQ(result[0], 2);
    EXPECT_EQ(result[1], 4);
    EXPECT_EQ(result[2], 6);
    EXPECT_EQ(result[3], 8);
}

TEST(transform_to_vector, works_with_list)
{
    std::list<std::string> input{"a", "bb", "ccc"};
    auto result = transform_to_vector(input, [](std::string const& s) {
        return s.size();
    });

    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0], 1u);
    EXPECT_EQ(result[1], 2u);
    EXPECT_EQ(result[2], 3u);
}

TEST(transform_to_vector, works_with_input_range_stream)
{
    std::istringstream iss{"10 20 30"};
    auto input = std::ranges::istream_view<int>(iss);

    auto result = transform_to_vector(input, [](int x) { return x + 1; });

    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0], 11);
    EXPECT_EQ(result[1], 21);
    EXPECT_EQ(result[2], 31);
}

TEST(transform_to_vector, works_with_empty_range)
{
    std::vector<int> input{};
    auto result = transform_to_vector(input, [](int x) { return x * 10; });

    EXPECT_TRUE(result.empty());
}

TEST(transform_to_vector, works_with_move_only_type)
{
    struct move_only
    {
        int value;

        explicit move_only(int v)
            : value(v)
        {
        }

        move_only(move_only&&) = default;
        move_only(move_only const&) = delete;

        ~move_only() = default;

        auto operator=(move_only&&) -> move_only& = default;
        auto operator=(move_only const&) -> move_only& = delete;
    };

    std::vector<int> input{1, 2, 3};
    auto result =
        transform_to_vector(input, [](int x) { return move_only{x}; });

    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0].value, 1);
    EXPECT_EQ(result[1].value, 2);
    EXPECT_EQ(result[2].value, 3);
}

} // namespace piejam::algorithm::test
