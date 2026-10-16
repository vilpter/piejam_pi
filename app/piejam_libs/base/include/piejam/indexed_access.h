// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <boost/assert.hpp>

#include <iterator>

namespace piejam
{

template <class Container, class T>
auto
insert_at(Container& c, std::size_t index, T&& value)
{
    using std::begin;
    using std::size;

    BOOST_ASSERT(index <= size(c));

    return c.insert(std::next(begin(c), index), std::forward<T>(value));
}

template <class Container>
auto
erase_at(Container& c, std::size_t index)
{
    using std::begin;
    using std::size;

    BOOST_ASSERT(index < size(c));

    return c.erase(std::next(begin(c), index));
}

} // namespace piejam
