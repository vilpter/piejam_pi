# SPDX-FileCopyrightText: 2020-2026 Dimitrij Kotrev
#
# SPDX-License-Identifier: CC0-1.0

add_library(piejam_algorithm INTERFACE
    include/piejam/algorithm/all_of_adjacent.h
    include/piejam/algorithm/concepts.h
    include/piejam/algorithm/copy_or_move.h
    include/piejam/algorithm/edit_script.h
    include/piejam/algorithm/escape_html.h
    include/piejam/algorithm/erase_if.h
    include/piejam/algorithm/find_or_get_first.h
    include/piejam/algorithm/for_each_adjacent.h
    include/piejam/algorithm/for_each_visit.h
    include/piejam/algorithm/index_of.h
    include/piejam/algorithm/npos.h
    include/piejam/algorithm/shift_push_back.h
    include/piejam/algorithm/transform_to_vector.h
)
target_include_directories(piejam_algorithm INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(tests)
