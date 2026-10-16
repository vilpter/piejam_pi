// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/MixerChannelAdd.h>

#include <piejam/runtime/actions/mixer_actions.h>

namespace piejam::gui::model
{

namespace
{

void
addChannel(
    runtime::state_access const& state_access,
    QString const& name,
    runtime::mixer::channel_type type)
{
    runtime::actions::add_mixer_channel action;
    action.name = name.toStdString();
    action.channel_type = type;
    state_access.dispatch(action);
}

} // namespace

MixerChannelAdd::MixerChannelAdd(runtime::state_access const& state_access)
    : SubscribableModel{state_access}
{
}

void
MixerChannelAdd::onSubscribe()
{
}

void
MixerChannelAdd::addMonoChannel(QString const& newChannelName)
{
    addChannel(
        state_access(),
        newChannelName,
        runtime::mixer::channel_type::mono);
}

void
MixerChannelAdd::addStereoChannel(QString const& newChannelName)
{
    addChannel(
        state_access(),
        newChannelName,
        runtime::mixer::channel_type::stereo);
}

void
MixerChannelAdd::addAuxChannel(QString const& newChannelName)
{
    addChannel(
        state_access(),
        newChannelName,
        runtime::mixer::channel_type::aux);
}

} // namespace piejam::gui::model
