// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/MidiInputSettings.h>

#include <piejam/gui/ListModelEditScriptProcessor.h>
#include <piejam/gui/model/MidiDeviceConfig.h>
#include <piejam/gui/model/ObjectListModel.h>

#include <piejam/algorithm/edit_script.h>
#include <piejam/runtime/selectors.h>

namespace piejam::gui::model
{

struct MidiInputSettings::Impl
{
    box<midi::device_ids_t> device_ids;
};

MidiInputSettings::MidiInputSettings(runtime::state_access const& state_access)
    : CompositeSubscribableModel(state_access)
    , m_impl{make_pimpl<Impl>()}
    , m_devices{&addQObject<MidiDeviceList>()}
{
}

void
MidiInputSettings::onSubscribe()
{
    observe(
        runtime::selectors::select_midi_input_devices,
        [this](auto const& devs) {
            algorithm::apply_edit_script(
                algorithm::edit_script(*m_impl->device_ids, *devs),
                ListModelEditScriptProcessor{
                    static_cast<MidiDeviceList&>(*m_devices),
                    [this](midi::device_id_t device_id) {
                        return std::make_unique<MidiDeviceConfig>(
                            state_access(),
                            device_id);
                    }});

            m_impl->device_ids = devs;
        });
}

} // namespace piejam::gui::model
