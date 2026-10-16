// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/audio/engine/fwd.h>
#include <piejam/runtime/midi_assignment.h>
#include <piejam/runtime/parameters.h>

#include <memory>

namespace piejam::runtime::processors
{

template <class Parameter>
auto make_midi_cc_to_parameter_processor(Parameter const&)
    -> std::unique_ptr<audio::engine::processor>;

template <class Parameter>
auto make_midi_pitch_bend_to_parameter_processor(Parameter const&)
    -> std::unique_ptr<audio::engine::processor>;

template <class Parameter>
auto
make_midi_to_parameter_processor(
    midi_assignment const& assign,
    Parameter const& param) -> std::unique_ptr<audio::engine::processor>
{
    switch (assign.control_type)
    {
        case midi_assignment::type::cc:
            return make_midi_cc_to_parameter_processor<Parameter>(param);

        case midi_assignment::type::pitch_bend:
            return make_midi_pitch_bend_to_parameter_processor<Parameter>(
                param);
    }

    return nullptr;
}

} // namespace piejam::runtime::processors
