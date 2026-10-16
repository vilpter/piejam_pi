// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/ladspa/fwd.h>
#include <piejam/runtime/fwd.h>

#include <piejam/pimpl.h>

namespace piejam::runtime
{

class ladspa_fx_middleware final
{
public:
    explicit ladspa_fx_middleware(ladspa::instance_manager&);

    void operator()(middleware_functors const&, action const&);

private:
    template <class Action>
    void process_ladspa_fx_action(middleware_functors const&, Action const&);

    void unload_removed_instances(state const&);

    ladspa::instance_manager& m_ladspa_control;

    struct impl;
    pimpl<impl> m_impl;
};

} // namespace piejam::runtime
