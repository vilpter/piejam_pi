// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "FxUtility.h"

#include "../utility_internal_id.h"

namespace piejam::fx_modules::utility::gui
{

using namespace piejam::gui::model;

auto
FxUtility::type() const noexcept -> FxModuleType
{
    return {.id = internal_id()};
}

} // namespace piejam::fx_modules::utility::gui
