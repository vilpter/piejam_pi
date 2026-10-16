// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/actions/set_string.h>

#include <piejam/runtime/state.h>

namespace piejam::runtime::actions
{

void
set_string::reduce(state& st) const
{
    if (st.strings.at(id).get() != str.get())
    {
        st.strings.assign(id, str);

        st.session_modified = true;
    }
}

} // namespace piejam::runtime::actions
