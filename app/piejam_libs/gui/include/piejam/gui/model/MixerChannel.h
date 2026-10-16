// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/SubscribableModel.h>
#include <piejam/gui/model/Types.h>
#include <piejam/gui/model/fwd.h>

#include <piejam/runtime/mixer_fwd.h>

namespace piejam::gui::model
{

class MixerChannel : public CompositeSubscribableModel
{
    Q_OBJECT

    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::ChannelType, channelType)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::BusType, busType)
    PIEJAM_GUI_PROPERTY(QString, name, setName)
    PIEJAM_GUI_PROPERTY(MaterialColor, color, setColor)

public:
    MixerChannel(runtime::state_access const&, runtime::mixer::channel_id);
    ~MixerChannel() override;

protected:
    auto channel_id() const noexcept -> runtime::mixer::channel_id
    {
        return m_channel_id;
    }

    void onSubscribe() override;

private:
    runtime::mixer::channel_id m_channel_id;
};

} // namespace piejam::gui::model
