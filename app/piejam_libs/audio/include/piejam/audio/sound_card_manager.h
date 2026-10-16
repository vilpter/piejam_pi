// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/audio/fwd.h>
#include <piejam/audio/sound_card_descriptor.h>

#include <memory>

namespace piejam::audio
{

class sound_card_manager
{
public:
    virtual ~sound_card_manager() = default;

    virtual auto get_sound_cards() -> sound_cards = 0;

    virtual auto
    get_hw_params(sound_card_descriptor const&, sample_rate, period_size)
        -> sound_card_hw_params = 0;

    virtual auto
    make_io_process(sound_card_descriptor const&, sound_card_config const&)
        -> std::unique_ptr<io_process> = 0;
};

auto get_default_sound_card_manager() -> sound_card_manager&;

} // namespace piejam::audio
