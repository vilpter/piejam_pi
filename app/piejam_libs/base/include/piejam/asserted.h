// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <boost/assert.hpp>

#include <utility>

namespace piejam::asserted
{

template <class N>
[[nodiscard]]
constexpr auto
deref(N&& n) noexcept(noexcept(*std::forward<N>(n))) -> decltype(auto)
{
    BOOST_ASSERT(!!n);          // runtime check
    return *std::forward<N>(n); // safe dereference
}

} // namespace piejam::asserted
