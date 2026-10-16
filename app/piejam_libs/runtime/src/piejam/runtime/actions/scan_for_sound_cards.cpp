// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/actions/scan_for_sound_cards.h>

#include <piejam/audio/sound_card_manager.h>
#include <piejam/runtime/actions/refresh_sound_cards.h>
#include <piejam/runtime/state.h>
#include <piejam/runtime/ui/thunk_action.h>

#include <algorithm>

namespace piejam::runtime::actions
{

auto
scan_for_sound_cards() -> thunk_action
{
    return [](auto&& get_state, auto&& dispatch) {
        auto scanned_sound_cards =
            audio::get_default_sound_card_manager().get_sound_cards();

        if (!std::ranges::equal(scanned_sound_cards, *get_state().sound_cards))
        {
            dispatch(refresh_sound_cards{});
        }
    };
}

} // namespace piejam::runtime::actions
