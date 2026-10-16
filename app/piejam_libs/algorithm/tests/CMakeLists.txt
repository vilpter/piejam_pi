# SPDX-FileCopyrightText: 2020-2026 Dimitrij Kotrev
#
# SPDX-License-Identifier: CC0-1.0

if(NOT PIEJAM_TESTS)
    return()
endif()

add_executable(piejam_algorithm_test
    all_of_adjacent_test.cpp
    copy_or_move_test.cpp
    edit_script_test.cpp
    escape_html_test.cpp
    find_or_get_first_test.cpp
    for_each_adjacent_test.cpp
    for_each_visit_test.cpp
    index_of_test.cpp
    shift_push_back_test.cpp
    transform_to_vector_test.cpp
)
target_link_libraries(piejam_algorithm_test gtest_driver piejam_compiler_warnings piejam_algorithm)

add_test(NAME piejam_algorithm_test COMMAND piejam_algorithm_test)

install(TARGETS piejam_algorithm_test RUNTIME DESTINATION bin)
