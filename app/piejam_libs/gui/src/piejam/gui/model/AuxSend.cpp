// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/AuxSend.h>

#include <piejam/gui/model/BoolParameter.h>
#include <piejam/gui/model/EnumParameter.h>
#include <piejam/gui/model/FloatParameter.h>

#include <piejam/runtime/actions/mixer_actions.h>
#include <piejam/runtime/selectors.h>

namespace piejam::gui::model
{

AuxSend::AuxSend(
    runtime::state_access const& state_access,
    runtime::mixer::channel_id channel_id,
    runtime::mixer::channel_id aux_id)
    : CompositeSubscribableModel{state_access}
    , m_active{&addModel<BoolParameter>(observe_once(
          runtime::selectors::make_aux_send_active_selector(
              channel_id,
              aux_id)))}
    , m_faderTap{&addModel<EnumParameter>(observe_once(
          runtime::selectors::make_aux_send_fader_tap_selector(
              channel_id,
              aux_id)))}
    , m_volume{&addModel<FloatParameter>(observe_once(
          runtime::selectors::make_aux_send_volume_parameter_selector(
              channel_id,
              aux_id)))}
    , m_channel_id{channel_id}
    , m_aux_id{aux_id}
{
}

void
AuxSend::onSubscribe()
{
    setName(
        QString::fromStdString(observe_once(
            runtime::selectors::make_mixer_channel_name_string_selector(
                m_aux_id))));

    observe(
        runtime::selectors::make_can_toggle_aux_send_selector(
            m_channel_id,
            m_aux_id),
        [this](bool const x) { setCanToggle(x); });
}

} // namespace piejam::gui::model
