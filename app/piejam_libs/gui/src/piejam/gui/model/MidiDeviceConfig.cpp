// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/MidiDeviceConfig.h>

#include <piejam/runtime/actions/activate_midi_device.h>
#include <piejam/runtime/actions/deactivate_midi_device.h>
#include <piejam/runtime/selectors.h>

namespace piejam::gui::model
{

MidiDeviceConfig::MidiDeviceConfig(
    runtime::state_access const& state_access,
    midi::device_id_t device_id)
    : SubscribableModel(state_access)
    , m_device_id(std::move(device_id))
{
}

void
MidiDeviceConfig::onSubscribe()
{
    observe(
        runtime::selectors::make_midi_device_name_selector(m_device_id),
        [this](std::string const& name) {
            setName(QString::fromStdString(name));
        });

    observe(
        runtime::selectors::make_midi_device_enabled_selector(m_device_id),
        [this](bool const x) { setEnabled(x); });
}

void
MidiDeviceConfig::changeEnabled(bool x)
{
    emit enabledChanged();

    if (x)
    {
        runtime::actions::activate_midi_device action;
        action.device_id = m_device_id;
        dispatch(action);
    }
    else
    {
        runtime::actions::deactivate_midi_device action;
        action.device_id = m_device_id;
        dispatch(action);
    }
}

} // namespace piejam::gui::model
