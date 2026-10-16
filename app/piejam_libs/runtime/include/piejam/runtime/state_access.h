// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/store.h>
#include <piejam/runtime/subscriber.h>

#include <piejam/redux/subscriber.h>

namespace piejam::runtime
{

class state_access
{
public:
    state_access(
        runtime::store& store,
        runtime::subscriber& state_change_subscriber)
        : m_store{&store}
        , m_state_change_subscriber{state_change_subscriber}
    {
    }

    void dispatch(runtime::action const& a) const;

    template <class Value>
    auto observe_once(runtime::selector<Value> const& sel) const
    {
        return m_state_change_subscriber.observe_once(sel);
    }

    template <class Value, class Handler>
    auto observe(runtime::selector<Value> sel, Handler&& h)
    {
        return m_state_change_subscriber.observe(
            std::move(sel),
            std::forward<Handler>(h));
    }

private:
    runtime::store* m_store;
    runtime::subscriber& m_state_change_subscriber;
};

} // namespace piejam::runtime
