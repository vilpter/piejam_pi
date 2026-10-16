# SPDX-FileCopyrightText: 2020-2026 Dimitrij Kotrev
#
# SPDX-License-Identifier: CC0-1.0

find_package(Qt5 REQUIRED COMPONENTS Core Network)

set(CMAKE_AUTOMOC ON)

add_library(piejam_network_manager STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/piejam/network_manager/fwd.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/piejam/network_manager/network_controller.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/piejam/network_manager/wifi_network.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/piejam/network_manager/wifi_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/piejam/network_manager/wifi_network_model.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/piejam/network_manager/nfs_mount.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/piejam/network_manager/nfs_server.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/piejam/network_manager/nfs_client.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/piejam/network_manager/nfs_mount_model.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/piejam/network_manager/network_controller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/piejam/network_manager/wifi_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/piejam/network_manager/wifi_network_model.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/piejam/network_manager/nfs_server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/piejam/network_manager/nfs_client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/piejam/network_manager/nfs_mount_model.cpp
)

target_include_directories(piejam_network_manager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(piejam_network_manager
    PUBLIC
    Qt5::Core
    Qt5::Network
    PRIVATE
    piejam_compiler_warnings
    piejam_log
)

add_subdirectory(tests)
