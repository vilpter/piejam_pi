// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/fwd.h>
#include <piejam/runtime/ui/cloneable_action.h>

namespace piejam::runtime::actions
{

struct set_wifi_enabled final
    : ui::cloneable_action<set_wifi_enabled, reducible_action>
{
    bool enabled{};
    void reduce(state&) const override;
};

struct set_nfs_server_enabled final
    : ui::cloneable_action<set_nfs_server_enabled, reducible_action>
{
    bool enabled{};
    void reduce(state&) const override;
};

} // namespace piejam::runtime::actions
