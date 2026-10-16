// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <system_error>

namespace piejam::system
{

[[nodiscard]]
auto mlockall() -> std::error_code;

} // namespace piejam::system
