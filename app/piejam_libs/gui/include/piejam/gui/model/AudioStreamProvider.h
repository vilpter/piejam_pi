// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/model/AudioStream.h>
#include <piejam/gui/model/SubscribableModel.h>
#include <piejam/gui/model/fwd.h>

#include <piejam/runtime/audio_stream_id.h>

namespace piejam::gui::model
{

class AudioStreamProvider : public SubscribableModel
{
    Q_OBJECT

public:
    AudioStreamProvider(runtime::state_access const&, runtime::audio_stream_id);

signals:
    void captured(piejam::gui::model::AudioStream);

private:
    void onSubscribe() override;

    runtime::audio_stream_id m_stream_id;
};

} // namespace piejam::gui::model
