// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/renew.h>

#include <gtest/gtest.h>

namespace piejam::test
{

TEST(renew, throwing_constructor_with_assignment)
{
    struct may_throw
    {
        int x;
        may_throw(int a)
        {
            if (a < 0)
            {
                throw std::runtime_error("bad");
            }
            x = a;
        }
        may_throw& operator=(may_throw const&) = default;
    };

    may_throw obj(10);
    ASSERT_EQ(obj.x, 10);

    renew(obj, 5);
    EXPECT_EQ(obj.x, 5);

    EXPECT_NO_THROW(renew(obj, 1));
    EXPECT_EQ(obj.x, 1);

    EXPECT_THROW(renew(obj, -1), std::runtime_error);
    EXPECT_EQ(obj.x, 1);
}

TEST(renew, non_copyable_no_throw)
{
    struct non_copyable
    {
        int x;
        non_copyable(int a) noexcept
            : x(a)
        {
        }
        non_copyable(non_copyable const&) = delete;
        non_copyable& operator=(non_copyable const&) = delete;
    };

    non_copyable obj(7);
    ASSERT_EQ(obj.x, 7);

    renew(obj, 99);
    EXPECT_EQ(obj.x, 99);
}

} // namespace piejam::test
