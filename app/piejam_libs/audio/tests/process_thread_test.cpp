// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/audio/process_thread.h>

#include <gtest/gtest.h>

namespace piejam::audio::test
{

namespace
{

bool
wait_for_running(process_thread& t, std::chrono::milliseconds timeout)
{
    auto start = std::chrono::steady_clock::now();
    while (!t.is_running())
    {
        if (std::chrono::steady_clock::now() - start > timeout)
        {
            return false;
        }

        std::this_thread::yield();
    }
    return true;
}

bool
wait_for_stopped(process_thread& t, std::chrono::milliseconds timeout)
{
    auto start = std::chrono::steady_clock::now();
    while (t.is_running())
    {
        if (std::chrono::steady_clock::now() - start > timeout)
        {
            return false;
        }

        std::this_thread::yield();
    }
    return true;
}

} // namespace

TEST(process_thread, start_is_running_true)
{
    process_thread sut;
    sut.start({}, []() { return std::error_condition(); });
    EXPECT_TRUE(wait_for_running(sut, std::chrono::seconds{1}));
}

TEST(process_thread, start_stop_is_running_false)
{
    process_thread sut;
    sut.start({}, []() { return std::error_condition(); });
    ASSERT_TRUE(wait_for_running(sut, std::chrono::seconds{1}));
    sut.stop();
    EXPECT_TRUE(wait_for_stopped(sut, std::chrono::seconds{1}));
}

TEST(process_thread, stop_on_error)
{
    process_thread sut;

    std::atomic_bool generate_error{false};

    ASSERT_EQ(std::error_condition(), sut.error());
    sut.start({}, [&]() {
        if (generate_error)
        {
            return std::make_error_condition(std::errc::broken_pipe);
        }

        return std::error_condition();
    });

    ASSERT_TRUE(wait_for_running(sut, std::chrono::seconds{1}));

    generate_error = true;

    EXPECT_TRUE(wait_for_stopped(sut, std::chrono::seconds{1}));
    EXPECT_EQ(std::make_error_condition(std::errc::broken_pipe), sut.error());
}

} // namespace piejam::audio::test
