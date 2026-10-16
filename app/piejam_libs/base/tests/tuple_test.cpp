// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/tuple.h>
#include <piejam/tuple_element_compare.h>

#include <gtest/gtest.h>

#include <tuple>

namespace piejam::test
{

TEST(tuple, for_each_while_with_non_empty_lvalue_tuple)
{
    auto t = std::tuple<int, int, int, unsigned, char>{10, 20, 30, 100, 50};

    int counter{};
    EXPECT_FALSE(tuple::for_each_while(t, [&](auto&& x) {
        counter += std::is_rvalue_reference_v<decltype(x)> -
                   std::is_lvalue_reference_v<decltype(x)>;
        return x < 100;
    }));

    EXPECT_EQ(counter, -4);
}

TEST(tuple, for_each_while_with_non_empty_rvalue_tuple)
{
    int counter{};
    EXPECT_FALSE(
        tuple::for_each_while(
            std::tuple<int, int, int, unsigned, char>{10, 20, 30, 100, 50},
            [&](auto&& x) {
                counter += std::is_rvalue_reference_v<decltype(x)> -
                           std::is_lvalue_reference_v<decltype(x)>;
                return x < 100;
            }));

    EXPECT_EQ(counter, 4);
}

TEST(tuple, for_each_while_and_condition_always_true)
{
    int counter{};
    EXPECT_TRUE(
        tuple::for_each_while(
            std::tuple<int, int, int, unsigned, char>{10, 20, 30, 100, 50},
            [&](auto&& x) {
                ++counter;
                return x < 1000;
            }));

    EXPECT_EQ(counter, 5);
}

TEST(tuple, for_each_while_with_empty_tuple)
{
    int counter{};
    EXPECT_TRUE(tuple::for_each_while(std::tuple{}, [&](auto&& x) {
        ++counter;
        return x < 1000;
    }));

    EXPECT_EQ(counter, 0);
}

TEST(tuple, for_each_while_with_pair)
{
    int counter{};
    EXPECT_TRUE(tuple::for_each_while(std::pair{1, 2}, [&](auto&& x) {
        ++counter;
        return x < 1000;
    }));

    EXPECT_EQ(counter, 2);
}

TEST(tuple, for_each_while_with_mutable_functor)
{
    // Should compile and run correctly because lambda is mutable
    EXPECT_TRUE(
        tuple::for_each_while(
            std::make_tuple(1, 2, 3),
            [v = int{42}](int x) mutable {
                // move-only due to unique_ptr capture
                v += x;
                return true;
            }));
}

TEST(tuple, for_each_until_with_non_empty_lvalue_tuple)
{
    auto t = std::tuple<int, int, int, unsigned, char>{10, 20, 30, 100, 50};

    int counter{};
    EXPECT_TRUE(tuple::for_each_until(t, [&](auto&& x) {
        counter += std::is_rvalue_reference_v<decltype(x)> -
                   std::is_lvalue_reference_v<decltype(x)>;
        return x >= 100;
    }));

    EXPECT_EQ(counter, -4);
}

TEST(tuple, for_each_until_with_non_empty_rvalue_tuple)
{
    int counter{};
    EXPECT_TRUE(
        tuple::for_each_until(
            std::tuple<int, int, int, unsigned, char>{10, 20, 30, 100, 50},
            [&](auto&& x) {
                counter += std::is_rvalue_reference_v<decltype(x)> -
                           std::is_lvalue_reference_v<decltype(x)>;
                return x >= 100;
            }));

    EXPECT_EQ(counter, 4);
}

TEST(tuple, for_each_until_and_condition_not_reached)
{
    int counter{};
    EXPECT_FALSE(
        tuple::for_each_until(
            std::tuple<int, int, int, unsigned, char>{10, 20, 30, 100, 50},
            [&](auto&& x) {
                ++counter;
                return x >= 1000;
            }));

    EXPECT_EQ(counter, 5);
}

TEST(tuple, for_each_until_with_empty_tuple)
{
    int counter{};
    EXPECT_FALSE(tuple::for_each_until(std::tuple{}, [&](auto&& x) {
        ++counter;
        return x >= 100;
    }));

    EXPECT_EQ(counter, 0);
}

TEST(tuple, element_compare_test_move_only_value)
{
    auto cmp = tuple::element<0>.not_equal_to(
        std::make_unique<int>(42)); // move-only value

    EXPECT_TRUE(cmp(std::make_tuple(std::make_unique<int>(42), 100)));
}
} // namespace piejam::test
