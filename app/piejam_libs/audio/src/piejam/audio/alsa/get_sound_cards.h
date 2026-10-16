// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/audio/sound_card_descriptor.h>

#include <filesystem>

namespace piejam::audio::alsa
{

using stream_descriptors = io_pair<std::filesystem::path>;

auto get_sound_cards() -> sound_cards;

} // namespace piejam::audio::alsa
