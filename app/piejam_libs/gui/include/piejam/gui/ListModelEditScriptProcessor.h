// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/algorithm/edit_script.h>

namespace piejam::gui
{

template <class ListModel, class ListItemFactory>
struct ListModelEditScriptProcessor
{
    ListModel& list;
    ListItemFactory makeListItem;

    void operator()(algorithm::edit_script_deletion const& del) const
    {
        list.remove(del.pos);
    }

    template <class EditValue>
    void
    operator()(algorithm::edit_script_insertion<EditValue> const& ins) const
    {
        list.add(ins.pos, makeListItem(ins.value));
    }
};

template <class ListModel, class ListItemFactory>
ListModelEditScriptProcessor(ListModel&, ListItemFactory&&)
    -> ListModelEditScriptProcessor<ListModel, ListItemFactory>;

} // namespace piejam::gui
