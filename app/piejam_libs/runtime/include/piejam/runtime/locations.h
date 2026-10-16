// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>

namespace piejam::runtime
{

struct locations
{
    std::filesystem::path home_dir;
    std::filesystem::path config_dir;

    std::filesystem::path log_file{home_dir / "piejam.log"};
    std::filesystem::path config_file{config_dir / "piejam.config"};
    std::filesystem::path last_session_file{home_dir / "last.pjs"};
    std::filesystem::path sessions_dir{home_dir / "sessions"};
    std::filesystem::path recordings_dir{home_dir / "recordings"};
};

} // namespace piejam::runtime
