// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/MixerChannel.h>

#include <piejam/runtime/mixer_fwd.h>

namespace piejam::gui::model
{

class MixerChannelEdit final : public MixerChannel
{
    Q_OBJECT

    PIEJAM_GUI_PROPERTY(bool, canMoveLeft, setCanMoveLeft)
    PIEJAM_GUI_PROPERTY(bool, canMoveRight, setCanMoveRight)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::AudioRouting*, in)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::AudioRouting*, out)

public:
    MixerChannelEdit(runtime::state_access const&, runtime::mixer::channel_id);

    Q_INVOKABLE void changeName(QString const&);
    Q_INVOKABLE void changeColor(MaterialColor);
    Q_INVOKABLE void moveLeft();
    Q_INVOKABLE void moveRight();
    Q_INVOKABLE void deleteChannel();

private:
    void onSubscribe() override;
};

} // namespace piejam::gui::model
