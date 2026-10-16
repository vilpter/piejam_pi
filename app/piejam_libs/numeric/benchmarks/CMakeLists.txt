# SPDX-FileCopyrightText: 2020-2026 Dimitrij Kotrev
#
# SPDX-License-Identifier: CC0-1.0

if(NOT PIEJAM_BENCHMARKS)
    return()
endif()

find_package(benchmark REQUIRED)

add_executable(piejam_numeric_benchmark
    rms_benchmark.cpp
)
target_link_libraries(piejam_numeric_benchmark benchmark benchmark_main piejam_numeric)
target_compile_options(piejam_numeric_benchmark PRIVATE -Wall -Wextra -Werror -pedantic-errors)

install(TARGETS piejam_numeric_benchmark RUNTIME DESTINATION bin)
