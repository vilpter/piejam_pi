# SPDX-FileCopyrightText: 2020-2026 Dimitrij Kotrev
#
# SPDX-License-Identifier: CC0-1.0

add_library(piejam_range INTERFACE
    include/piejam/range/dynamic_stride.h
    include/piejam/range/indices.h
    include/piejam/range/indirected.h
    include/piejam/range/iota.h
    include/piejam/range/pointer_stride_iterator.h
    include/piejam/range/strided_span.h
    include/piejam/range/table_view.h
)

target_include_directories(piejam_range INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(tests)
