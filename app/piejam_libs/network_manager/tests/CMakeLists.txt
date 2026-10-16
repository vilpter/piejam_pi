# SPDX-FileCopyrightText: 2020-2026 Dimitrij Kotrev
#
# SPDX-License-Identifier: CC0-1.0

if(PIEJAM_TESTS)
    add_executable(piejam_network_manager_test
        network_controller_test.cpp
        wifi_manager_test.cpp
        nfs_mount_test.cpp
        nfs_server_test.cpp
        nfs_client_test.cpp
    )

    target_link_libraries(piejam_network_manager_test
        PRIVATE
        piejam_network_manager
        gtest_driver
    )

    add_test(NAME piejam_network_manager_test COMMAND piejam_network_manager_test)
endif()
