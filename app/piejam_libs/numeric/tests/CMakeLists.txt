# SPDX-FileCopyrightText: 2020-2026 Dimitrij Kotrev
#
# SPDX-License-Identifier: CC0-1.0

if(NOT PIEJAM_TESTS)
    return()
endif()

add_executable(piejam_numeric_test
    align_test.cpp
    bit_test.cpp
    clamp_test.cpp
    dB_convert_test.cpp
    float_compare_test.cpp
    generators_cosine_window_test.cpp
    generators_sine_test.cpp
    intops_test.cpp
    intx_test.cpp
    linear_map_test.cpp
    lrot_n_test.cpp
    mipp_iterator_test.cpp
    normalize_test.cpp
    pos_mod_test.cpp
    pow_n_test.cpp
    rms_test.cpp
    rolling_sum_test.cpp
)
target_link_libraries(piejam_numeric_test gtest_driver piejam_compiler_warnings piejam_numeric)

add_test(NAME piejam_numeric_test COMMAND piejam_numeric_test)

install(TARGETS piejam_numeric_test RUNTIME DESTINATION bin)
