# SPDX-FileCopyrightText: 2020-2026 Dimitrij Kotrev
#
# SPDX-License-Identifier: CC0-1.0

add_library(piejam_log INTERFACE
    include/piejam/log/generic_log_sink.h
)

target_include_directories(piejam_log INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(piejam_log INTERFACE spdlog::spdlog_header_only)
target_compile_definitions(piejam_log INTERFACE SPDLOG_USE_STD_FORMAT)
