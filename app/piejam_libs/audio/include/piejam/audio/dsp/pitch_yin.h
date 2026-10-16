// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/audio/sample_rate.h>

#include <concepts>
#include <span>

namespace piejam::audio::dsp
{

template <std::floating_point T>
[[nodiscard]]
auto pitch_yin(std::span<T const> in, sample_rate) -> T;

} // namespace piejam::audio::dsp
