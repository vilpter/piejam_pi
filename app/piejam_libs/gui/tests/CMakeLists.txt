# SPDX-FileCopyrightText: 2020-2026 Dimitrij Kotrev
#
# SPDX-License-Identifier: CC0-1.0

if(NOT PIEJAM_TESTS)
    return()
endif()

add_executable(piejam_gui_test
    ${CMAKE_CURRENT_SOURCE_DIR}/DbScaleData_test.cpp
)
target_link_libraries(piejam_gui_test gtest_driver gmock piejam_compiler_warnings piejam_gui)

add_test(NAME piejam_gui_test COMMAND piejam_gui_test)

install(TARGETS piejam_gui_test RUNTIME DESTINATION bin)
