// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "FxScope.h"

#include "../scope_internal_id.h"
#include "../scope_module.h"

#include <piejam/audio/types.h>
#include <piejam/gui/model/AudioStreamProvider.h>
#include <piejam/gui/model/BoolParameter.h>
#include <piejam/gui/model/EnumParameter.h>
#include <piejam/gui/model/FloatParameter.h>
#include <piejam/gui/model/IntParameter.h>
#include <piejam/gui/model/ScopeGenerator.h>
#include <piejam/gui/model/ScopeSlot.h>
#include <piejam/gui/model/StreamProcessor.h>
#include <piejam/gui/model/StreamSamplesCache.h>
#include <piejam/gui/model/WaveformGenerator.h>
#include <piejam/gui/model/WaveformSlot.h>
#include <piejam/renew.h>
#include <piejam/runtime/parameter/map.h>
#include <piejam/runtime/selectors.h>

namespace piejam::fx_modules::scope::gui
{

using namespace piejam::gui::model;

namespace
{

struct ScopeStreamProcessor : StreamProcessor<ScopeStreamProcessor>
{
    ScopeStreamProcessor(
        WaveformSlot& waveform,
        ScopeSlot& scope,
        BoolParameter& active,
        EnumParameter& channel,
        FloatParameter& gain)
        : StreamProcessor<ScopeStreamProcessor>{active, channel, gain}
        , waveform{waveform}
        , scope{scope}
    {
    }

    template <class Samples>
    void process(Samples&& samples)
    {
        if (processAsWaveform)
        {
            waveform.update(
                waveformGenerator.process(std::forward<Samples>(samples)));
        }
        else
        {
            scopeCache.process(std::forward<Samples>(samples));
        }
    }

    WaveformSlot& waveform;
    ScopeSlot& scope;

    bool processAsWaveform{true};

    WaveformGenerator waveformGenerator;
    StreamSamplesCache scopeCache;
};

} // namespace

struct FxScope::Impl
{
    static constexpr audio::sample_rate default_sample_rate{48000u};
    audio::sample_rate sample_rate{default_sample_rate};

    std::pair<
        std::optional<ScopeStreamProcessor>,
        std::optional<ScopeStreamProcessor>>
        streamProcessor;
    ScopeGenerator scopeGenerator;

    auto holdTimeInFrames(float holdTime) const noexcept -> std::size_t
    {
        return sample_rate.samples_for_duration(
            std::chrono::milliseconds{static_cast<int>(holdTime)});
    }

    void updateSampleRate(audio::sample_rate sr)
    {
        if (sr.valid())
        {
            sample_rate = sr;
        }
    }

    void updateMode(Mode mode)
    {
        bool processAsWaveform = mode == Mode::Free;
        streamProcessor.first->processAsWaveform = processAsWaveform;
        streamProcessor.second->processAsWaveform = processAsWaveform;
    }

    void updateWaveformGenerator(int waveformResolution)
    {
        int samplesPerPixel = std::pow(2, waveformResolution * 3);
        renew(streamProcessor.first->waveformGenerator, samplesPerPixel);
        renew(streamProcessor.second->waveformGenerator, samplesPerPixel);
    }

    void updateWaveform(std::size_t viewSize)
    {
        streamProcessor.first->waveform.resize(viewSize);
        streamProcessor.second->waveform.resize(viewSize);
    }

