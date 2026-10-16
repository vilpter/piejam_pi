// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <boost/hof/lift.hpp>

#include <cmath>

namespace piejam::numeric
{

inline constexpr auto abs = BOOST_HOF_LIFT(std::abs);

} // namespace piejam::numeric
