# SPDX-FileCopyrightText: 2020-2026 Dimitrij Kotrev
#
# SPDX-License-Identifier: CC0-1.0

if(NOT PIEJAM_TESTS)
    return()
endif()

add_executable(piejam_redux_test
    ${CMAKE_CURRENT_SOURCE_DIR}/queueing_middleware_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/selector_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/store_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/subscriber_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thread_delegate_middleware_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thunk_middleware_test.cpp
)
target_link_libraries(piejam_redux_test gtest_driver piejam_compiler_warnings piejam_redux)

add_test(NAME piejam_redux_test COMMAND piejam_redux_test)

install(TARGETS piejam_redux_test RUNTIME DESTINATION bin)