    void updateScopeSamplesCache(std::size_t viewSize, int scopeResolution)
    {

        std::size_t doubleViewSize = viewSize * 2;
        int stride = scopeResolution * 2 + 1;
        renew(streamProcessor.first->scopeCache, doubleViewSize, stride);
        renew(streamProcessor.second->scopeCache, doubleViewSize, stride);
    }
};

FxScope::FxScope(
    runtime::state_access const& state_access,
    runtime::fx::module_id const fx_mod_id)
    : FxModule{state_access, fx_mod_id}
    , m_impl{make_pimpl<Impl>()}
    , m_mode{&addModel<EnumParameter>(
          parameters().get<runtime::enum_parameter_id>(parameter_key::mode))}
    , m_triggerSlope{&addModel<EnumParameter>(
          parameters().get<runtime::enum_parameter_id>(
              parameter_key::trigger_slope))}
    , m_triggerLevel{&addModel<FloatParameter>(
          parameters().get<runtime::float_parameter_id>(
              parameter_key::trigger_level))}
    , m_holdTime{&addModel<FloatParameter>(
          parameters().get<runtime::float_parameter_id>(
              parameter_key::hold_time))}
    , m_waveformWindowSize{&addModel<EnumParameter>(
          parameters().get<runtime::enum_parameter_id>(
              parameter_key::waveform_window_size))}
    , m_scopeWindowSize{&addModel<EnumParameter>(
          parameters().get<runtime::enum_parameter_id>(
              parameter_key::scope_window_size))}
    , m_activeA{&addModel<BoolParameter>(
          parameters().get<runtime::bool_parameter_id>(
              parameter_key::stream_a_active))}
    , m_activeB{&addModel<BoolParameter>(
          parameters().get<runtime::bool_parameter_id>(
              parameter_key::stream_b_active))}
    , m_channelA{&addModel<EnumParameter>(
          parameters().get<runtime::enum_parameter_id>(
              parameter_key::channel_a))}
    , m_channelB{&addModel<EnumParameter>(
          parameters().get<runtime::enum_parameter_id>(
              parameter_key::channel_b))}
    , m_gainA{&addModel<FloatParameter>(
          parameters().get<runtime::float_parameter_id>(parameter_key::gain_a))}
    , m_gainB{&addModel<FloatParameter>(
          parameters().get<runtime::float_parameter_id>(parameter_key::gain_b))}
    , m_freeze{&addModel<BoolParameter>(
          parameters().get<runtime::bool_parameter_id>(parameter_key::freeze))}
    , m_waveformA{&addQObject<WaveformSlot>()}
    , m_waveformB{&addQObject<WaveformSlot>()}
    , m_scopeA{&addQObject<ScopeSlot>()}
    , m_scopeB{&addQObject<ScopeSlot>()}
{
    m_impl->streamProcessor.first
        .emplace(*m_waveformA, *m_scopeA, *m_activeA, *m_channelA, *m_gainA);
    m_impl->streamProcessor.second
        .emplace(*m_waveformB, *m_scopeB, *m_activeB, *m_channelB, *m_gainB);

    auto& stream = addAttachedModel<AudioStreamProvider>(
        streams().at(std::to_underlying(stream_key::input)));

    auto clear_fn = [this]() { clear(); };

    if (busType() == BusType::Mono)
    {
        QObject::connect(
            &stream,
            &AudioStreamProvider::captured,
            this,
            [this](AudioStream captured) {
                if (m_freeze->value())
                {
                    return;
                }

                BOOST_ASSERT(captured.num_channels() == 1);
                m_impl->streamProcessor.first->processSamples(
                    captured.samples());

                if (m_mode->as<Mode>() != Mode::Free)
                {
                    auto scope = m_impl->scopeGenerator.process(
                        0,
                        {m_impl->streamProcessor.first->scopeCache.cached()},
                        m_viewSize,
                        m_triggerSlope->as<TriggerSlope>(),
                        m_triggerLevel->valueF(),
                        captured.num_frames(),
                        m_impl->holdTimeInFrames(m_holdTime->valueF()));

                    if (scope.size() == 1)
                    {
                        m_impl->streamProcessor.first->scope.update(scope[0]);
                    }
                }
            });
    }
    else
    {
        QObject::connect(
            &stream,
            &AudioStreamProvider::captured,
            this,
            [this](AudioStream captured) {
                if (m_freeze->value())
                {
                    return;
                }

                auto stereo_captured = captured.channels_cast<2>();

                m_impl->streamProcessor.first->processStereo(stereo_captured);
                m_impl->streamProcessor.second->processStereo(stereo_captured);

                if (m_mode->as<Mode>() != Mode::Free)
                {
                    auto scopeSamples = m_impl->scopeGenerator.process(
                        m_mode->as<Mode>() == Mode::TriggerB,
                        {m_impl->streamProcessor.first->scopeCache.cached(),
                         m_impl->streamProcessor.second->scopeCache.cached()},
                        m_viewSize,
                        m_triggerSlope->as<TriggerSlope>(),
                        m_triggerLevel->valueF(),
                        captured.num_frames(),
                        m_impl->holdTimeInFrames(m_holdTime->valueF()));

                    if (scopeSamples.size() == 2)
                    {
                        m_impl->streamProcessor.first->scope.update(
                            scopeSamples[0]);
                        m_impl->streamProcessor.second->scope.update(
                            scopeSamples[1]);
                    }
                }
            });

        QObject::connect(
            m_activeA,
            &BoolParameter::valueChanged,
            this,
            clear_fn);

        QObject::connect(
            m_channelA,
            &EnumParameter::valueChanged,
            this,
            clear_fn);

        QObject::connect(
            m_activeB,
            &BoolParameter::valueChanged,
            this,
            clear_fn);

        QObject::connect(
            m_channelB,
            &EnumParameter::valueChanged,
            this,
            clear_fn);
    }

    QObject::connect(m_mode, &EnumParameter::valueChanged, this, [this]() {
        m_impl->updateMode(m_mode->as<Mode>());
        clear();
    });

    QObject::connect(
        m_waveformWindowSize,
        &EnumParameter::valueChanged,
        this,
        [this]() {
            m_impl->updateWaveformGenerator(m_waveformWindowSize->value());
        });

    QObject::connect(
        m_scopeWindowSize,
        &EnumParameter::valueChanged,
        this,
        [this]() {
            m_impl->updateScopeSamplesCache(
                m_viewSize,
                m_scopeWindowSize->value());
            clear();
        });

    QObject::connect(this, &FxScope::viewSizeChanged, this, [this]() {
        m_impl->updateWaveform(m_viewSize);
        m_impl->updateScopeSamplesCache(m_viewSize, m_scopeWindowSize->value());
        clear();
    });
}

auto
FxScope::type() const noexcept -> FxModuleType
{
    return {.id = internal_id()};
}

void
FxScope::clear()
{
    m_impl->streamProcessor.first->scope.clear();
    m_impl->streamProcessor.second->scope.clear();
    m_impl->scopeGenerator.clear();

    m_impl->streamProcessor.first->waveform.clear();
    m_impl->streamProcessor.second->waveform.clear();
    m_impl->streamProcessor.first->waveformGenerator.reset();
    m_impl->streamProcessor.second->waveformGenerator.reset();
}

void
FxScope::onSubscribe()
{
    auto sample_rate =
        observe_once(runtime::selectors::select_sample_rate)->current;
    m_impl->updateSampleRate(sample_rate);
    setSampleRate(sample_rate.as<double>());
}

} // namespace piejam::fx_modules::scope::gui
