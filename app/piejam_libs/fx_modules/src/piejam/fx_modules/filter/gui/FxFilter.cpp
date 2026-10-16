// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "FxFilter.h"

#include "../filter_internal_id.h"
#include "../filter_module.h"

#include <piejam/gui/model/AudioStreamProvider.h>
#include <piejam/gui/model/EnumParameter.h>
#include <piejam/gui/model/FloatParameter.h>
#include <piejam/gui/model/SpectrumGenerator.h>
#include <piejam/gui/model/SpectrumSlot.h>
#include <piejam/renew.h>
#include <piejam/runtime/parameter/map.h>
#include <piejam/runtime/selectors.h>

namespace piejam::fx_modules::filter::gui
{

using namespace piejam::gui::model;

struct FxFilter::Impl
{
    static constexpr audio::sample_rate default_sample_rate{48000u};
    audio::sample_rate sample_rate{default_sample_rate};

    SpectrumGenerator spectrumInGenerator{sample_rate};
    SpectrumGenerator spectrumOutGenerator{sample_rate};

    void updateSampleRate(audio::sample_rate sr)
    {
        if (sr.valid() && sample_rate != sr)
        {
            sample_rate = sr;
            renew(spectrumInGenerator, sr);
            renew(spectrumOutGenerator, sr);
        }
    }
};

FxFilter::FxFilter(
    runtime::state_access const& state_access,
    runtime::fx::module_id const fx_mod_id)
    : FxModule{state_access, fx_mod_id}
    , m_impl{make_pimpl<Impl>()}
    , m_spectrumIn{&addQObject<SpectrumSlot>()}
    , m_spectrumOut{&addQObject<SpectrumSlot>()}
    , m_filterType{&addModel<EnumParameter>(
          parameters().get<runtime::enum_parameter_id>(parameter_key::type))}
    , m_cutoff{&addModel<FloatParameter>(
          parameters().get<runtime::float_parameter_id>(parameter_key::cutoff))}
    , m_resonance{&addModel<FloatParameter>(
          parameters().get<runtime::float_parameter_id>(
              parameter_key::resonance))}
{
    auto& inOutStream = addAttachedModel<AudioStreamProvider>(
        streams().at(std::to_underlying(stream_key::in_out)));

    if (busType() == BusType::Mono)
    {
        QObject::connect(
            &inOutStream,
            &AudioStreamProvider::captured,
            this,
            [this](AudioStream captured) {
                BOOST_ASSERT(captured.num_channels() == 2);

                m_spectrumIn->update(m_impl->spectrumInGenerator.process(
                    captured.channels_cast<2>().channels()[0]));
                m_spectrumOut->update(m_impl->spectrumOutGenerator.process(
                    captured.channels_cast<2>().channels()[1]));
            });
    }
    else
    {
        QObject::connect(
            &inOutStream,
            &AudioStreamProvider::captured,
            this,
            [this](AudioStream captured) {
                BOOST_ASSERT(captured.num_channels() == 4);

                m_spectrumIn->update(
                    m_impl->spectrumInGenerator.process(toMiddle(
                        captured.channels_subview(0, 2).channels_cast<2>())));
                m_spectrumOut->update(
                    m_impl->spectrumOutGenerator.process(toMiddle(
                        captured.channels_subview(2, 2).channels_cast<2>())));
            });
    }
}

void
FxFilter::onSubscribe()
{
    m_impl->updateSampleRate(
        observe_once(runtime::selectors::select_sample_rate)->current);
}

auto
FxFilter::type() const noexcept -> FxModuleType
{
    return {.id = internal_id()};
}

void
FxFilter::clear()
{
    m_spectrumIn->clear();
    m_spectrumOut->clear();
}

} // namespace piejam::fx_modules::filter::gui
