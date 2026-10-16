// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ladspa_processor_factory_mock.h"
#include "middleware_functors_mock.h"
#include "sound_card_manager_mock.h"

#include <piejam/audio/engine/processor.h>
#include <piejam/audio/io_process.h>
#include <piejam/audio/period_size.h>
#include <piejam/audio/sample_rate.h>
#include <piejam/runtime/actions/initiate_sound_card_selection.h>
#include <piejam/runtime/actions/select_period_size.h>
#include <piejam/runtime/actions/select_sample_rate.h>
#include <piejam/runtime/audio_engine_middleware.h>
#include <piejam/runtime/midi_input_controller.h>
#include <piejam/runtime/state.h>
#include <piejam/thread/configuration.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace piejam::runtime::test
{

struct audio_engine_middleware_test : ::testing::Test
{
    testing::StrictMock<middleware_functors_mock> mf_mock;
    testing::StrictMock<sound_card_manager_mock> sound_card_manager;
    testing::StrictMock<ladspa_processor_factory_mock> ladspa_processor_factory;

    audio_engine_middleware
        sut{{}, {}, sound_card_manager, ladspa_processor_factory, nullptr};
};

TEST_F(
    audio_engine_middleware_test,
    non_device_actions_are_passed_directly_to_next)
{
    struct non_device_action : reducible_action
    {
        [[nodiscard]]
        auto clone() const -> std::unique_ptr<action> override
        {
            return std::make_unique<non_device_action>();
        }

        void reduce(state&) const override
        {
        }
    } action;

    state test_state;

    EXPECT_CALL(mf_mock, get_state())
        .WillRepeatedly(testing::ReturnRef(test_state));
    EXPECT_CALL(mf_mock, next(::testing::Ref(action)));
    sut(make_middleware_functors(mf_mock), action);
}

TEST_F(
    audio_engine_middleware_test,
    select_sample_rate_is_ignored_if_index_is_not_in_current_sample_rates)
{
    using namespace testing;

    state st;
    EXPECT_CALL(mf_mock, get_state()).WillRepeatedly(ReturnRef(st));

    actions::select_sample_rate action;
    sut(make_middleware_functors(mf_mock), action);
}

TEST_F(audio_engine_middleware_test, select_sample_rate)
{
    using namespace testing;

    audio::sound_card_hw_params const default_hw_params{
        .sample_rates =
            {audio::sample_rate(44100u), audio::sample_rate(48000u)},
        .period_sizes = {audio::period_size(64u), audio::period_size(128u)},
    };

    state st;
    st.sound_cards = audio::sound_cards{{
        .name = "foo",
        .num_channels = {},
        .impl_data = 0,
    }};
    st.selected_sound_card.index = 0;
    st.selected_sound_card.hw_params = default_hw_params;
    EXPECT_CALL(mf_mock, get_state()).WillRepeatedly(ReturnRef(st));
    EXPECT_CALL(mf_mock, next(_)).WillRepeatedly([&st](auto const& a) {
        dynamic_cast<reducible_action const&>(a).reduce(st);
    });
    EXPECT_CALL(sound_card_manager, get_hw_params(_, _, _))
        .WillRepeatedly(Return(default_hw_params));
    EXPECT_CALL(sound_card_manager, make_io_process(_, _))
        .WillOnce(Return(ByMove(audio::make_dummy_io_process())));

    actions::select_sample_rate action;
    action.index = 1;

    ASSERT_EQ(audio::sample_rate(), st.sample_rate);
    sut(make_middleware_functors(mf_mock), action);

    EXPECT_EQ(audio::sample_rate(48000u), st.sample_rate);
}

TEST_F(audio_engine_middleware_test, select_period_size)
{
    using namespace testing;

    audio::sound_card_hw_params const default_hw_params{
        .sample_rates =
            {audio::sample_rate(44100u), audio::sample_rate(48000u)},
        .period_sizes = {audio::period_size(64u), audio::period_size(128u)},
    };

    state st;
    st.sound_cards = audio::sound_cards{{
        .name = "foo",
        .num_channels = {},
        .impl_data = 0,
    }};
    st.selected_sound_card.index = 0;
    st.selected_sound_card.hw_params = default_hw_params;
    EXPECT_CALL(mf_mock, get_state()).WillRepeatedly(ReturnRef(st));
    EXPECT_CALL(mf_mock, next(_)).WillRepeatedly([&st](auto const& a) {
        dynamic_cast<reducible_action const&>(a).reduce(st);
    });
    EXPECT_CALL(sound_card_manager, get_hw_params(_, _, _))
        .WillRepeatedly(Return(default_hw_params));
    EXPECT_CALL(sound_card_manager, make_io_process(_, _))
        .WillOnce(Return(ByMove(audio::make_dummy_io_process())));

    actions::select_period_size action;
    action.index = 1;

    ASSERT_EQ(audio::period_size(), st.period_size);
    sut(make_middleware_functors(mf_mock), action);

    EXPECT_EQ(audio::period_size(128u), st.period_size);
}

TEST_F(audio_engine_middleware_test, initiate_device_selection)
{
    using namespace testing;

    piejam::audio::sound_card_hw_params hw_params{
        .sample_rates =
            {audio::sample_rate(44100u),
             audio::sample_rate(48000u),
             audio::sample_rate(96000u)},
        .period_sizes =
            {audio::period_size(128u),
             audio::period_size(192u),
             audio::period_size(256u)},
    };

    state st;
    st.sound_cards = audio::sound_cards{
        {.name = "foo", .num_channels = {1u, 1u}, .impl_data = 0},
        {.name = "bar", .num_channels = {2u, 2u}, .impl_data = 0},
    };

    EXPECT_CALL(mf_mock, get_state()).WillRepeatedly(ReturnRef(st));
    EXPECT_CALL(mf_mock, next(_)).WillRepeatedly([&st](auto const& a) {
        dynamic_cast<reducible_action const&>(a).reduce(st);
    });
    EXPECT_CALL(sound_card_manager, get_hw_params(_, _, _))
        .WillRepeatedly(Return(hw_params));
    EXPECT_CALL(sound_card_manager, make_io_process(_, _))
        .WillOnce(Return(ByMove(audio::make_dummy_io_process())));

    actions::initiate_sound_card_selection in_action;
    in_action.index = 1; // select another sound card

    sut(make_middleware_functors(mf_mock), in_action);

    EXPECT_EQ(1u, st.selected_sound_card.index);
    EXPECT_EQ(2u, st.selected_sound_card.num_channels.in());
    EXPECT_EQ(2u, st.selected_sound_card.num_channels.out());
    EXPECT_EQ(48000u, st.sample_rate.value());
    EXPECT_EQ(192u, st.period_size.value());
}

} // namespace piejam::runtime::test
