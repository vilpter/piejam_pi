// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/fwd.h>

#include <filesystem>

namespace piejam::runtime
{

class persistence_middleware
{
public:
    explicit persistence_middleware(
        std::filesystem::path home_dir,
        std::filesystem::path sessions_dir);

    void operator()(middleware_functors const&, action const&);

private:
    template <class Action>
    void process_persistence_action(middleware_functors const&, Action const&);

    std::filesystem::path m_home_dir;
    std::filesystem::path m_sessions_dir;
    std::filesystem::path m_template_file;
    std::filesystem::path m_last_session_file;
};

} // namespace piejam::runtime
