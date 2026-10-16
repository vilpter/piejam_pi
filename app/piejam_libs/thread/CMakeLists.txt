# SPDX-FileCopyrightText: 2020-2026 Dimitrij Kotrev
#
# SPDX-License-Identifier: CC0-1.0

add_library(piejam_thread STATIC
    include/piejam/thread/affinity.h
    include/piejam/thread/alloc_debug.h
    include/piejam/thread/cache_line_size.h
    include/piejam/thread/configuration.h
    include/piejam/thread/cpu_clock.h
    include/piejam/thread/cpu_util.h
    include/piejam/thread/fwd.h
    include/piejam/thread/name.h
    include/piejam/thread/priority.h
    include/piejam/thread/spsc_slot.h
    src/piejam/thread/affinity.cpp
    src/piejam/thread/alloc_debug.cpp
    src/piejam/thread/configuration.cpp
    src/piejam/thread/cpu_clock.cpp
    src/piejam/thread/cpu_util.cpp
    src/piejam/thread/name.cpp
    src/piejam/thread/priority.cpp
)

target_include_directories(piejam_thread PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(piejam_thread
    PRIVATE
    $<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},x86_64>>:-msse>
)

target_link_libraries(piejam_thread PRIVATE piejam_compiler_warnings pthread)

add_subdirectory(benchmarks)
add_subdirectory(tests)
