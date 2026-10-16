// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/parameter/fwd.h>

#include <boost/container/container_fwd.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/list.hpp>

#include <variant>

namespace piejam::runtime
{

struct midi_assignment;

struct float_parameter_tag;
using float_parameter = parameter::descriptor<float_parameter_tag, float>;
using float_parameter_id = parameter::id_t<float_parameter>;

struct int_parameter_tag;
using int_parameter = parameter::descriptor<int_parameter_tag, int>;
using int_parameter_id = parameter::id_t<int_parameter>;

struct bool_parameter_tag;
using bool_parameter = parameter::descriptor<bool_parameter_tag, bool>;
using bool_parameter_id = parameter::id_t<bool_parameter>;

struct enum_parameter_tag;
using enum_parameter = parameter::descriptor<enum_parameter_tag, int>;
using enum_parameter_id = parameter::id_t<enum_parameter>;

// Store the value type here, so we don't have to include the descriptor
// headers.
using parameters_fwd_t = boost::mp11::
    mp_list<float_parameter, int_parameter, bool_parameter, enum_parameter>;

using parameter_ids_t =
    boost::mp11::mp_transform<parameter::id_t, parameters_fwd_t>;

using parameter_id = boost::mp11::mp_rename<parameter_ids_t, std::variant>;

using parameter_value = boost::mp11::mp_rename<
    boost::mp11::mp_transform<parameter::tagged_value, parameters_fwd_t>,
    std::variant>;

using parameters_map = parameter::map<parameter_id>;

using parameter_value_assignment = parameter::assignment<parameter_value>;
using parameter_midi_assignment = parameter::assignment<midi_assignment>;

enum class parameter_flags : std::size_t
{
    bipolar,
    not_midi_assignable,
    audio_graph_affecting,
    solo_state_affecting,
};

} // namespace piejam::runtime
