// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/audio/engine/smoother_processor.h>

#include <piejam/audio/engine/event_buffer.h>
#include <piejam/audio/engine/event_buffer_memory.h>
#include <piejam/audio/engine/event_input_buffers.h>
#include <piejam/audio/engine/event_output_buffers.h>
#include <piejam/audio/engine/process_context.h>
#include <piejam/audio/engine/processor.h>
#include <piejam/audio/slice.h>

#include <piejam/numeric/mipp.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace piejam::audio::engine::test
{

struct smoother_processor_test : public testing::Test
{
    smoother_processor_test()
    {
        ev_ins.add(event_port(std::in_place_type<float>, "ev"));
        ev_ins.set(0, ev_in_buf);
    }

    audio::engine::event_buffer_memory ev_buf_mem{1024};
    std::pmr::memory_resource* ev_buf_pmr_mem{&ev_buf_mem.memory_resource()};
    audio::engine::event_buffer<float> ev_in_buf{ev_buf_pmr_mem};
    audio::engine::event_input_buffers ev_ins;
    event_output_buffers ev_outs{};
    static constexpr std::size_t buffer_size{8};
    alignas(mipp::RequiredAlignment) std::array<float, buffer_size> out_buf{};
    std::array<std::span<float>, 1> outputs{out_buf};
    std::array<slice<float>, 1> results{outputs[0]};
    audio::engine::process_context ctx{
        .outputs = std::span{outputs},
        .results = std::span{results},
        .event_inputs = ev_ins,
        .event_outputs = ev_outs,
        .buffer_size = buffer_size,
    };

    std::array<float, 11>
        lut{0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f};
};

TEST_F(smoother_processor_test, smooth_up)
{
    auto sut = make_lut_smoother_processor(lut, 0.27f);

    ev_in_buf.insert(0, 0.75f);

    sut->process(ctx);

    ASSERT_TRUE(ctx.results[0].is_span());

    EXPECT_THAT(
        ctx.results[0].span(),
        testing::ElementsAre(
            0.3f,
            0.4f,
            0.5f,
            0.6f,
            0.7f,
            0.75f,
            0.75f,
            0.75f));
}

TEST_F(smoother_processor_test, smooth_up_to_max)
{
    auto sut = make_lut_smoother_processor(lut, 0.57f);

    ev_in_buf.insert(2, 1.0f);

    sut->process(ctx);

    ASSERT_TRUE(ctx.results[0].is_span());

    EXPECT_THAT(
        ctx.results[0].span(),
        testing::ElementsAre(0.57f, 0.57f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.0f));
}

TEST_F(smoother_processor_test, smooth_up_from_min)
{
    auto sut = make_lut_smoother_processor(lut, 0.0f);

    ev_in_buf.insert(2, 1.0f);

    sut->process(ctx);

    ASSERT_TRUE(ctx.results[0].is_span());

    EXPECT_THAT(
        ctx.results[0].span(),
        testing::ElementsAre(0.0f, 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f));
}

TEST_F(smoother_processor_test, smooth_up_inside_entry)
{
    auto sut = make_lut_smoother_processor(lut, 0.31f);

    ev_in_buf.insert(2, 0.37f);

    sut->process(ctx);

    ASSERT_TRUE(ctx.results[0].is_span());

    EXPECT_THAT(
        ctx.results[0].span(),
        testing::ElementsAre(
            0.31f,
            0.31f,
            0.37f,
            0.37f,
            0.37f,
            0.37f,
            0.37f,
            0.37f));
}

TEST_F(smoother_processor_test, smooth_up_consecutive)
{
    auto sut = make_lut_smoother_processor(lut, 0.31f);

    ev_in_buf.insert(2, 0.57f);
    ev_in_buf.insert(3, 0.77f);

    sut->process(ctx);

    ASSERT_TRUE(ctx.results[0].is_span());

    EXPECT_THAT(
        ctx.results[0].span(),
        testing::ElementsAre(
            0.31f,
            0.31f,
            0.40f,
            0.50f,
            0.60f,
            0.70f,
            0.77f,
            0.77f));
}

TEST_F(smoother_processor_test, smooth_up_and_up)
{
    auto sut = make_lut_smoother_processor(lut, 0.31f);

    ev_in_buf.insert(0, 0.57f);
    ev_in_buf.insert(3, 0.77f);

    sut->process(ctx);

    ASSERT_TRUE(ctx.results[0].is_span());

    EXPECT_THAT(
        ctx.results[0].span(),
        testing::ElementsAre(
            0.40f,
            0.50f,
            0.57f,
            0.60f,
            0.70f,
            0.77f,
            0.77f,
            0.77f));
}

TEST_F(smoother_processor_test, smooth_down)
{
    auto sut = make_lut_smoother_processor(lut, 0.77f);

    ev_in_buf.insert(0, 0.27f);

    sut->process(ctx);

    ASSERT_TRUE(ctx.results[0].is_span());

    EXPECT_THAT(
        ctx.results[0].span(),
        testing::ElementsAre(
            0.7f,
            0.6f,
            0.5f,
            0.4f,
            0.3f,
            0.27f,
            0.27f,
            0.27f));
}

TEST_F(smoother_processor_test, smooth_down_to_min)
{
    auto sut = make_lut_smoother_processor(lut, 0.47f);

    ev_in_buf.insert(2, 0.0f);

    sut->process(ctx);

    ASSERT_TRUE(ctx.results[0].is_span());

    EXPECT_THAT(
        ctx.results[0].span(),
        testing::ElementsAre(0.47f, 0.47f, 0.4f, 0.3f, 0.2f, 0.1f, 0.0f, 0.0f));
}

TEST_F(smoother_processor_test, smooth_down_from_max)
{
    auto sut = make_lut_smoother_processor(lut, 1.0f);

    ev_in_buf.insert(2, 0.0f);

    sut->process(ctx);

    ASSERT_TRUE(ctx.results[0].is_span());

    EXPECT_THAT(
        ctx.results[0].span(),
        testing::ElementsAre(1.0f, 1.0f, 0.9f, 0.8f, 0.7f, 0.6f, 0.5f, 0.4f));
}

TEST_F(smoother_processor_test, smooth_down_inside_entry)
{
    auto sut = make_lut_smoother_processor(lut, 1.0f);

    ev_in_buf.insert(2, 0.97f);

    sut->process(ctx);

    ASSERT_TRUE(ctx.results[0].is_span());

    EXPECT_THAT(
        ctx.results[0].span(),
        testing::ElementsAre(
            1.0f,
            1.0f,
            0.97f,
            0.97f,
            0.97f,
            0.97f,
            0.97f,
            0.97f));
}

TEST_F(smoother_processor_test, smooth_down_consecutive)
{
    auto sut = make_lut_smoother_processor(lut, 0.71f);

    ev_in_buf.insert(2, 0.57f);
    ev_in_buf.insert(3, 0.44f);

    sut->process(ctx);

    ASSERT_TRUE(ctx.results[0].is_span());

    EXPECT_THAT(
        ctx.results[0].span(),
        testing::ElementsAre(
            0.71f,
            0.71f,
            0.70f,
            0.60f,
            0.50f,
            0.44f,
            0.44f,
            0.44f));
}

TEST_F(smoother_processor_test, smooth_down_and_down)
{
    auto sut = make_lut_smoother_processor(lut, 0.71f);

    ev_in_buf.insert(0, 0.57f);
    ev_in_buf.insert(3, 0.23f);

    sut->process(ctx);

    ASSERT_TRUE(ctx.results[0].is_span());

    EXPECT_THAT(
        ctx.results[0].span(),
        testing::ElementsAre(
            0.7f,
            0.6f,
            0.57f,
            0.50f,
            0.40f,
            0.30f,
            0.23f,
            0.23f));
}

TEST_F(smoother_processor_test, smooth_up_and_down)
{
    auto sut = make_lut_smoother_processor(lut, 0.27f);

    ev_in_buf.insert(0, 0.75f);
    ev_in_buf.insert(4, 0.27f);

    sut->process(ctx);

    ASSERT_TRUE(ctx.results[0].is_span());

    EXPECT_THAT(
        ctx.results[0].span(),
        testing::ElementsAre(0.3f, 0.4f, 0.5f, 0.6f, 0.5f, 0.4f, 0.3f, 0.27f));
}

TEST_F(smoother_processor_test, smooth_down_and_up)
{
    auto sut = make_lut_smoother_processor(lut, 0.77f);

    ev_in_buf.insert(0, 0.27f);
    ev_in_buf.insert(4, 0.75f);

    sut->process(ctx);

    ASSERT_TRUE(ctx.results[0].is_span());

    EXPECT_THAT(
        ctx.results[0].span(),
        testing::ElementsAre(0.7f, 0.6f, 0.5f, 0.4f, 0.5f, 0.6f, 0.7f, 0.75f));
}

TEST_F(smoother_processor_test, smooth_exact_to_exact)
{
    auto sut = make_lut_smoother_processor(lut, 0.5f);

    ev_in_buf.insert(0, 0.3f);
    ev_in_buf.insert(4, 0.8f);

    sut->process(ctx);

    ASSERT_TRUE(ctx.results[0].is_span());

    EXPECT_THAT(
        ctx.results[0].span(),
        testing::ElementsAre(0.4f, 0.3f, 0.3f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f));
}

} // namespace piejam::audio::engine::test
