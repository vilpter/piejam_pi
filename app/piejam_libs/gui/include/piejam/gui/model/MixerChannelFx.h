// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/MixerChannel.h>

#include <piejam/runtime/mixer_fwd.h>

class QAbstractListModel;

namespace piejam::gui::model
{

class MixerChannelFx final : public MixerChannel
{
    Q_OBJECT

    PIEJAM_GUI_MODEL_PIMPL

    PIEJAM_GUI_PROPERTY(bool, focused, setFocused)
    PIEJAM_GUI_CONSTANT_PROPERTY(QAbstractListModel*, fxChain)
    PIEJAM_GUI_PROPERTY(bool, canMoveUpFxModule, setCanMoveUpFxModule)
    PIEJAM_GUI_PROPERTY(bool, canMoveDownFxModule, setCanMoveDownFxModule)

public:
    MixerChannelFx(runtime::state_access const&, runtime::mixer::channel_id);

    Q_INVOKABLE void appendFxModule();
    Q_INVOKABLE void moveUpFxModule();
    Q_INVOKABLE void moveDownFxModule();

private:
    void onSubscribe() override;
};

} // namespace piejam::gui::model
