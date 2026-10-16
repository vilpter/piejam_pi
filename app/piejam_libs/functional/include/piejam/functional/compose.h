// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <boost/callable_traits/args.hpp>

#include <functional>
#include <utility>

namespace piejam::functional
{

struct compose_fn
{
    // Base case: single callable
    template <class F>
    constexpr auto operator()(F&& f) const -> decltype(auto)
    {
        return std::forward<F>(f);
    }

    // Recursive case: at least two callables
    template <class F, class G, class... Rest>
    constexpr auto operator()(F&& f, G&& g, Rest&&... rest) const
    {
        // Compose the inner functions first
        auto inner_composed =
            (*this)(std::forward<G>(g), std::forward<Rest>(rest)...);

        // Extract argument types of the innermost function
        using args_t = boost::callable_traits::args_t<
            std::remove_cvref_t<decltype(inner_composed)>>;

        // Dispatch helper: fixes argument types of lambda to match innermost
        return dispatch(
            std::forward<F>(f),
            std::move(inner_composed),
            std::in_place_type<args_t>);
    }

private:
    template <class F, class Composed, class... Args>
    static constexpr auto
    dispatch(F&& f, Composed&& c, std::in_place_type_t<std::tuple<Args...>>)
    {
        return [f = std::forward<F>(f),
                c = std::forward<Composed>(c)](Args... args) mutable
                   -> std::invoke_result_t<
                       std::remove_reference_t<F>,
                       std::invoke_result_t<
                           std::remove_reference_t<Composed>,
                           Args...>> {
            return std::invoke(f, std::invoke(c, std::forward<Args>(args)...));
        };
    }
};

inline constexpr compose_fn compose{};

} // namespace piejam::functional
