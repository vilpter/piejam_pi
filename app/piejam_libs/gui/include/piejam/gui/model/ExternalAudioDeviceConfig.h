// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/SubscribableModel.h>
#include <piejam/gui/model/fwd.h>

#include <piejam/runtime/external_audio_fwd.h>

namespace piejam::gui::model
{

class ExternalAudioDeviceConfig final : public CompositeSubscribableModel
{
    Q_OBJECT

    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::String*, name)
    PIEJAM_GUI_CONSTANT_PROPERTY(bool, mono)
    PIEJAM_GUI_PROPERTY(int, monoChannel, setMonoChannel)
    PIEJAM_GUI_PROPERTY(int, stereoLeftChannel, setStereoLeftChannel)
    PIEJAM_GUI_PROPERTY(int, stereoRightChannel, setStereoRightChannel)

public:
    ExternalAudioDeviceConfig(
        runtime::state_access const&,
        runtime::external_audio::device_id);

    Q_INVOKABLE void changeMonoChannel(unsigned);
    Q_INVOKABLE void changeStereoLeftChannel(unsigned);
    Q_INVOKABLE void changeStereoRightChannel(unsigned);
    Q_INVOKABLE void remove();

private:
    void onSubscribe() override;

    runtime::external_audio::device_id m_device_id;
};

} // namespace piejam::gui::model
