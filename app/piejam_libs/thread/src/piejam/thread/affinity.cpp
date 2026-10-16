// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/thread/affinity.h>

#include <pthread.h>
#include <sched.h>

#include <boost/assert.hpp>

#include <system_error>
#include <thread>

namespace piejam::this_thread
{

void
set_affinity(unsigned int const cpu)
{
    BOOST_ASSERT(cpu < std::thread::hardware_concurrency());

    cpu_set_t cpuset{};
    CPU_SET(cpu, &cpuset);
    auto const status =
        pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
    if (status)
    {
        throw std::system_error(status, std::generic_category());
    }
}

} // namespace piejam::this_thread
