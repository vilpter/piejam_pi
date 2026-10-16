// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/audio/sound_card_manager.h>

#include "alsa/get_set_hw_params.h"
#include "alsa/get_sound_cards.h"
#include "alsa/pcm_io.h"

#include <piejam/audio/sound_card_descriptor.h>
#include <piejam/audio/sound_card_hw_params.h>

#include <piejam/box.h>
#include <piejam/io_pair.h>

#include <algorithm>

namespace piejam::audio
{

namespace
{

class alsa_sound_card_manager final : public sound_card_manager
{
public:
    auto get_sound_cards() -> sound_cards override
    {
        return alsa::get_sound_cards();
    }

    auto get_hw_params(
        sound_card_descriptor const& d,
        sample_rate const sample_rate,
        period_size const period_size) -> sound_card_hw_params override
    {
        auto const& [in, out] =
            std::any_cast<alsa::stream_descriptors>(d.impl_data);

        sound_card_hw_params result;

        auto in_params = alsa::get_hw_params(in, sample_rate, period_size);
        auto out_params = alsa::get_hw_params(out, sample_rate, period_size);

        std::ranges::set_intersection(
            in_params.sample_rates,
            out_params.sample_rates,
            std::back_inserter(result.sample_rates),
            {},
            &sample_rate::value,
            &sample_rate::value);

        std::ranges::set_intersection(
            in_params.period_sizes,
            out_params.period_sizes,
            std::back_inserter(result.period_sizes),
            {},
            &period_size::value,
            &period_size::value);

        return result;
    }

    auto make_io_process(
        sound_card_descriptor const& d,
        sound_card_config const& config) -> std::unique_ptr<io_process> override
    {
        auto const& [in, out] =
            std::any_cast<alsa::stream_descriptors>(d.impl_data);
        return std::make_unique<alsa::pcm_io>(in, out, config);
    }
};

} // namespace

auto
get_default_sound_card_manager() -> sound_card_manager&
{
    static auto s_instance = alsa_sound_card_manager{};
    return s_instance;
}

} // namespace piejam::audio
