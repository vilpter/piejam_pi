// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace piejam::this_thread
{

void enable_flush_to_zero() noexcept;

void cpu_spin_yield() noexcept;

} // namespace piejam::this_thread
