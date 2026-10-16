// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/io_pair.h>

#include <any>
#include <string>
#include <vector>

namespace piejam::audio
{

struct sound_card_descriptor
{
    std::string name;
    io_pair<unsigned> num_channels;

    std::any impl_data;

    [[nodiscard]]
    auto operator==(sound_card_descriptor const& other) const noexcept -> bool
    {
        return name == other.name && num_channels == other.num_channels;
    }

    [[nodiscard]]
    auto operator!=(sound_card_descriptor const& other) const noexcept -> bool
    {
        return !(*this == other);
    }
};

using sound_cards = std::vector<sound_card_descriptor>;

} // namespace piejam::audio
