// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/ExternalAudioDeviceConfig.h>

#include <piejam/gui/model/String.h>

#include <piejam/runtime/actions/external_audio_device_actions.h>
#include <piejam/runtime/selectors.h>

#include <boost/assert.hpp>

namespace piejam::gui::model
{

ExternalAudioDeviceConfig::ExternalAudioDeviceConfig(
    runtime::state_access const& state_access,
    runtime::external_audio::device_id const device_id)
    : CompositeSubscribableModel(state_access)
    , m_name{&addModel<String>(observe_once(
          runtime::selectors::make_external_audio_device_name_selector(
              device_id)))}
    , m_mono{observe_once(runtime::selectors::make_external_audio_device_bus_type_selector(device_id)) == audio::bus_type::mono}
    , m_device_id{device_id}
{
}

void
ExternalAudioDeviceConfig::onSubscribe()
{
    if (m_mono)
    {
        observe(
            runtime::selectors::make_external_audio_device_bus_channel_selector(
                m_device_id,
                audio::bus_channel::mono),
            [this](std::size_t const ch) { setMonoChannel(ch + 1); });
    }
    else
    {
        observe(
            runtime::selectors::make_external_audio_device_bus_channel_selector(
                m_device_id,
                audio::bus_channel::left),
            [this](std::size_t const ch) { setStereoLeftChannel(ch + 1); });

        observe(
            runtime::selectors::make_external_audio_device_bus_channel_selector(
                m_device_id,
                audio::bus_channel::right),
            [this](std::size_t const ch) { setStereoRightChannel(ch + 1); });
    }
}

static void
changeChannel(
    runtime::state_access state_access,
    runtime::external_audio::device_id const device_id,
    audio::bus_channel const bus_channel,
    unsigned const channel_index)
{
    runtime::actions::set_external_audio_device_bus_channel action;
    action.device_id = device_id;
    action.channel_selector = bus_channel;
    action.channel_index = static_cast<std::size_t>(channel_index) - 1;
    state_access.dispatch(action);
}

void
ExternalAudioDeviceConfig::changeMonoChannel(unsigned const ch)
{
    BOOST_ASSERT(m_mono);
    changeChannel(state_access(), m_device_id, audio::bus_channel::mono, ch);
}

void
ExternalAudioDeviceConfig::changeStereoLeftChannel(unsigned const ch)
{
    BOOST_ASSERT(!m_mono);
    changeChannel(state_access(), m_device_id, audio::bus_channel::left, ch);
}

void
ExternalAudioDeviceConfig::changeStereoRightChannel(unsigned const ch)
{
    BOOST_ASSERT(!m_mono);
    changeChannel(state_access(), m_device_id, audio::bus_channel::right, ch);
}

void
ExternalAudioDeviceConfig::remove()
{
    runtime::actions::remove_external_audio_device action;
    action.device_id = m_device_id;
    dispatch(action);
}

} // namespace piejam::gui::model
