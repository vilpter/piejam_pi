// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "get_set_hw_params.h"

#include "io_process_config.h"

#include <piejam/audio/pcm_format.h>
#include <piejam/audio/period_size.h>
#include <piejam/audio/sample_rate.h>
#include <piejam/audio/sound_card_descriptor.h>
#include <piejam/audio/sound_card_hw_params.h>
#include <piejam/system/device.h>

#include <spdlog/spdlog.h>

#include <boost/assert.hpp>

#include <sound/asound.h>
#include <sys/ioctl.h>

#include <algorithm>
#include <format>
#include <iterator>
#include <ranges>

namespace piejam::audio::alsa
{

namespace
{

auto
refine_hw_params(system::device& fd, snd_pcm_hw_params& hw_params) noexcept
    -> std::error_code
{
    hw_params.cmask = 0u;
    return fd.ioctl(SNDRV_PCM_IOCTL_HW_REFINE, hw_params);
}

constexpr auto
mask_offset(unsigned const val) noexcept -> unsigned
{
    return val >> 5u;
}

constexpr auto
mask_bit(unsigned const i) noexcept -> unsigned
{
    return 1u << (i & 31u);
}

constexpr auto
test_mask_bit(
    snd_pcm_hw_params const& params,
    unsigned const mask,
    unsigned const bit) noexcept -> bool
{
    return params.masks[mask - SNDRV_PCM_HW_PARAM_FIRST_MASK]
               .bits[mask_offset(bit)] &
           mask_bit(bit);
}

constexpr auto
test_mask_bit(snd_pcm_hw_params const& params, unsigned const mask) noexcept
{
    return [&params, mask](unsigned const bit) {
        return test_mask_bit(params, mask, bit);
    };
}

constexpr void
set_mask_bit(
    snd_pcm_hw_params& params,
    unsigned const mask,
    unsigned const bit) noexcept
{
    params.masks[mask - SNDRV_PCM_HW_PARAM_FIRST_MASK].bits[mask_offset(bit)] =
        mask_bit(bit);
    params.rmask |= (1u << mask);
}

constexpr auto
get_interval(
    snd_pcm_hw_params const& params,
    unsigned const ival_index) noexcept -> snd_interval const&
{
    return params.intervals[ival_index - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL];
}

constexpr void
set_interval_value(
    snd_pcm_hw_params& params,
    unsigned const ival_index,
    unsigned const value) noexcept
{
    snd_interval& ival =
        params.intervals[ival_index - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL];
    ival.min = value;
    ival.max = value;
    ival.openmin = 0;
    ival.openmax = 0;
    ival.integer = 1;
    params.rmask |= (1u << ival_index);
}

constexpr auto
test_interval_value(
    snd_pcm_hw_params params,
    unsigned const ival_index,
    unsigned const value) noexcept -> bool
{
    auto const& ival =
        params.intervals[ival_index - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL];
    return ival.min <= value && value <= ival.max;
}

constexpr auto
test_interval_value(
    snd_pcm_hw_params const& params,
    unsigned const ival_index) noexcept
{
    return [&params, ival_index](unsigned const value) -> bool {
        return test_interval_value(params, ival_index, value);
    };
}

constexpr auto
make_snd_pcm_hw_params_for_refine_any() noexcept -> snd_pcm_hw_params
{
    snd_pcm_hw_params hw_params{};

    hw_params.rmask = ~0u;
    std::ranges::for_each(hw_params.masks, [](snd_mask& m) {
        std::ranges::fill(m.bits, ~0u);
    });
    std::ranges::for_each(hw_params.intervals, [](snd_interval& i) {
        i.min = 0;
        i.max = ~0u;
    });

    return hw_params;
}

constexpr auto
alsa_to_pcm_format(unsigned alsa_format) noexcept -> pcm_format
{
    switch (alsa_format)
    {
        case SNDRV_PCM_FORMAT_S8:
            return pcm_format::s8;
        case SNDRV_PCM_FORMAT_U8:
            return pcm_format::u8;
        case SNDRV_PCM_FORMAT_S16_LE:
            return pcm_format::s16_le;
        case SNDRV_PCM_FORMAT_S16_BE:
            return pcm_format::s16_be;
        case SNDRV_PCM_FORMAT_U16_LE:
            return pcm_format::u16_le;
        case SNDRV_PCM_FORMAT_U16_BE:
            return pcm_format::u16_be;
        case SNDRV_PCM_FORMAT_S32_LE:
            return pcm_format::s32_le;
        case SNDRV_PCM_FORMAT_S32_BE:
            return pcm_format::s32_be;
        case SNDRV_PCM_FORMAT_U32_LE:
            return pcm_format::u32_le;
        case SNDRV_PCM_FORMAT_U32_BE:
            return pcm_format::u32_be;
        case SNDRV_PCM_FORMAT_S24_3LE:
            return pcm_format::s24_3le;
        case SNDRV_PCM_FORMAT_S24_3BE:
            return pcm_format::s24_3be;
        case SNDRV_PCM_FORMAT_U24_3LE:
            return pcm_format::u24_3le;
        case SNDRV_PCM_FORMAT_U24_3BE:
            return pcm_format::u24_3be;
        default:
            return pcm_format::unsupported;
    }
}

auto
set_access_mode(system::device& fd, snd_pcm_hw_params& hw_params) noexcept
    -> bool
{
    if (test_mask_bit(
            hw_params,
            SNDRV_PCM_HW_PARAM_ACCESS,
            SNDRV_PCM_ACCESS_RW_INTERLEAVED))
    {
        set_mask_bit(
            hw_params,
            SNDRV_PCM_HW_PARAM_ACCESS,
            SNDRV_PCM_ACCESS_RW_INTERLEAVED);

        if (!refine_hw_params(fd, hw_params))
        {
            return true;
        }
    }

    return false;
}

auto
set_pcm_format(system::device& fd, snd_pcm_hw_params& hw_params) noexcept
    -> pcm_format
{
    static constexpr std::array preferred_formats{
        SNDRV_PCM_FORMAT_S32_LE,
        SNDRV_PCM_FORMAT_U32_LE,
        SNDRV_PCM_FORMAT_S32_BE,
        SNDRV_PCM_FORMAT_U32_BE,
        SNDRV_PCM_FORMAT_S24_3LE,
        SNDRV_PCM_FORMAT_S24_3BE,
        SNDRV_PCM_FORMAT_U24_3LE,
        SNDRV_PCM_FORMAT_U24_3BE,
        SNDRV_PCM_FORMAT_S16_LE,
        SNDRV_PCM_FORMAT_U16_LE,
        SNDRV_PCM_FORMAT_S16_BE,
        SNDRV_PCM_FORMAT_U16_BE,
        SNDRV_PCM_FORMAT_S8,
        SNDRV_PCM_FORMAT_U8};

    auto const it_format = std::ranges::find_if(
        preferred_formats,
        test_mask_bit(hw_params, SNDRV_PCM_HW_PARAM_FORMAT));

    if (it_format == preferred_formats.end())
    {
        return pcm_format::unsupported;
    }

    set_mask_bit(hw_params, SNDRV_PCM_HW_PARAM_FORMAT, *it_format);

    if (refine_hw_params(fd, hw_params))
    {
        return pcm_format::unsupported;
    }

    return alsa_to_pcm_format(*it_format);
}

auto
set_num_channels(system::device& fd, snd_pcm_hw_params& hw_params) noexcept
    -> unsigned
{
    auto num_channels =
        get_interval(hw_params, SNDRV_PCM_HW_PARAM_CHANNELS).max;

    set_interval_value(hw_params, SNDRV_PCM_HW_PARAM_CHANNELS, num_channels);

    if (refine_hw_params(fd, hw_params))
    {
        return 0;
    }

    return num_channels;
}

auto
set_sample_rate(
    system::device& fd,
    snd_pcm_hw_params& hw_params,
    audio::sample_rate sr) noexcept -> std::error_code
{
    set_interval_value(hw_params, SNDRV_PCM_HW_PARAM_RATE, sr.value());
    return refine_hw_params(fd, hw_params);
}

auto
set_period_size(
    system::device& fd,
    snd_pcm_hw_params& hw_params,
    audio::period_size psz) noexcept -> std::error_code
{
    set_interval_value(hw_params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE, psz.value());
    return refine_hw_params(fd, hw_params);
}

auto
set_period_count(system::device& fd, snd_pcm_hw_params& hw_params) noexcept
    -> unsigned
{
    unsigned const max_period_count =
        get_interval(hw_params, SNDRV_PCM_HW_PARAM_PERIODS).max;
    auto availabe_period_counts = std::views::iota(2u, max_period_count + 1);
    auto ideal_period_count = std::ranges::find_if(
        availabe_period_counts,
        test_interval_value(hw_params, SNDRV_PCM_HW_PARAM_PERIODS));

    if (ideal_period_count == availabe_period_counts.end())
    {
        return 0u;
    }

    set_interval_value(
        hw_params,
        SNDRV_PCM_HW_PARAM_PERIODS,
        *ideal_period_count);
    if (refine_hw_params(fd, hw_params))
    {
        return 0u;
    }

    return *ideal_period_count;
}

} // namespace

auto
get_num_channels(std::filesystem::path const& pcm_device_path) -> unsigned
{
    system::device fd(pcm_device_path, system::device::blocking::off);

    auto hw_params = make_snd_pcm_hw_params_for_refine_any();
    if (auto err = fd.ioctl(SNDRV_PCM_IOCTL_HW_REFINE, hw_params))
    {
        throw std::system_error(err);
    }

    // TODO: do proper refinement?
    return get_interval(hw_params, SNDRV_PCM_HW_PARAM_CHANNELS).max;
}

auto
get_hw_params(
    std::filesystem::path const& pcm_device_path,
    sample_rate const sample_rate,
    period_size const period_size) -> sound_card_hw_params
{
    if (pcm_device_path.empty())
    {
        static sound_card_hw_params dummy_hw_params{
            .sample_rates =
                {preferred_sample_rates.begin(), preferred_sample_rates.end()},
            .period_sizes =
                {preferred_period_sizes.begin(), preferred_period_sizes.end()},
        };
        return dummy_hw_params;
    }

    system::device fd;

    try
    {
        fd = system::device(pcm_device_path, system::device::blocking::off);
    }
    catch (std::system_error const& err)
    {
        spdlog::error("failed to open sound card: {}", err.what());
        return {};
    }

    auto hw_params = make_snd_pcm_hw_params_for_refine_any();
    if (refine_hw_params(fd, hw_params) || !set_access_mode(fd, hw_params) ||
        set_pcm_format(fd, hw_params) == pcm_format::unsupported ||
        set_num_channels(fd, hw_params) == 0)
    {
        return {};
    }

    sound_card_hw_params result;

    BOOST_ASSERT(result.sample_rates.empty());
    std::ranges::copy_if(
        preferred_sample_rates,
        std::back_inserter(result.sample_rates),
        test_interval_value(hw_params, SNDRV_PCM_HW_PARAM_RATE),
        &audio::sample_rate::value);

    if (sample_rate.valid() &&
        std::ranges::contains(result.sample_rates, sample_rate))
    {
        if (set_sample_rate(fd, hw_params, sample_rate))
        {
            return {};
        }
    }

    BOOST_ASSERT(result.period_sizes.empty());
    std::ranges::copy_if(
        preferred_period_sizes,
        std::back_inserter(result.period_sizes),
        test_interval_value(hw_params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE),
        &audio::period_size::value);

    if (period_size.valid() &&
        std::ranges::contains(result.period_sizes, period_size))
    {
        if (set_period_size(fd, hw_params, period_size))
        {
            return {};
        }
    }

    return result;
}

auto
set_hw_params(system::device& fd, sound_card_config const& sc_config)
    -> set_hw_params_result
{
    set_hw_params_result result;

    auto hw_params = make_snd_pcm_hw_params_for_refine_any();

    if (auto err = refine_hw_params(fd, hw_params))
    {
        throw std::system_error(err);
    }

    if (!set_access_mode(fd, hw_params))
    {
        throw std::runtime_error("failed to configure sound card, access mode");
    }

    result.format = set_pcm_format(fd, hw_params);
    if (result.format == pcm_format::unsupported)
    {
        throw std::runtime_error("failed to configure sound card, pcm format");
    }

    result.num_channels = set_num_channels(fd, hw_params);
    if (result.num_channels == 0)
    {
        throw std::runtime_error(
            "failed to configure sound card, number of channels");
    }

    BOOST_ASSERT(sc_config.sample_rate.valid());
    if (set_sample_rate(fd, hw_params, sc_config.sample_rate))
    {
        throw std::runtime_error("failed to configure sound card, sample rate");
    }

    BOOST_ASSERT(sc_config.period_size.valid());
    if (set_period_size(fd, hw_params, sc_config.period_size))
    {
        throw std::runtime_error("failed to configure sound card, period size");
    }

    result.period_count = set_period_count(fd, hw_params);
    if (result.period_count == 0)
    {
        throw std::runtime_error(
            "failed to configure sound card, period count");
    }

    if (auto err = fd.ioctl(SNDRV_PCM_IOCTL_HW_PARAMS, hw_params))
    {
        throw std::runtime_error(
            std::format("failed to configure sound card: {}", err.message()));
    }

    return result;
}

} // namespace piejam::audio::alsa
