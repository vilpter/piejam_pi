// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/FxBrowserEntryLADSPA.h>

#include <piejam/npos.h>
#include <piejam/runtime/actions/insert_fx_module.h>
#include <piejam/runtime/midi_assignment.h>
#include <piejam/runtime/parameter/assignment.h>
#include <piejam/runtime/selectors.h>
#include <piejam/runtime/ui/thunk_action.h>

namespace piejam::gui::model
{

FxBrowserEntryLADSPA::FxBrowserEntryLADSPA(
    runtime::state_access const& state_access,
    ladspa::plugin_descriptor const& plugin_desc)
    : FxBrowserEntry(state_access)
    , m_pd(plugin_desc)
{
    static QString s_section_ladspa{tr("LADSPA")};

    setName(QString::fromStdString(m_pd.name));
    setSection(s_section_ladspa);

    if (!m_pd.name.empty())
    {
        setAuthor(QString::fromStdString(m_pd.author));
    }

    if (!m_pd.copyright.empty())
    {
        setDescription(QString("Copyright / License: %1")
                           .arg(QString::fromStdString(m_pd.copyright)));
    }
}

void
FxBrowserEntryLADSPA::onSubscribe()
{
}

void
FxBrowserEntryLADSPA::appendModule()
{
    runtime::actions::load_ladspa_fx_plugin action;
    action.fx_chain_id =
        observe_once(runtime::selectors::select_fx_browser_fx_chain);
    action.position = npos;
    action.plugin_id = m_pd.id;
    action.name = m_pd.name;
    action.show_fx_module = true;
    dispatch(action);
}

} // namespace piejam::gui::model
