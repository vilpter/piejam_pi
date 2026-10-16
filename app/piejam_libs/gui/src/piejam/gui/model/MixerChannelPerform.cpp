// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/MixerChannelPerform.h>

#include <piejam/gui/model/AudioStreamProvider.h>
#include <piejam/gui/model/BoolParameter.h>
#include <piejam/gui/model/FloatParameter.h>
#include <piejam/gui/model/StereoLevel.h>

#include <piejam/audio/pair.h>
#include <piejam/functional/operators.h>
#include <piejam/numeric/flush_to_zero_if.h>
#include <piejam/numeric/mipp_iterator.h>
#include <piejam/numeric/simd/math.h>
#include <piejam/numeric/simd/rms.h>
#include <piejam/renew.h>
#include <piejam/runtime/selectors.h>

namespace piejam::gui::model
{

namespace
{

template <std::floating_point T>
auto
calcPeakLevel(
    std::span<T const> const samples,
    T const currentLevel,
    audio::sample_rate const sr) -> T
{
    T const blockPeak = mipp::hmax(
        std::transform_reduce(
            numeric::mipp_begin(samples),
            numeric::mipp_end(samples),
            mipp::Reg<T>(T{}),
            numeric::simd::max,
            numeric::simd::abs));

    T const dt = static_cast<T>(samples.size()) / sr.as<T>();
    constexpr T tau = T{0.2}; // 200ms
    T const coeff = std::exp(-dt / tau);
    T const newLevel = std::max(blockPeak, currentLevel * coeff);

    return numeric::flush_to_zero_if(newLevel, less(T{1e-3})); // -60dB
}

template <std::floating_point T>
auto
calcRmsLevel(
    std::span<T const> const samples,
    T const currentLevel,
    audio::sample_rate const sr) -> T
{
    T const blockRms = numeric::simd::rms(samples);

    T const dt = static_cast<T>(samples.size()) / sr.as<T>();

    constexpr T attack = T{0.06}; // 60ms
    constexpr T release = T{0.4}; // 400ms;

    T const tau = blockRms > currentLevel ? attack : release;
    T const coeff = std::exp(-dt / tau);
    T const newLevel = coeff * currentLevel + (1 - coeff) * blockRms;

    return numeric::flush_to_zero_if(newLevel, less(T{1e-8})); // -160dB
}

} // namespace

struct MixerChannelPerform::Impl
{
    MixerChannelPerform& self;

    static constexpr audio::sample_rate default_sample_rate{48000u};
    audio::sample_rate sample_rate{default_sample_rate};

    void updateSampleRate(audio::sample_rate sr)
    {
        if (sr.valid() && sample_rate != sr)
        {
            sample_rate = sr;
        }
    }

    template <audio::pair_channel C>
    void updatePeakLevel(std::span<float const> ch)
    {
        self.m_peakLevel->setLevel<C>(static_cast<double>(calcPeakLevel<float>(
            ch,
            static_cast<float>(self.m_peakLevel->level<C>()),
            sample_rate)));
    }

    template <audio::pair_channel C>
    void updateRmsLevel(std::span<float const> ch)
    {
        self.m_rmsLevel->setLevel<C>(static_cast<double>(calcRmsLevel<float>(
            ch,
            static_cast<float>(self.m_rmsLevel->level<C>()),
            sample_rate)));
    }

    void resetLevelMeter()
    {
        self.m_peakLevel->reset();
        self.m_rmsLevel->reset();
    }
};

MixerChannelPerform::MixerChannelPerform(
    runtime::state_access const& state_access,
    runtime::mixer::channel_id const id)
    : MixerChannel{state_access, id}
    , m_impl{make_pimpl<Impl>(*this)}
    , m_peakLevel{&addQObject<StereoLevel>()}
    , m_rmsLevel{&addQObject<StereoLevel>()}
    , m_volume{&addModel<FloatParameter>(observe_once(
          runtime::selectors::make_mixer_channel_volume_parameter_selector(
              id)))}
    , m_panBalance{&addModel<FloatParameter>(observe_once(
          runtime::selectors::make_mixer_channel_pan_balance_parameter_selector(
              id)))}
    , m_record{&addModel<BoolParameter>(observe_once(
          runtime::selectors::make_mixer_channel_record_parameter_selector(
              id)))}
    , m_solo{&addModel<BoolParameter>(observe_once(
          runtime::selectors::make_mixer_channel_solo_parameter_selector(id)))}
    , m_mute{&addModel<BoolParameter>(observe_once(
          runtime::selectors::make_mixer_channel_mute_parameter_selector(id)))}
{
    AudioStreamProvider& outStream =
        addAttachedModel<AudioStreamProvider>(observe_once(
            runtime::selectors::make_mixer_channel_out_stream_selector(id)));

    QObject::connect(
        &outStream,
        &AudioStreamProvider::captured,
        this,
        [this](AudioStream captured) {
            auto captured_stereo = captured.channels_cast<2>();

            m_impl->updatePeakLevel<audio::pair_channel::left>(
                captured_stereo.channels()[0]);
            m_impl->updatePeakLevel<audio::pair_channel::right>(
                captured_stereo.channels()[1]);

            m_impl->updateRmsLevel<audio::pair_channel::left>(
                captured_stereo.channels()[0]);
            m_impl->updateRmsLevel<audio::pair_channel::right>(
                captured_stereo.channels()[1]);
        });
}

void
MixerChannelPerform::onSubscribe()
{
    m_impl->resetLevelMeter();

    MixerChannel::onSubscribe();

    m_impl->updateSampleRate(
        observe_once(runtime::selectors::select_sample_rate)->current);

    observe(
        runtime::selectors::make_muted_by_solo_selector(channel_id()),
        [this](bool x) { setMutedBySolo(x); });
}

} // namespace piejam::gui::model
