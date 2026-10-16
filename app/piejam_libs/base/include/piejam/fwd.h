// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace piejam
{

template <class EntityTag>
struct entity_id;

template <class Entity>
class entity_map;

template <class Id, class Data>
class entity_data_map;

template <class T>
struct io_pair;

template <class T>
class box;

template <class Map>
class boxed_map;

template <class Map>
class lean_map_facade;

} // namespace piejam
