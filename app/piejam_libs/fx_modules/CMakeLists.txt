# SPDX-FileCopyrightText: 2020-2026 Dimitrij Kotrev
#
# SPDX-License-Identifier: CC0-1.0

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Quick REQUIRED)
find_package(Qt5QuickControls2 REQUIRED)

set(RESOURCE_FILES "resources/piejam_fx_modules_resources.qrc")

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    find_package(Qt5QuickCompiler)
    qtquick_compiler_add_resources(RESOURCES ${RESOURCE_FILES})
else()
    qt5_add_resources(RESOURCES ${RESOURCE_FILES})
endif()

list(APPEND PIEJAM_QML_IMPORT_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/resources)
set(PIEJAM_QML_IMPORT_PATHS ${PIEJAM_QML_IMPORT_PATHS} PARENT_SCOPE)

add_library(piejam_fx_modules STATIC
    ${RESOURCES}

    include/piejam/fx_modules/init.h
    src/piejam/fx_modules/init.cpp
    src/piejam/fx_modules/module_registration.h
    src/piejam/fx_modules/module_registration.cpp
)

add_subdirectory(src/piejam/fx_modules/dual_pan)
add_subdirectory(src/piejam/fx_modules/filter)
add_subdirectory(src/piejam/fx_modules/scope)
add_subdirectory(src/piejam/fx_modules/spectrum)
add_subdirectory(src/piejam/fx_modules/tuner)
add_subdirectory(src/piejam/fx_modules/utility)

target_include_directories(piejam_fx_modules PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(piejam_fx_modules
    PUBLIC

    PRIVATE
    piejam_compiler_warnings
    piejam_runtime
    piejam_gui
)

unset(RESOURCE_FILES)
unset(RESOURCES)
