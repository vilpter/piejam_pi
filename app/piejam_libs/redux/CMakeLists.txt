# SPDX-FileCopyrightText: 2020-2026 Dimitrij Kotrev
#
# SPDX-License-Identifier: CC0-1.0

add_library(piejam_redux INTERFACE
    include/piejam/redux/action_tracker_middleware.h
    include/piejam/redux/concepts.h
    include/piejam/redux/functors.h
    include/piejam/redux/fwd.h
    include/piejam/redux/middleware_factory.h
    include/piejam/redux/middleware_functors.h
    include/piejam/redux/queueing_middleware.h
    include/piejam/redux/selector.h
    include/piejam/redux/store.h
    include/piejam/redux/subscriber.h
    include/piejam/redux/subscription.h
    include/piejam/redux/thread_delegate_middleware.h
    include/piejam/redux/thunk_middleware.h
)

target_include_directories(piejam_redux INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(piejam_redux INTERFACE piejam_functional nod)

add_subdirectory(tests)
