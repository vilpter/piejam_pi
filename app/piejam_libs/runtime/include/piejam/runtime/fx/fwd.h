// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/audio_stream.h>
#include <piejam/runtime/parameters.h>

#include <piejam/fwd.h>
#include <piejam/ladspa/fwd.h>

#include <boost/container/container_fwd.hpp>

#include <variant>
#include <vector>

namespace piejam::runtime::fx
{

using internal_id = entity_id<struct internal_id_tag>;

struct unavailable_ladspa;
using unavailable_ladspa_id = entity_id<unavailable_ladspa>;
using unavailable_ladspa_plugins_t = boxed_map<entity_map<unavailable_ladspa>>;

using ladspa_instances_t = boxed_map<
    boost::container::flat_map<ladspa::instance_id, ladspa::plugin_descriptor>>;

using instance_id =
    std::variant<internal_id, ladspa::instance_id, unavailable_ladspa_id>;

class ladspa_manager;

struct registry;

using stream_key = std::size_t;
using module_streams = boost::container::flat_map<stream_key, audio_stream_id>;

struct module;
using module_id = entity_id<module>;
using modules_t = boxed_map<entity_map<module>>;
using chain_t = std::vector<module_id>;

using active_modules_t =
    boxed_map<boost::container::flat_map<fx::module_id, bool_parameter_id>>;

struct state;

} // namespace piejam::runtime::fx
