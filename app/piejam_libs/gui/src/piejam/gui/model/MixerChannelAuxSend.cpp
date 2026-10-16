// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/MixerChannelAuxSend.h>

#include <piejam/gui/ListModelEditScriptProcessor.h>
#include <piejam/gui/model/AuxChannel.h>
#include <piejam/gui/model/AuxSend.h>
#include <piejam/gui/model/ObjectListModel.h>

#include <piejam/algorithm/edit_script.h>
#include <piejam/runtime/selectors.h>

#include <boost/polymorphic_cast.hpp>

namespace piejam::gui::model
{

struct MixerChannelAuxSend::Impl
{
    box<runtime::mixer::channel_ids_t> send_ids;
};

MixerChannelAuxSend::MixerChannelAuxSend(
    runtime::state_access const& state_access,
    runtime::mixer::channel_id const id)
    : MixerChannel{state_access, id}
    , m_impl{make_pimpl<Impl>()}
    , m_sends{channelType() != ChannelType::Aux ? &addQObject<AuxSendsList>() : nullptr}
    , m_aux{
          channelType() == ChannelType::Aux ? &addModel<AuxChannel>(id)
                                            : nullptr}
{
}

void
MixerChannelAuxSend::onSubscribe()
{
    MixerChannel::onSubscribe();

    if (channelType() != ChannelType::Aux)
    {
        observe(
            runtime::selectors::select_mixer_aux_channels,
            [this](box<runtime::mixer::channel_ids_t> const& send_ids) {
                algorithm::apply_edit_script(
                    algorithm::edit_script(*m_impl->send_ids, *send_ids),
                    ListModelEditScriptProcessor{
                        boost::polymorphic_downcast<AuxSendsList&>(*m_sends),
                        [this](auto const& send_id) {
                            return std::make_unique<AuxSend>(
                                state_access(),
                                channel_id(),
                                send_id);
                        }});

                m_impl->send_ids = send_ids;
            });
    }
}

} // namespace piejam::gui::model
