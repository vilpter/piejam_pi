# SPDX-FileCopyrightText: 2020-2026 Dimitrij Kotrev
#
# SPDX-License-Identifier: CC0-1.0

add_library(piejam_functional INTERFACE
    include/piejam/functional/address_compare.h
    include/piejam/functional/any_equal_to.h
    include/piejam/functional/compose.h
    include/piejam/functional/edge_detect.h
    include/piejam/functional/get.h
    include/piejam/functional/get_if.h
    include/piejam/functional/in_interval.h
    include/piejam/functional/memo.h
    include/piejam/functional/operators.h
)

target_include_directories(piejam_functional INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(tests)
