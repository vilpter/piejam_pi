// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/model/AudioStream.h>
#include <piejam/gui/model/BoolParameter.h>
#include <piejam/gui/model/EnumParameter.h>
#include <piejam/gui/model/FloatParameter.h>
#include <piejam/gui/model/Types.h>

#include <piejam/functional/operators.h>
#include <piejam/switch_cast.h>

#include <boost/range/iterator_range_core.hpp>

namespace piejam::gui::model
{

template <class Derived>
struct StreamProcessor
{
    StreamProcessor(
        BoolParameter& active,
        EnumParameter& channel,
        FloatParameter& gain)
        : active{active}
        , channel{channel}
        , gain{gain}
    {
    }

    template <class Samples>
    auto processSamples(Samples&& samples)
    {
        auto const g = static_cast<float>(gain.value());
        switch (switch_cast(g))
        {
            case switch_cast(1.f):
                static_cast<Derived&>(*this).process(
                    std::forward<Samples>(samples));
                break;

            default:
                static_cast<Derived&>(*this).process(
                    boost::make_iterator_range(
                        std::begin(samples),
                        std::end(samples)) |
                    std::views::transform(multiplies(g)));
                break;
        }
    }

    void processStereo(StereoAudioStream stream)
    {
        if (!active.value())
        {
            return;
        }

        switch (static_cast<StereoChannel>(channel.value()))
        {
            case StereoChannel::Left:
                processSamples(toLeft(stream));
                break;

            case StereoChannel::Right:
                processSamples(toRight(stream));
                break;

            case StereoChannel::Middle:
                processSamples(toMiddle(stream));
                break;

            case StereoChannel::Side:
                processSamples(toSide(stream));
                break;
        }
    }

    BoolParameter& active;
    EnumParameter& channel;
    FloatParameter& gain;
};

} // namespace piejam::gui::model
