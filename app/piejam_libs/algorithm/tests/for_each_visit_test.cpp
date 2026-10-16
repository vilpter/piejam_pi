// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/algorithm/for_each_visit.h>

#include <gtest/gtest.h>

#include <vector>

namespace piejam::algorithm::test
{

TEST(for_each_visit_test, stateless_visitor)
{
    std::vector<std::variant<int, double>> data{1, 2.5, 3};
    int sum = 0;

    auto visitor = [&](auto x) { sum += static_cast<int>(x); };
    for_each_visit(data, visitor);

    EXPECT_EQ(sum, 1 + 2 + 3); // double gets truncated
}

TEST(for_each_visit_test, stateful_visitor)
{
    std::vector<std::variant<int>> data{1, 2, 3};

    struct counter_visitor
    {
        int count = 0;
        void operator()(int)
        {
            ++count;
        }
    };

    counter_visitor visitor;
    for_each_visit(data, visitor);

    EXPECT_EQ(visitor.count, 3);
}

TEST(for_each_visit_test, move_only_visitor)
{
    std::vector<std::variant<std::unique_ptr<int>>> data;
    data.push_back(std::make_unique<int>(10));
    data.push_back(std::make_unique<int>(20));

    struct move_only_visitor
    {
        int sum = 0;

        move_only_visitor() = default;
        move_only_visitor(move_only_visitor const&) = delete; // no copy
        move_only_visitor(move_only_visitor&&) = default;     // allow move

        ~move_only_visitor() = default;

        auto operator=(move_only_visitor const&)
            -> move_only_visitor& = delete; // no copy
        auto operator=(move_only_visitor&&)
            -> move_only_visitor& = default; // allow move

        void operator()(std::unique_ptr<int>& ptr)
        {
            sum += *ptr;
        }
    };

    auto visitor = for_each_visit(data, move_only_visitor{});

    EXPECT_EQ(visitor.sum, 30);
}

} // namespace piejam::algorithm::test
