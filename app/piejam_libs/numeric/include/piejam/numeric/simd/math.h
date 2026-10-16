// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <mipp.h>

#include <boost/hof/lift.hpp>

namespace piejam::numeric::simd
{

inline constexpr auto abs = BOOST_HOF_LIFT(mipp::abs);
inline constexpr auto max = BOOST_HOF_LIFT(mipp::max);

} // namespace piejam::numeric::simd
