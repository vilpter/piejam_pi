# SPDX-FileCopyrightText: 2020-2026 Dimitrij Kotrev
#
# SPDX-License-Identifier: CC0-1.0

add_library(piejam_numeric STATIC
    include/piejam/numeric/align.h
    include/piejam/numeric/bit.h
    include/piejam/numeric/clamp.h
    include/piejam/numeric/concepts.h
    include/piejam/numeric/constants.h
    include/piejam/numeric/dB_convert.h
    include/piejam/numeric/dB_lut.h
    include/piejam/numeric/dft.h
    include/piejam/numeric/float_compare.h
    include/piejam/numeric/flush_to_zero_if.h
    include/piejam/numeric/fwd.h
    include/piejam/numeric/generators/cosine_window.h
    include/piejam/numeric/generators/sine.h
    include/piejam/numeric/intops.h
    include/piejam/numeric/intx.h
    include/piejam/numeric/linear_map.h
    include/piejam/numeric/math.h
    include/piejam/numeric/mipp.h
    include/piejam/numeric/mipp_iterator.h
    include/piejam/numeric/normalize.h
    include/piejam/numeric/pos_mod.h
    include/piejam/numeric/pow_n.h
    include/piejam/numeric/rms.h
    include/piejam/numeric/rolling_mean.h
    include/piejam/numeric/simd/clamp.h
    include/piejam/numeric/simd/fsqradd.h
    include/piejam/numeric/simd/lrot_n.h
    include/piejam/numeric/simd/math.h
    include/piejam/numeric/simd/rms.h
    include/piejam/numeric/simd/rolling_sum.h
    include/piejam/numeric/type_traits.h
    src/piejam/numeric/dft.cpp
)

target_include_directories(piejam_numeric PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(piejam_numeric
    PUBLIC
    MIPP

    PRIVATE
    piejam_compiler_warnings
    FFTW::Float)

add_subdirectory(benchmarks)
add_subdirectory(tests)
