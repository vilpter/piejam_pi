// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/SpectrumGenerator.h>

#include <piejam/algorithm/transform_to_vector.h>
#include <piejam/audio/sample_rate.h>
#include <piejam/numeric/dB_convert.h>
#include <piejam/numeric/dft.h>
#include <piejam/numeric/generators/cosine_window.h>
#include <piejam/range/iota.h>

#include <boost/assert.hpp>

#include <functional>
#include <vector>

namespace piejam::gui::model
{

namespace
{

auto
dftForResolution(DFTResolution const resolution) -> numeric::dft&
{
    switch (resolution)
    {
        case DFTResolution::Medium:
        {
            static numeric::dft s_dft{4096};
            return s_dft;
        }

        case DFTResolution::High:
        {
            static numeric::dft s_dft{8192};
            return s_dft;
        }

        case DFTResolution::VeryHigh:
        {
            static numeric::dft s_dft{16384};
            return s_dft;
        }

        case DFTResolution::Low:
        default:
        {
            static numeric::dft s_dft{2048};
            return s_dft;
        }
    }
}

} // namespace

struct SpectrumGenerator::Impl
{
    explicit Impl(audio::sample_rate sample_rate, DFTResolution dftResolution)
        : m_dft{dftForResolution(dftResolution)}
        , m_window(m_dft.size())
        , m_dataPoints(m_dft.output_size())
    {
        std::ranges::generate(
            m_window,
            numeric::generators::hann<>{m_dft.size()});

        float const binSize =
            sample_rate.as<float>() / static_cast<float>(m_dft.size());
        for (std::size_t const i : range::iota(m_dft.output_size()))
        {
            m_dataPoints[i].frequency_Hz = static_cast<float>(i) * binSize;
        }
    }

    static constexpr auto envelope(float const prev, float const in) noexcept
        -> float
    {
        BOOST_ASSERT(in >= 0.f);
        return in > prev ? in : in + 0.85f * (prev - in);
    }

    auto process(std::span<float const> samples) -> SpectrumDataPoints
    {
        BOOST_ASSERT(samples.size() == m_window.size());
        BOOST_ASSERT(samples.size() == m_dft.input_buffer().size());

        std::transform(
            samples.begin(),
            samples.end(),
            m_window.begin(),
            m_dft.input_buffer().begin(),
            std::multiplies<>{});

        auto const spectrum = m_dft.process();

        BOOST_ASSERT(spectrum.size() == m_dft.output_size());
        BOOST_ASSERT(m_dataPoints.size() == m_dft.output_size());

        auto const dft_size = static_cast<float>(m_dft.size());
        auto const two_div_dft_size = 2.f / dft_size;

        constexpr auto min_level = 1.e-20f;

        m_dataPoints[0].level =
            envelope(m_dataPoints[0].level, std::abs(spectrum[0]) / dft_size);
        m_dataPoints[0].level_dB =
            numeric::to_dB(m_dataPoints[0].level, min_level);

        for (std::size_t i = 1, e = m_dft.output_size(); i < e; ++i)
        {
            m_dataPoints[i].level = envelope(
                m_dataPoints[i].level,
                std::abs(spectrum[i]) * two_div_dft_size);
            m_dataPoints[i].level_dB =
                numeric::to_dB(m_dataPoints[i].level, min_level);
        }

        return m_dataPoints;
    }

    numeric::dft& m_dft;
    std::vector<float> m_window;
    std::vector<SpectrumDataPoint> m_dataPoints;
};

SpectrumGenerator::SpectrumGenerator(
    audio::sample_rate sample_rate,
    DFTResolution dftResolution)
    : m_impl{make_pimpl<Impl>(sample_rate, dftResolution)}
    , m_dftPrepareBuffer(m_impl->m_dft.size())
{
}

auto
SpectrumGenerator::process() -> SpectrumDataPoints
{
    return m_impl->process(m_dftPrepareBuffer);
}

} // namespace piejam::gui::model
