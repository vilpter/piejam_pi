// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/actions/set_parameter_value.h>

#include <piejam/runtime/parameter_factory.h>
#include <piejam/runtime/state.h>

#include <gtest/gtest.h>

namespace piejam::runtime::actions::test
{

struct set_parameter_value_test : testing::Test
{
    state st;

    int_parameter_id basic_param{
        parameter_factory{st.params}.make_parameter(make_int_parameter({
            .name = "basic",
            .default_value = 5,
            .min = 0,
            .max = 10,
        }))};

    int_parameter_id solo_param{parameter_factory{st.params}.make_parameter(
        make_int_parameter({
                               .name = "basic",
                               .default_value = 5,
                               .min = 0,
                               .max = 10,
                           })
            .set_flags({parameter_flags::solo_state_affecting}))};

    int_parameter_id audio_graph_param{
        parameter_factory{st.params}.make_parameter(
            make_int_parameter({
                                   .name = "basic",
                                   .default_value = 5,
                                   .min = 0,
                                   .max = 10,
                               })
                .set_flags({parameter_flags::audio_graph_affecting}))};
};

TEST_F(set_parameter_value_test, setting_basic_param_to_another_value)
{
    EXPECT_EQ(st.params.at(basic_param).get(), 5);
    set_parameter_value{basic_param, 6}.reduce(st);
    EXPECT_EQ(st.params.at(basic_param).get(), 6);

    EXPECT_EQ(st.audio_graph_update_count, 0);
    EXPECT_EQ(st.solo_state_update_count, 0);
}

TEST_F(set_parameter_value_test, setting_basic_param_to_same_value)
{
    EXPECT_EQ(st.params.at(basic_param).get(), 5);
    set_parameter_value{basic_param, 5}.reduce(st);
    EXPECT_EQ(st.params.at(basic_param).get(), 5);

    EXPECT_EQ(st.audio_graph_update_count, 0);
    EXPECT_EQ(st.solo_state_update_count, 0);
}

TEST_F(set_parameter_value_test, setting_solo_param_to_another_value)
{
    EXPECT_EQ(st.params.at(solo_param).get(), 5);
    set_parameter_value{solo_param, 6}.reduce(st);
    EXPECT_EQ(st.params.at(solo_param).get(), 6);

    EXPECT_EQ(st.audio_graph_update_count, 0);
    EXPECT_EQ(st.solo_state_update_count, 1);
}

TEST_F(set_parameter_value_test, setting_solo_param_to_same_value)
{
    EXPECT_EQ(st.params.at(solo_param).get(), 5);
    set_parameter_value{solo_param, 5}.reduce(st);
    EXPECT_EQ(st.params.at(solo_param).get(), 5);

    EXPECT_EQ(st.audio_graph_update_count, 0);
    EXPECT_EQ(st.solo_state_update_count, 0);
}

TEST_F(set_parameter_value_test, setting_audio_graph_param_to_another_value)
{
    EXPECT_EQ(st.params.at(audio_graph_param).get(), 5);
    set_parameter_value{audio_graph_param, 6}.reduce(st);
    EXPECT_EQ(st.params.at(audio_graph_param).get(), 6);

    EXPECT_EQ(st.audio_graph_update_count, 1);
    EXPECT_EQ(st.solo_state_update_count, 0);
}

TEST_F(set_parameter_value_test, setting_audio_graph_param_to_same_value)
{
    EXPECT_EQ(st.params.at(audio_graph_param).get(), 5);
    set_parameter_value{audio_graph_param, 5}.reduce(st);
    EXPECT_EQ(st.params.at(audio_graph_param).get(), 5);

    EXPECT_EQ(st.audio_graph_update_count, 0);
    EXPECT_EQ(st.solo_state_update_count, 0);
}

} // namespace piejam::runtime::actions::test
