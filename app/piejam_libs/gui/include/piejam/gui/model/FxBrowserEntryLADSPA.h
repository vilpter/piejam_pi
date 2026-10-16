// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/model/FxBrowserEntry.h>

#include <piejam/entity_id.h>
#include <piejam/ladspa/plugin_descriptor.h>
#include <piejam/runtime/mixer_fwd.h>

namespace piejam::gui::model
{

class FxBrowserEntryLADSPA final : public FxBrowserEntry
{
public:
    FxBrowserEntryLADSPA(
        runtime::state_access const&,
        ladspa::plugin_descriptor const&);

    void appendModule() override;

private:
    void onSubscribe() override;

    ladspa::plugin_descriptor m_pd;
};

} // namespace piejam::gui::model
