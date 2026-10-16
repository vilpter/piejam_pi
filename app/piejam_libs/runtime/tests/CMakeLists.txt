# SPDX-FileCopyrightText: 2020-2026 Dimitrij Kotrev
#
# SPDX-License-Identifier: CC0-1.0

if(NOT PIEJAM_TESTS)
    return()
endif()

add_executable(piejam_runtime_test
    audio_engine_middleware_test.cpp
    fader_mappiing_test.cpp
    ladspa_fx_middleware_test.cpp
    ladspa_instance_manager_mock.h
    ladspa_processor_factory_mock.h
    middleware_functors_mock.h
    midi_assignment_processor_test.cpp
    midi_control_middleware_test.cpp
    midi_input_controller_mock.h
    midi_input_processor_test.cpp
    midi_learn_processor_test.cpp
    midi_to_parameter_processor_test.cpp
    mute_solo_processor_test.cpp
    parameter_processor_factory_test.cpp
    parameters_store_test.cpp
    set_parameter_value_test.cpp
    sound_card_manager_mock.h
    state_test.cpp
    stream_processor_factory_test.cpp
)
target_link_libraries(piejam_runtime_test gtest_driver gmock piejam_compiler_warnings piejam_runtime)

add_test(NAME piejam_runtime_test COMMAND piejam_runtime_test)

install(TARGETS piejam_runtime_test RUNTIME DESTINATION bin)

add_executable(piejam_runtime_render_test audio_engine_render_test.cpp)
target_link_libraries(piejam_runtime_render_test gtest_driver piejam_runtime)
target_compile_options(piejam_runtime_render_test PRIVATE -Wall -Wextra -Werror -pedantic-errors)
