// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/thread/cpu_clock.h>

#include <ctime>

namespace piejam::thread
{

auto
cpu_clock::now() -> time_point
{
    struct timespec ts;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    return time_point(duration{ts.tv_sec * 1'000'000'000LL + ts.tv_nsec});
}

} // namespace piejam::thread
