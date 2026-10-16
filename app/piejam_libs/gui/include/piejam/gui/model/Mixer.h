// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/SubscribableModel.h>
#include <piejam/gui/model/fwd.h>

namespace piejam::gui::model
{

class Mixer final : public CompositeSubscribableModel
{
    Q_OBJECT

    PIEJAM_GUI_MODEL_PIMPL

    PIEJAM_GUI_CONSTANT_PROPERTY(QAbstractListModel*, userChannels)
    PIEJAM_GUI_CONSTANT_PROPERTY(
        piejam::gui::model::MixerChannelModels*,
        mainChannel)
    PIEJAM_GUI_CONSTANT_PROPERTY(
        piejam::gui::model::MixerChannelAdd*,
        channelAdd)

public:
    explicit Mixer(runtime::state_access const&);

private:
    void onSubscribe() override;
};

} // namespace piejam::gui::model
