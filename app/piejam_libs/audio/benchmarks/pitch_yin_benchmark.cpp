// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/audio/dsp/pitch_yin.h>

#include <piejam/numeric/generators/sine.h>

#include <mipp.h>

#include <benchmark/benchmark.h>

#include <array>
#include <span>

constexpr std::array freqs{22.34f, 76.53f, 222.33f, 345.67f, 450.99f, 1230.45f};

static void
BM_pitch_yin(benchmark::State& state)
{
    constexpr piejam::audio::sample_rate sr{48000};

    mipp::vector<float> in_buf(8192);
    std::ranges::generate(
        in_buf,
        piejam::numeric::generators::sine<float>(
            sr.as<float>(),
            freqs[state.range(0)]));
    benchmark::ClobberMemory();

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(
            piejam::audio::dsp::pitch_yin<float>(in_buf, sr));
    }
}

BENCHMARK(BM_pitch_yin)->DenseRange(0, freqs.size() - 1);
