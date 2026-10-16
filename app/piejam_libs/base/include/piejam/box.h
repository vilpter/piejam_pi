// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/on_scope_exit.h>

#include <boost/assert.hpp>
#include <boost/hof/returns.hpp>

#include <concepts>
#include <memory>
#include <utility>

namespace piejam
{

template <class T>
class box;

namespace detail
{

template <class>
inline constexpr bool is_box_dispatch_v = false;

template <class T>
inline constexpr bool is_box_dispatch_v<box<T>> = true;

} // namespace detail

template <class T>
inline constexpr bool is_box_v =
    detail::is_box_dispatch_v<std::remove_cvref_t<T>>;

namespace detail
{

template <class From, class To>
concept convertible_to_box_value =
    std::convertible_to<From, To> && !is_box_v<From>;

} // namespace detail

template <class T>
class box
{
public:
    using value_type = T;

    static_assert(
        std::is_same_v<T, std::remove_cvref_t<T>>,
        "T must be a non-cv, non-reference type");

    class write_lock
    {
    public:
        ~write_lock()
        {
            m_box.m_value = m_value;
            BOOST_ASSERT_MSG(
                std::exchange(m_box.m_locked, false),
                "box should be locked for writing");
        }

        write_lock(write_lock const&) = delete;
        write_lock(write_lock&&) = delete;

        auto operator=(write_lock const&) = delete;
        auto operator=(write_lock&&) = delete;

        auto get() const noexcept -> T&
        {
            return *m_value;
        }

        auto operator*() const noexcept -> T&
        {
            return *m_value;
        }

        auto operator->() const noexcept -> T*
        {
            return m_value.get();
        }

    private:
        explicit write_lock(box<T>& b)
            : m_box{b}
        {
            BOOST_ASSERT_MSG(
                !std::exchange(m_box.m_locked, true),
                "box is already locked for writing");
        }

        friend class box<T>;

        box<T>& m_box;
        std::shared_ptr<T> m_value{std::make_shared<T>(*m_box.m_value)};
    };

    box() = default;

    template <detail::convertible_to_box_value<T const> U>
    explicit box(U&& v)
        : box(std::in_place, std::forward<U>(v))
    {
    }

    template <class... Args>
    box(std::in_place_t, Args&&... args)
        : m_value(std::make_shared<T const>(std::forward<Args>(args)...))
    {
    }

    box(box const&) = default;

    box(box&& other) noexcept
        : m_value{other.m_value}
    {
    }

    ~box() = default;

    auto operator=(box const&) -> box& = default;

    auto operator=(box&& other) noexcept -> box&
    {
        m_value = other.m_value;
        return *this;
    }

    auto get() const noexcept -> T const&
    {
        return *m_value;
    }

    operator T const&() const noexcept
    {
        return *m_value;
    }

    auto operator*() const noexcept -> T const&
    {
        return *m_value;
    }

    auto operator->() const noexcept -> T const*
    {
        return m_value.get();
    }

    template <detail::convertible_to_box_value<T const> U>
    auto operator=(U&& value) -> box&
    {
        m_value = std::make_shared<T const>(std::forward<U>(value));
        return *this;
    }

    auto lock() -> write_lock
    {
        return write_lock{*this};
    }

    auto operator==(box const& other) const noexcept -> bool
    {
        return m_value.get() == other.m_value.get();
    }

    auto operator!=(box const& other) const noexcept -> bool
    {
        return !(*this == other);
    }

private:
    static auto get_default() -> std::shared_ptr<T const>
    {
        static std::shared_ptr<T const> const s_default{
            std::make_shared<T>(T{})};
        return s_default;
    }

    std::shared_ptr<T const> m_value{get_default()};

#ifndef NDEBUG
    bool m_locked{false};
#endif
};

template <class T, class U>
    requires(!is_box_v<U> && std::equality_comparable_with<T, U>)
auto operator==(box<T> const& lhs, U const& rhs)
    BOOST_HOF_RETURNS(lhs.get() == rhs);

template <class T, class U>
    requires(!is_box_v<U> && std::equality_comparable_with<T, U>)
auto operator==(U const& lhs, box<T> const& rhs)
    BOOST_HOF_RETURNS(lhs == rhs.get());

template <class T, class U>
    requires(!is_box_v<U> && std::totally_ordered_with<T, U>)
auto operator<=>(box<T> const& lhs, U const& rhs)
    BOOST_HOF_RETURNS(lhs.get() <=> rhs);

template <class T, class U>
    requires(!is_box_v<U> && std::totally_ordered_with<T, U>)
auto operator<=>(U const& lhs, box<T> const& rhs)
    BOOST_HOF_RETURNS(lhs <=> rhs.get());

template <class T>
    requires(!is_box_v<T>)
box(T&&) -> box<std::remove_cvref_t<T>>;

} // namespace piejam
