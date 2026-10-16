// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "FxSpectrum.h"

#include "../spectrum_internal_id.h"
#include "../spectrum_module.h"

#include <piejam/functional/operators.h>
#include <piejam/gui/model/AudioStreamProvider.h>
#include <piejam/gui/model/BoolParameter.h>
#include <piejam/gui/model/EnumParameter.h>
#include <piejam/gui/model/FloatParameter.h>
#include <piejam/gui/model/SpectrumGenerator.h>
#include <piejam/gui/model/StreamProcessor.h>
#include <piejam/renew.h>
#include <piejam/runtime/parameter/map.h>
#include <piejam/runtime/selectors.h>
#include <piejam/switch_cast.h>

#include <optional>

namespace piejam::fx_modules::spectrum::gui
{

using namespace piejam::gui::model;

namespace
{

struct SpectrumProcessor : StreamProcessor<SpectrumProcessor>
{
    using Base = StreamProcessor<SpectrumProcessor>;

    SpectrumProcessor(
        SpectrumSlot& spectrum,
        BoolParameter& active,
        EnumParameter& channel,
        FloatParameter& gain)
        : Base{active, channel, gain}
        , spectrum{spectrum}
    {
    }

    static constexpr audio::sample_rate default_sample_rate{48000u};

    template <class Samples>
    void process(Samples&& samples)
    {
        spectrum.update(
            spectrumGenerator.process(std::forward<Samples>(samples)));
    }

    void updateSampleRate(audio::sample_rate sr)
    {
        if (sr.valid() && sample_rate != sr)
        {
            renew(spectrumGenerator, sr);
            sample_rate = sr;
        }
    }

    SpectrumSlot& spectrum;
    audio::sample_rate sample_rate{default_sample_rate};
    SpectrumGenerator spectrumGenerator{sample_rate};
};

} // namespace

struct FxSpectrum::Impl
{
    std::
        pair<std::optional<SpectrumProcessor>, std::optional<SpectrumProcessor>>
            spectrumProcessor;

    void updateSampleRate(audio::sample_rate sr)
    {
        spectrumProcessor.first->updateSampleRate(sr);
        spectrumProcessor.second->updateSampleRate(sr);
    }
};

FxSpectrum::FxSpectrum(
    runtime::state_access const& state_access,
    runtime::fx::module_id const fx_mod_id)
    : FxModule{state_access, fx_mod_id}
    , m_impl{make_pimpl<Impl>()}
    , m_spectrumA{&addQObject<SpectrumSlot>()}
    , m_spectrumB{&addQObject<SpectrumSlot>()}
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
{
    m_impl->spectrumProcessor.first
        .emplace(*m_spectrumA, *m_activeA, *m_channelA, *m_gainA);
    m_impl->spectrumProcessor.second
        .emplace(*m_spectrumB, *m_activeB, *m_channelB, *m_gainB);

    auto& stream = addAttachedModel<AudioStreamProvider>(
        streams().at(std::to_underlying(stream_key::input)));

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
                m_impl->spectrumProcessor.first->processSamples(
                    captured.samples());
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

                auto captured_stereo = captured.channels_cast<2>();
                m_impl->spectrumProcessor.first->processStereo(captured_stereo);
                m_impl->spectrumProcessor.second->processStereo(
                    captured_stereo);
            });

        auto const clear_fn = [this]() { clear(); };

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
}

auto
FxSpectrum::type() const noexcept -> FxModuleType
{
    return {.id = internal_id()};
}

void
FxSpectrum::clear()
{
    m_spectrumA->clear();
    m_spectrumB->clear();
}

void
FxSpectrum::onSubscribe()
{
    m_impl->updateSampleRate(
        observe_once(runtime::selectors::select_sample_rate)->current);
}

} // namespace piejam::fx_modules::spectrum::gui
