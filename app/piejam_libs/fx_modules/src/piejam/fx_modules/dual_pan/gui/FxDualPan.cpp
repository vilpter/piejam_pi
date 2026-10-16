// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "FxDualPan.h"

#include "../dual_pan_internal_id.h"

namespace piejam::fx_modules::dual_pan::gui
{

using namespace piejam::gui::model;

auto
FxDualPan::type() const noexcept -> FxModuleType
{
    return {.id = internal_id()};
}

} // namespace piejam::fx_modules::dual_pan::gui
