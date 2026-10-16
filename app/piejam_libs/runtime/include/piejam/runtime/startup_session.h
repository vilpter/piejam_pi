// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace piejam::runtime
{

enum class startup_session : bool
{
    new_,
    last,
};

} // namespace piejam::runtime
