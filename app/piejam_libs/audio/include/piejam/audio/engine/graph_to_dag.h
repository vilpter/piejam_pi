// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/audio/engine/fwd.h>

namespace piejam::audio::engine
{

auto graph_to_dag(graph const&) -> dag;

} // namespace piejam::audio::engine
