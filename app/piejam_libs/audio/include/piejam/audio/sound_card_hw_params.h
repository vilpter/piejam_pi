// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/audio/period_size.h>
#include <piejam/audio/sample_rate.h>

namespace piejam::audio
{

struct sound_card_hw_params
{
    sample_rates_t sample_rates;
    period_sizes_t period_sizes;

    [[nodiscard]]
    constexpr auto operator==(sound_card_hw_params const&) const noexcept
        -> bool = default;
};

} // namespace piejam::audio
