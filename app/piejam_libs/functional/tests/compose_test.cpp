// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/functional/compose.h>

#include <boost/callable_traits/return_type.hpp>

#include <gtest/gtest.h>

namespace piejam::functional::test
{

TEST(compose_test, single_value_argument)
{
    auto f1 = [](int x) { return x + 1; };
    auto f2 = [](int y) { return y * 2; };

    auto h = compose(f1, f2);

    using h_t = decltype(h);
    static_assert(
        std::is_same_v<int, boost::callable_traits::return_type_t<h_t>>);
    static_assert(
        std::is_same_v<std::tuple<int>, boost::callable_traits::args_t<h_t>>);

    EXPECT_EQ(h(10), 21); // (10*2)+1
}

TEST(compose_test, lvalue_reference_argument)
{
    int x = 5;
    auto f1 = [](int& y) -> int& {
        y += 10;
        return y;
    };
    auto f2 = [](int& z) -> int& { return z; };

    auto h = compose(f1, f2);
    int& result = h(x);

    using h_t = decltype(h);
    static_assert(
        std::is_same_v<int&, boost::callable_traits::return_type_t<h_t>>);
    static_assert(
        std::is_same_v<std::tuple<int&>, boost::callable_traits::args_t<h_t>>);

    EXPECT_EQ(result, 15);
    EXPECT_EQ(x, 15); // lvalue forwarded correctly
}

TEST(compose_test, const_lvalue_reference_argument)
{
    int const cx = 7;
    auto f1 = [](int const& y) { return y * 3; };
    auto f2 = [](int const& z) -> int const& { return z; };

    auto h = compose(f1, f2);

    using h_t = decltype(h);
    static_assert(
        std::is_same_v<int, boost::callable_traits::return_type_t<h_t>>);
    static_assert(std::is_same_v<
                  std::tuple<int const&>,
                  boost::callable_traits::args_t<h_t>>);

    EXPECT_EQ(h(cx), 21);
}

TEST(compose_test, rvalue_reference_argument)
{
    auto f1 = [](std::string&& s) { return s + "!"; };
    auto f2 = [](std::string&& t) -> std::string&& { return std::move(t); };

    auto h = compose(f1, f2);
    std::string str = "hello";

    using h_t = decltype(h);
    static_assert(
        std::
            is_same_v<std::string, boost::callable_traits::return_type_t<h_t>>);
    static_assert(std::is_same_v<
                  std::tuple<std::string&&>,
                  boost::callable_traits::args_t<h_t>>);

    EXPECT_EQ(h(std::move(str)), "hello!");
}

TEST(compose_test, variadic_composition_multiple_functions)
{
    auto f1 = [](int x) { return x + 1; };
    auto f2 = [](int y) { return y * 2; };
    auto f3 = [](int z) { return z - 3; };

    auto h = compose(f1, f2, f3); // f1(f2(f3(x)))

    using h_t = decltype(h);
    static_assert(
        std::is_same_v<int, boost::callable_traits::return_type_t<h_t>>);
    static_assert(
        std::is_same_v<std::tuple<int>, boost::callable_traits::args_t<h_t>>);

    EXPECT_EQ(h(10), 15); // ((10-3)*2)+1
}

TEST(compose_test, args_t_and_return_type)
{
    auto f1 = [](double x) { return x + 0.5; };
    auto f2 = [](double const& y) { return y * 2; };
    auto f3 = [](double&& z) { return z - 1; };

    auto h = compose(f1, f2, f3);

    // The final composed lambda should have the same args_t as f3
    using expected_args = boost::callable_traits::args_t<decltype(f3)>;
    using composed_args = boost::callable_traits::args_t<decltype(h)>;
    EXPECT_TRUE((std::is_same_v<expected_args, composed_args>));

    // The return type should be f1(f2(f3(args)))
    using expected_return = std::invoke_result_t<
        decltype(f1),
        std::invoke_result_t<
            decltype(f2),
            std::invoke_result_t<decltype(f3), double&&>>>;
    using composed_return = boost::callable_traits::return_type_t<decltype(h)>;
    EXPECT_TRUE((std::is_same_v<expected_return, composed_return>));
}

TEST(compose_test, move_only_argument)
{
    auto f1 = [](std::unique_ptr<int> p) {
        return *p + 10; // consumes the pointer
    };

    auto f2 = [](std::unique_ptr<int>&& p) -> std::unique_ptr<int> {
        return std::move(p); // forwards the pointer
    };

    auto h = compose(f1, f2);

    std::unique_ptr<int> ptr = std::make_unique<int>(5);

    int result = h(std::move(ptr)); // move-only argument

    EXPECT_EQ(result, 15); // 5 + 10

    // ptr should now be null, because it was moved
    EXPECT_EQ(ptr, nullptr);
}

TEST(compose_test, mutable_lambdas)
{
    auto f1 = [acc = 0](int x) mutable {
        acc += x;
        return acc;
    };

    auto f2 = [acc = 0](int x) mutable {
        acc += x;
        return acc;
    };

    auto h = compose(f1, f2);

    EXPECT_EQ(h(1), 1);
    EXPECT_EQ(h(1), 3);
    EXPECT_EQ(h(1), 6);
}

} // namespace piejam::functional::test
