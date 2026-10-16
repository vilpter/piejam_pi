// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/audio_stream.h>
#include <piejam/runtime/processors/fwd.h>

#include <piejam/audio/engine/fwd.h>

#include <memory>
#include <string_view>

namespace piejam::runtime::components
{

auto wrap_with_in_out_stream(
    std::unique_ptr<audio::engine::component> comp,
    std::string_view name,
    runtime::audio_stream_id stream_id,
    runtime::processors::stream_processor_factory& stream_proc_factory,
    std::size_t const buffer_capacity_per_channel)
    -> std::unique_ptr<audio::engine::component>;

} // namespace piejam::runtime::components
