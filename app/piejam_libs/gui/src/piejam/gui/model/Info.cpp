// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/Info.h>

#include <piejam/runtime/actions/control_midi_assignment.h>
#include <piejam/runtime/actions/recording.h>
#include <piejam/runtime/actions/request_info_update.h>
#include <piejam/runtime/selectors.h>

namespace piejam::gui::model
{

Info::Info(runtime::state_access const& state_access)
    : SubscribableModel(state_access)
{
}

void
Info::onSubscribe()
{
    observe(runtime::selectors::select_recording, [this](bool const x) {
        setRecording(x);
    });

    observe(runtime::selectors::select_xruns, [this](std::size_t const xruns) {
        setXruns(static_cast<unsigned>(xruns));
    });

    observe(runtime::selectors::select_cpu_load, [this](float const cpu_load) {
        setAudioLoad(cpu_load);
    });

    observe(
        runtime::selectors::select_midi_learning,
        [this](bool const midi_learning) { setMidiLearning(midi_learning); });

    requestUpdates(std::chrono::milliseconds{40}, [this]() {
        dispatch(runtime::actions::request_info_update{});
    });
}

void
Info::changeRecording(bool const rec)
{
    if (rec)
    {
        dispatch(runtime::actions::start_recording{});
    }
    else
    {
        dispatch(runtime::actions::stop_recording{});
    }
}

void
Info::stopMidiLearn()
{
    dispatch(runtime::actions::stop_midi_learning{});
}

} // namespace piejam::gui::model
