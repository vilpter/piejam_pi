// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/redux/middleware_functors.h>

#include <spdlog/spdlog.h>

#include <exception>

namespace piejam::runtime
{

template <class Action>
class exception_middleware
{
public:
    template <class State>
    void operator()(
        redux::middleware_functors<State, Action> const& mw_fs,
        Action const& a)
    {
        try
        {
            mw_fs.next(a);
        }
        catch (std::exception const& err)
        {
            spdlog::error("unhandled exception: {}", err.what());
            throw;
        }
    }
};

} // namespace piejam::runtime
