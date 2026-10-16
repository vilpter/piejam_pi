// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/AudioRoutingSelection.h>

#include <piejam/runtime/selectors.h>

namespace piejam::gui::model
{

struct AudioRoutingSelection::Impl
{
    runtime::mixer::channel_id mixer_channel_id;
    io_direction io_socket;
};

AudioRoutingSelection::AudioRoutingSelection(
    runtime::state_access const& state_access,
    runtime::mixer::channel_id const id,
    io_direction const io_socket)
    : SubscribableModel(state_access)
    , m_impl{make_pimpl<Impl>(id, io_socket)}
{
}

void
AudioRoutingSelection::onSubscribe()
{
    using namespace runtime::selectors;

    observe(
        make_mixer_channel_selected_route_selector(
            m_impl->mixer_channel_id,
            m_impl->io_socket),
        [this](selected_route const& sel_route) {
            setState([](selected_route::state_t state) {
                switch (state)
                {
                    case selected_route::state_t::valid:
                        return State::Valid;

                    case selected_route::state_t::not_mixed:
                        return State::NotMixed;
                }

                BOOST_ASSERT(false);
                return State::Invalid;
            }(sel_route.state));
            setLabel(QString::fromStdString(*sel_route.name));
        });
}

} // namespace piejam::gui::model
