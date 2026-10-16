// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/fwd.h>
#include <piejam/runtime/parameter/store.h>

#include <boost/container/flat_map.hpp>

namespace piejam::runtime
{

class parameter_factory
{
public:
    explicit parameter_factory(parameter::store& params)
        : m_params{params}
    {
    }

    template <class P>
    auto make_parameter(P&& param) const
    {
        auto param_id = parameter::id_t<P>::generate();
        m_params.emplace(param_id, std::forward<P>(param));
        return param_id;
    }

private:
    parameter::store& m_params;
};

} // namespace piejam::runtime
