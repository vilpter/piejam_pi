// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/thread/configuration.h>

#include <piejam/thread/affinity.h>
#include <piejam/thread/alloc_debug.h>
#include <piejam/thread/cpu_util.h>
#include <piejam/thread/name.h>
#include <piejam/thread/priority.h>

namespace piejam::thread
{

void
configuration::apply() const
{
    if (affinity)
    {
        this_thread::set_affinity(*affinity);
    }

    if (realtime_priority)
    {
        this_thread::set_realtime_priority(*realtime_priority);
        this_thread::prohibit_dynamic_memory_allocation();
        this_thread::enable_flush_to_zero();
    }

    if (name)
    {
        this_thread::set_name(*name);
    }
}

} // namespace piejam::thread
