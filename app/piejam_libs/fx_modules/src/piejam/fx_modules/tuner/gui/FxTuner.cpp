// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "FxTuner.h"

#include "../tuner_internal_id.h"
#include "../tuner_module.h"

#include <piejam/audio/pitch.h>
#include <piejam/gui/model/AudioStreamProvider.h>
#include <piejam/gui/model/PitchGenerator.h>
#include <piejam/renew.h>
#include <piejam/runtime/selectors.h>

#include <boost/container/flat_map.hpp>

#include <format>

namespace piejam::fx_modules::tuner::gui
{

using namespace piejam::gui::model;

struct FxTuner::Impl
{
    static constexpr audio::sample_rate default_sample_rate{48000u};
    audio::sample_rate sample_rate{default_sample_rate};

    PitchGenerator pitchGenerator{sample_rate};

    void updateSampleRate(audio::sample_rate sr)
    {
        if (sr.valid() && sample_rate != sr)
        {
            sample_rate = sr;
            renew(pitchGenerator, sr);
        }
    }
};

FxTuner::FxTuner(
    runtime::state_access const& state_access,
    runtime::fx::module_id const fx_mod_id)
    : FxModule{state_access, fx_mod_id}
    , m_impl{make_pimpl<Impl>()}
{
    auto& stream = addAttachedModel<AudioStreamProvider>(
        streams().at(std::to_underlying(stream_key::input)));

    QObject::connect(
        &stream,
        &AudioStreamProvider::captured,
        this,
        [this](AudioStream captured) {
            float const detectedFrequency =
                busType() == BusType::Mono
                    ? m_impl->pitchGenerator.process(captured.samples())
                    : m_impl->pitchGenerator.process(
                          toMiddle(captured.channels_cast<2>()));

            if (detectedFrequency != m_detectedFrequency)
            {
                setDetectedFrequency(detectedFrequency);

                if (detectedFrequency > 0.f)
                {
                    auto pitch =
                        audio::pitch::from_frequency(detectedFrequency);

                    auto pc = [](audio::pitchclass pc) {
                        switch (pc)
                        {
                            case audio::pitchclass::A:
                                return "A";
                            case audio::pitchclass::A_sharp:
                                return "A#";
                            case audio::pitchclass::B:
                                return "B";
                            case audio::pitchclass::C:
                                return "C";
                            case audio::pitchclass::C_sharp:
                                return "C#";
                            case audio::pitchclass::D:
                                return "D";
                            case audio::pitchclass::D_sharp:
                                return "D#";
                            case audio::pitchclass::E:
                                return "E";
                            case audio::pitchclass::F:
                                return "F";
                            case audio::pitchclass::F_sharp:
                                return "F#";
                            case audio::pitchclass::G:
                                return "G";
                            case audio::pitchclass::G_sharp:
                                return "G#";
                        }

                        return "--";
                    }(pitch.pitchclass_);

                    setDetectedPitch(
                        QString::fromStdString(
                            std::format("{}{}", pc, pitch.octave)));

                    setDetectedCents(static_cast<int>(std::round(pitch.cents)));
                }
                else
                {
                    static QString s_empty{"--"};
                    setDetectedPitch(s_empty);
                    setDetectedCents(0);
                }
            }
        });
}

auto
FxTuner::type() const noexcept -> FxModuleType
{
    return {.id = internal_id()};
}

void
FxTuner::onSubscribe()
{
    m_impl->updateSampleRate(
        observe_once(runtime::selectors::select_sample_rate)->current);
}

} // namespace piejam::fx_modules::tuner::gui
