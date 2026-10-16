// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/audio/engine/fwd.h>

#include <memory>
#include <span>
#include <string_view>

namespace piejam::audio::engine
{

auto make_lut_smoother_processor(
    std::span<float const> lut,
    float current,
    std::string_view name = {}) -> std::unique_ptr<processor>;

} // namespace piejam::audio::engine
