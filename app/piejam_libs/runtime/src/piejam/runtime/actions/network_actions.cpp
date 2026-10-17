// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/actions/network_actions.h>

#include <piejam/runtime/state.h>

namespace piejam::runtime::actions
{

void
set_wifi_enabled::reduce(state& st) const
{
    st.wifi_enabled = enabled;
}

} // namespace piejam::runtime::actions
