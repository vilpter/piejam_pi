# SPDX-FileCopyrightText: 2020-2026 Dimitrij Kotrev
#
# SPDX-License-Identifier: CC0-1.0

if(NOT PIEJAM_TESTS)
    return()
endif()

add_executable(piejam_base_test
    box_test.cpp
    decay_tuple_elements_test.cpp
    entity_data_map_test.cpp
    entity_map_test.cpp
    indexed_access_test.cpp
    renew_test.cpp
    switch_cast_test.cpp
    tuple_test.cpp
    type_traits_test.cpp
)
target_link_libraries(piejam_base_test gtest_driver piejam_compiler_warnings piejam_base)

add_test(NAME piejam_base_test COMMAND piejam_base_test)

install(TARGETS piejam_base_test RUNTIME DESTINATION bin)
