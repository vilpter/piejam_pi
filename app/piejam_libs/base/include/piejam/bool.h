// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace piejam
{

constexpr auto
toggle_bool_in_place(bool& x) noexcept -> bool
{
    x = !x;
    return x;
}

} // namespace piejam
