// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/SubscribableModel.h>

#include <piejam/midi/device_id.h>

namespace piejam::gui::model
{

class MidiDeviceConfig final : public SubscribableModel
{
    Q_OBJECT

    PIEJAM_GUI_PROPERTY(QString, name, setName)
    PIEJAM_GUI_PROPERTY(bool, enabled, setEnabled)

public:
    MidiDeviceConfig(runtime::state_access const&, midi::device_id_t);

    Q_INVOKABLE void changeEnabled(bool x);

private:
    void onSubscribe() override;

    midi::device_id_t m_device_id;
};

} // namespace piejam::gui::model
