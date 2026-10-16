// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/FxBrowserEntryInternal.h>

#include <piejam/gui/model/FxBrowserEntryInternalData.h>
#include <piejam/npos.h>
#include <piejam/runtime/actions/insert_fx_module.h>
#include <piejam/runtime/selectors.h>

namespace piejam::gui::model
{

FxBrowserEntryInternal::FxBrowserEntryInternal(
    runtime::state_access const& state_access,
    runtime::fx::internal_id const fx_type)
    : FxBrowserEntry(state_access)
    , m_fx_type(fx_type)
{
    static QString s_section_internal{tr("Internal")};
    setSection(s_section_internal);

    auto const& data = FxBrowserEntryInternalDataMap::lookup(fx_type);
    setName(data.name);
    setDescription(data.description);
}

void
FxBrowserEntryInternal::onSubscribe()
{
}

void
FxBrowserEntryInternal::appendModule()
{
    runtime::actions::insert_internal_fx_module action;
    action.fx_chain_id =
        observe_once(runtime::selectors::select_fx_browser_fx_chain);
    action.position = npos;
    action.type = m_fx_type;
    action.show_fx_module = true;
    dispatch(action);
}

} // namespace piejam::gui::model
