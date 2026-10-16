# SPDX-FileCopyrightText: 2020-2026 Dimitrij Kotrev
#
# SPDX-License-Identifier: CC0-1.0

add_library(piejam_system STATIC
    include/piejam/system/avg_cpu_load_tracker.h
    include/piejam/system/cpu_load.h
    include/piejam/system/cpu_temp.h
    include/piejam/system/device.h
    include/piejam/system/disk_usage.h
    include/piejam/system/dll.h
    include/piejam/system/file_utils.h
    include/piejam/system/fwd.h
    include/piejam/system/memory.h
    src/piejam/system/avg_cpu_load_tracker.cpp
    src/piejam/system/cpu_load.cpp
    src/piejam/system/cpu_temp.cpp
    src/piejam/system/device.cpp
    src/piejam/system/dll.cpp
    src/piejam/system/file_utils.cpp
    src/piejam/system/memory.cpp
)

target_include_directories(piejam_system PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(piejam_system PUBLIC stdc++fs PRIVATE piejam_compiler_warnings dl)

add_subdirectory(tests)
