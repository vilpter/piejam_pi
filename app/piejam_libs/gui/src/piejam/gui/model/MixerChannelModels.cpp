// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/MixerChannelModels.h>

#include <piejam/gui/model/MixerChannelAuxSend.h>
#include <piejam/gui/model/MixerChannelEdit.h>
#include <piejam/gui/model/MixerChannelFx.h>
#include <piejam/gui/model/MixerChannelPerform.h>

namespace piejam::gui::model
{

MixerChannelModels::MixerChannelModels(
    runtime::state_access const& state_access,
    runtime::mixer::channel_id const id)
    : CompositeSubscribableModel(state_access)
    , m_perform{&addModel<MixerChannelPerform>(id)}
    , m_edit{&addModel<MixerChannelEdit>(id)}
    , m_fx{&addModel<MixerChannelFx>(id)}
    , m_auxSend{&addModel<MixerChannelAuxSend>(id)}
{
}

void
MixerChannelModels::onSubscribe()
{
}

} // namespace piejam::gui::model
