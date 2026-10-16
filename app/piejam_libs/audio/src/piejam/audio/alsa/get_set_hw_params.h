// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/audio/fwd.h>

#include <piejam/system/fwd.h>

#include <filesystem>

namespace piejam::audio::alsa
{

auto get_num_channels(std::filesystem::path const& device_path) -> unsigned;

auto get_hw_params(std::filesystem::path const&, sample_rate, period_size)
    -> sound_card_hw_params;

struct set_hw_params_result
{
    unsigned num_channels{};
    pcm_format format;
    unsigned period_count{};
};

auto set_hw_params(system::device&, sound_card_config const&)
    -> set_hw_params_result;

} // namespace piejam::audio::alsa
