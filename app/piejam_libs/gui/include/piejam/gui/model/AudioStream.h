// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026 Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/audio/multichannel_view.h>

#include <ranges>

namespace piejam::gui::model
{

using AudioStream = audio::
    multichannel_view<float const, audio::multichannel_layout_non_interleaved>;

using MonoAudioStream = audio::multichannel_view<
    float const,
    audio::multichannel_layout_non_interleaved,
    1>;

using StereoAudioStream = audio::multichannel_view<
    float const,
    audio::multichannel_layout_non_interleaved,
    2>;

constexpr auto
toLeft(StereoAudioStream stream)
{
    return stream.channels()[0];
}

constexpr auto
toRight(StereoAudioStream stream)
{
    return stream.channels()[1];
}

inline auto
toMiddle(StereoAudioStream stream)
{
    return stream.frames() | std::views::transform([](auto frame) {
               return frame[0] + frame[1];
           });
}

inline auto
toSide(StereoAudioStream stream)
{
    return stream.frames() | std::views::transform([](auto frame) {
               return frame[0] - frame[1];
           });
}

} // namespace piejam::gui::model
