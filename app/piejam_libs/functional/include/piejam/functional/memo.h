// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <boost/assert.hpp>
#include <boost/callable_traits/args.hpp>
#include <boost/callable_traits/return_type.hpp>
#include <boost/mp11/algorithm.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace piejam
{

template <class T>
class memo;

namespace detail
{

template <class>
inline constexpr bool is_memo_dispatch_v = false;

template <class T>
inline constexpr bool is_memo_dispatch_v<memo<T>> = true;

} // namespace detail

template <class T>
inline constexpr bool is_memo_v =
    detail::is_memo_dispatch_v<std::remove_cvref_t<T>>;

template <class F>
class memo
{
public:
    template <class G>
        requires(!is_memo_v<G>)
    explicit memo(G&& f) noexcept(std::is_nothrow_constructible_v<F, G>)
        : m_f{std::forward<G>(f)}
    {
    }

    template <class... Args>
    auto operator()(Args&&... args) const -> std::add_lvalue_reference_t<
        std::add_const_t<std::invoke_result_t<F, Args&&...>>>
    {
        if (!m_last ||
            m_last->args != std::forward_as_tuple(std::forward<Args>(args)...))
        {
            auto args_copy = std::tuple<std::decay_t<Args>...>(args...);
            auto result = std::invoke(m_f, std::forward<Args>(args)...);
            m_last.emplace(std::move(args_copy), std::move(result));
        }

        return m_last->result;
    }

private:
    struct args_and_result
    {
        using args_tuple_type = boost::mp11::
            mp_transform<std::decay_t, boost::callable_traits::args_t<F>>;
        using result_type =
            std::decay_t<boost::callable_traits::return_type_t<F>>;

        template <class Args, class Result>
        args_and_result(Args&& args, Result&& result)
            : args{std::forward<Args>(args)}
            , result{std::forward<Result>(result)}
        {
        }

        args_tuple_type args;
        result_type result;
    };

    F m_f;

    mutable std::optional<args_and_result> m_last{};
};

template <class F>
memo(F&&) -> memo<std::decay_t<F>>;

template <class T>
class shared_memo;

namespace detail
{

template <class>
inline constexpr bool is_shared_memo_dispatch_v = false;

template <class T>
inline constexpr bool is_shared_memo_dispatch_v<memo<T>> = true;

} // namespace detail

template <class T>
inline constexpr bool is_shared_memo_v =
    detail::is_shared_memo_dispatch_v<std::remove_cvref_t<T>>;

template <class F>
class shared_memo
{
public:
    template <class G>
        requires(!is_memo_v<G> && !is_shared_memo_v<G>)
    explicit shared_memo(G&& f)
        : m_memo{std::make_shared<memo<F>>(std::forward<G>(f))}
    {
    }

    template <class... Args>
    auto operator()(Args&&... args) const -> std::add_lvalue_reference_t<
        std::add_const_t<std::invoke_result_t<F, Args&&...>>>
    {
        BOOST_ASSERT(m_memo);
        return (*m_memo)(std::forward<Args>(args)...);
    }

private:
    std::shared_ptr<memo<F>> m_memo;
};

template <class F>
shared_memo(F&&) -> shared_memo<std::decay_t<F>>;

} // namespace piejam
