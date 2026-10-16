// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/state_access.h>

#include <piejam/runtime/state.h>

#include <piejam/redux/store.h>

namespace piejam::runtime
{

void
state_access::dispatch(action const& a) const
{
    m_store->dispatch(a);
}

} // namespace piejam::runtime
