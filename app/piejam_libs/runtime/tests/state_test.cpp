// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/state.h>

#include <piejam/numeric/dB_convert.h>
#include <piejam/runtime/parameter/store.h>

#include <gtest/gtest.h>

namespace piejam::runtime::test
{

struct state_with_one_mixer_input : testing::Test
{
    state_with_one_mixer_input()
        : channel_id(add_mixer_channel(sut, mixer::channel_type::mono, "foo"))
    {
    }

    state sut;
    mixer::channel_id channel_id;
    mixer::channel const* channel{};
};

TEST_F(state_with_one_mixer_input, after_add_mixer_channel)
{
    ASSERT_EQ(1u, sut.mixer_state.inputs->size());

    auto const& channel = sut.mixer_state.channels.at(channel_id);

    EXPECT_EQ("foo", *sut.strings.at(channel.name));

    auto const& volume_param = sut.params.at(channel.volume()).param();
    EXPECT_EQ(1.f, volume_param.default_value);
    EXPECT_EQ(0.f, volume_param.min);
    EXPECT_EQ(numeric::from_dB(6.f), volume_param.max);
    auto volume_value = sut.params.at(channel.volume()).get();
    EXPECT_EQ(1.f, volume_value);

    auto const& pan_balance_param =
        sut.params.at(channel.pan_balance()).param();
    EXPECT_EQ(0.f, pan_balance_param.default_value);
    EXPECT_EQ(-1.f, pan_balance_param.min);
    EXPECT_EQ(1.f, pan_balance_param.max);
    auto pan_balance_value = sut.params.at(channel.pan_balance()).get();
    EXPECT_EQ(0.f, pan_balance_value);

    auto const& mute_param = sut.params.at(channel.mute()).param();
    EXPECT_EQ(false, mute_param.default_value);
    auto mute_value = sut.params.at(channel.mute()).get();
    EXPECT_EQ(false, mute_value);
}

TEST_F(
    state_with_one_mixer_input,
    removing_mixer_channel_removes_also_its_parameters)
{
    ASSERT_EQ(1u, sut.mixer_state.inputs->size());

    remove_mixer_channel(sut, channel_id);

    EXPECT_TRUE(sut.mixer_state.inputs->empty());
    EXPECT_EQ(nullptr, sut.mixer_state.channels.find(channel_id));
}

} // namespace piejam::runtime::test
