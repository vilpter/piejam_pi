// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/audio/engine/rt_task_executor.h>

#include <gtest/gtest.h>

namespace piejam::audio::engine::test
{

TEST(rt_task_executor, executes_after_wakeup)
{
    std::atomic_bool worked{false};

    {
        rt_task_executor wt;
        wt.wakeup([&]() { worked.store(true, std::memory_order_release); });
    }

    EXPECT_TRUE(worked);
}

TEST(
    rt_task_executor,
    on_multiple_wakeups_block_until_previous_task_is_finished)
{
    std::size_t counter1{};
    std::size_t counter2{};
    bool select{};

    {
        rt_task_executor wt;
        for (std::size_t i = 0; i < 100; ++i)
        {
            if (select)
            {
                wt.wakeup([&]() { ++counter1; });
            }
            else
            {
                wt.wakeup([&]() { ++counter2; });
            }

            select = !select;
        }
    }

    EXPECT_EQ(50u, counter1);
    EXPECT_EQ(50u, counter2);
}

} // namespace piejam::audio::engine::test
