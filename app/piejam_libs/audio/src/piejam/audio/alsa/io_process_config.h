// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/audio/pcm_format.h>
#include <piejam/audio/sound_card_config.h>

namespace piejam::audio::alsa
{

struct sound_card_stream_config
{
    pcm_format format{};
    unsigned num_channels{};
};

struct io_process_config
{
    sound_card_stream_config in_config;
    sound_card_stream_config out_config;
    sound_card_config sc_config;
};

} // namespace piejam::audio::alsa
