// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/AuxChannel.h>

#include <piejam/gui/model/EnumParameter.h>

#include <piejam/runtime/actions/mixer_actions.h>
#include <piejam/runtime/selectors.h>

namespace piejam::gui::model
{

AuxChannel::AuxChannel(
    runtime::state_access const& state_access,
    runtime::mixer::channel_id aux_id)
    : CompositeSubscribableModel{state_access}
    , m_defaultFaderTap{&addModel<EnumParameter>(observe_once(
          runtime::selectors::
              make_aux_channel_default_fader_tap_parameter_selector(aux_id)))}
{
}

void
AuxChannel::onSubscribe()
{
}

} // namespace piejam::gui::model
