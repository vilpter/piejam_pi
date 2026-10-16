// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/model/SubscribableModel.h>

namespace piejam::gui::model
{

class MixerChannelAdd : public SubscribableModel
{
    Q_OBJECT

public:
    MixerChannelAdd(runtime::state_access const&);

    Q_INVOKABLE void addMonoChannel(QString const& newChannelName);
    Q_INVOKABLE void addStereoChannel(QString const& newChannelName);
    Q_INVOKABLE void addAuxChannel(QString const& newChannelName);

private:
    void onSubscribe() override;
};

} // namespace piejam::gui::model
