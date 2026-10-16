// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/actions/audio_engine_sync.h>

#include <piejam/runtime/actions/set_parameter_value.h>
#include <piejam/runtime/parameter/store.h>
#include <piejam/runtime/state.h>

#include <piejam/tuple.h>

#include <boost/mp11/tuple.hpp>

namespace piejam::runtime::actions
{

void
audio_engine_sync_update::reduce(state& st) const
{
    boost::mp11::tuple_for_each(
        values,
        [&st]<class P>(id_value_map_t<P> const& id_value_pairs) {
            for (auto [id, value] : id_value_pairs)
            {
                runtime::set_parameter_value<P>(st, id, value);
            }
        });

    for (auto&& [id, buffer] : streams)
    {
        BOOST_ASSERT(st.streams.contains(id));
        st.streams.assign(id, buffer);
    }
}

auto
audio_engine_sync_update::empty() const noexcept -> bool
{
    return !tuple::for_each_until(values, [](auto const& vs) {
        return vs.empty();
    }) && streams.empty();
}

} // namespace piejam::runtime::actions
