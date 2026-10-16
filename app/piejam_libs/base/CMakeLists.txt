# SPDX-FileCopyrightText: 2020-2026 Dimitrij Kotrev
#
# SPDX-License-Identifier: CC0-1.0

add_library(piejam_base INTERFACE
    include/piejam/asserted.h
    include/piejam/bool.h
    include/piejam/box.h
    include/piejam/boxed_map.h
    include/piejam/boxed_string.h
    include/piejam/boxed_vector.h
    include/piejam/enum.h
    include/piejam/entity_data_map.h
    include/piejam/entity_id.h
    include/piejam/entity_id_hash.h
    include/piejam/entity_map.h
    include/piejam/float_constant.h
    include/piejam/fwd.h
    include/piejam/indexed_access.h
    include/piejam/invalid.h
    include/piejam/io_direction.h
    include/piejam/io_pair.h
    include/piejam/lean_map_facade.h
    include/piejam/make_constant.h
    include/piejam/npos.h
    include/piejam/on_scope_exit.h
    include/piejam/pimpl.h
    include/piejam/registry_map.h
    include/piejam/renew.h
    include/piejam/set_if.h
    include/piejam/switch_cast.h
    include/piejam/tuple.h
    include/piejam/tuple_element_compare.h
    include/piejam/type_traits.h
)

target_include_directories(piejam_base INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(tests)
