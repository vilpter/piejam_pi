// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/Mixer.h>

#include <piejam/gui/ListModelEditScriptProcessor.h>
#include <piejam/gui/model/MixerChannelAdd.h>
#include <piejam/gui/model/MixerChannelModels.h>
#include <piejam/gui/model/ObjectListModel.h>

#include <piejam/algorithm/edit_script.h>
#include <piejam/audio/types.h>
#include <piejam/runtime/actions/mixer_actions.h>
#include <piejam/runtime/selectors.h>

#include <boost/polymorphic_cast.hpp>

namespace piejam::gui::model
{

struct Mixer::Impl
{
    box<runtime::mixer::channel_ids_t> user_channel_ids;
};

Mixer::Mixer(runtime::state_access const& state_access)
    : CompositeSubscribableModel{state_access}
    , m_impl{make_pimpl<Impl>()}
    , m_userChannels{&addQObject<MixerChannelsList>()}
    , m_mainChannel{&addModel<MixerChannelModels>(
          observe_once(runtime::selectors::select_mixer_main_channel))}
    , m_channelAdd{&addModel<MixerChannelAdd>()}
{
}

void
Mixer::onSubscribe()
{
    observe(
        runtime::selectors::select_mixer_user_channels,
        [this](box<runtime::mixer::channel_ids_t> const& user_channel_ids) {
            algorithm::apply_edit_script(
                algorithm::edit_script(
                    *m_impl->user_channel_ids,
                    *user_channel_ids),
                ListModelEditScriptProcessor{
                    boost::polymorphic_downcast<MixerChannelsList&>(
                        *m_userChannels),
                    [this](auto const& channel_id) {
                        return std::make_unique<MixerChannelModels>(
                            state_access(),
                            channel_id);
                    }});

            m_impl->user_channel_ids = user_channel_ids;
        });
}

} // namespace piejam::gui::model
