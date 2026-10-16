// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/AudioStreamProvider.h>

#include <piejam/runtime/audio_stream.h>
#include <piejam/runtime/selectors.h>

namespace piejam::gui::model
{

AudioStreamProvider::AudioStreamProvider(
    runtime::state_access const& state_access,
    runtime::audio_stream_id stream_id)
    : SubscribableModel(state_access)
    , m_stream_id{stream_id}
{
}

void
AudioStreamProvider::onSubscribe()
{
    observe(
        runtime::selectors::make_audio_stream_selector(m_stream_id),
        [this](runtime::audio_stream_buffer const& buf) {
            if (!buf->empty())
            {
                emit captured(buf->view());
            }
        });
}

} // namespace piejam::gui::model
