# SPDX-FileCopyrightText: 2020-2026 Dimitrij Kotrev
#
# SPDX-License-Identifier: CC0-1.0

if(NOT PIEJAM_TESTS)
    return()
endif()

add_executable(piejam_thread_test
    ${CMAKE_CURRENT_SOURCE_DIR}/spsc_slot_test.cpp
)
target_link_libraries(piejam_thread_test gtest_driver gmock piejam_compiler_warnings piejam_thread)

add_test(NAME piejam_thread_test COMMAND piejam_thread_test)

install(TARGETS piejam_thread_test RUNTIME DESTINATION bin)
