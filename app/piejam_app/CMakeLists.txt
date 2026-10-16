# SPDX-FileCopyrightText: 2020-2026 Dimitrij Kotrev
#
# SPDX-License-Identifier: CC0-1.0

add_executable(piejam_app main.cpp)
target_link_libraries(piejam_app
    PRIVATE
    piejam_compiler_warnings
    piejam_runtime
    piejam_redux
    piejam_algorithm
    piejam_log
    piejam_gui
    piejam_network_manager
    piejam_fx_modules
    m
)

# Try to link mvec if available
find_library(MVEC_LIB mvec)
if (MVEC_LIB)
    target_link_libraries(piejam_app PRIVATE ${MVEC_LIB})
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    target_compile_definitions(piejam_app PRIVATE QT_QML_DEBUG=1)
endif()
install(TARGETS piejam_app RUNTIME DESTINATION bin)
