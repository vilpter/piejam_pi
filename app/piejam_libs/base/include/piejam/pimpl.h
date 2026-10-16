// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <boost/assert.hpp>

#include <memory>

namespace piejam
{

namespace detail
{

struct pimpl_deleter
{
    constexpr pimpl_deleter() noexcept
        : m_delete{[]([[maybe_unused]] void* p) { BOOST_ASSERT(!p); }}
    {
    }

    template <class T>
    constexpr pimpl_deleter(std::in_place_type_t<T>) noexcept
        : m_delete{[](void* p) { delete static_cast<T*>(p); }}
    {
    }

    template <class T>
    void operator()(T* p) const noexcept
    {
        m_delete(p);
    }

private:
    using delete_f = void (*)(void*);
    delete_f m_delete;
};

} // namespace detail

template <class T>
using pimpl = std::unique_ptr<T, detail::pimpl_deleter>;

template <class T, class... Args>
auto
make_pimpl(Args&&... args) -> pimpl<T>
{
    return pimpl<T>{
        new T{std::forward<Args>(args)...},
        detail::pimpl_deleter{std::in_place_type<T>}};
}

} // namespace piejam
