// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/audio/pcm_buffer_converter.h>

#include <chrono>
#include <functional>
#include <span>

namespace piejam::audio
{

using init_process_function = std::function<void(
    std::span<pcm_input_buffer_converter const>,
    std::span<pcm_output_buffer_converter const>)>;

// return process execution time, possibly average over worker threads
using process_function =
    std::function<std::chrono::nanoseconds(std::size_t /*buffer_size*/)>;

} // namespace piejam::audio
