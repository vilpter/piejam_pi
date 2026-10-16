// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/system/memory.h>

#include <sys/mman.h>

namespace piejam::system
{

auto
mlockall() -> std::error_code
{
    if (::mlockall(MCL_CURRENT | MCL_FUTURE) != 0)
    {
        return std::error_code{errno, std::system_category()};
    }

    return {};
}

} // namespace piejam::system
