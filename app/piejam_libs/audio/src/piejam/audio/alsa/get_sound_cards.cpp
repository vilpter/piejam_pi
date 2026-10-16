// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "get_sound_cards.h"

#include "get_set_hw_params.h"

#include <piejam/box.h>
#include <piejam/io_pair.h>
#include <piejam/system/device.h>

#include <spdlog/spdlog.h>

#include <sound/asound.h>
#include <sys/ioctl.h>

#include <boost/assert.hpp>

#include <filesystem>
#include <format>
#include <fstream>

namespace piejam::audio::alsa
{

namespace
{

struct sound_card_info
{
    std::filesystem::path control_path;
    snd_ctl_card_info info;
};

std::filesystem::path const s_sound_cards_dir("/dev/snd");

auto
scan_for_sound_cards() -> std::vector<sound_card_info>
{
    std::vector<sound_card_info> cards;

    std::error_code ec;
    if (std::filesystem::exists(s_sound_cards_dir, ec))
    {
        for (auto const& entry :
             std::filesystem::directory_iterator(s_sound_cards_dir, ec))
        {
            unsigned card{};
            if (1 == std::sscanf(
                         entry.path().filename().c_str(),
                         "controlC%u",
                         &card))
            {
                system::device fd(entry.path());
                snd_ctl_card_info card_info{};

                if (auto err = fd.ioctl(SNDRV_CTL_IOCTL_CARD_INFO, card_info))
                {
                    auto const message = err.message();
                    spdlog::error("scan_for_sound_cards: {}", message);
                }
                else
                {
                    cards.emplace_back(
                        sound_card_info{
                            .control_path = entry.path(),
                            .info = card_info});
                }
            }
        }
    }

    return cards;
}

auto
get_num_channels_from_procfs(int card, unsigned int device, char stream_type)
    -> unsigned
{
    auto hw_params_path = std::filesystem::path{std::format(
        "/proc/asound/card{}/pcm{}{}/sub0/hw_params",
        card,
        device,
        stream_type)};
    if (!std::filesystem::exists(hw_params_path))
    {
        spdlog::error(
            "get_num_channels_from_procfs: {} does not exist",
            hw_params_path.string());
        return 0u;
    }

    std::ifstream file(hw_params_path);
    if (!file.is_open())
    {
        spdlog::error(
            "get_num_channels_from_procfs: cannot open {}",
            hw_params_path.string());
        return 0u;
    }

    std::string line;
    unsigned channels = 0;

    while (std::getline(file, line))
    {
        if (std::sscanf(line.c_str(), "channels: %u", &channels) == 1)
        {
            return channels;
        }
    }

    spdlog::error("get_num_channels_from_procfs: could not retrieve channels");
    return 0u;
}

auto
make_sound_card_stream_descriptor(
    int card,
    unsigned int device,
    char stream_type) -> std::pair<std::filesystem::path, unsigned>
{
    auto device_path = std::filesystem::path{
        std::format("/dev/snd/pcmC{}D{}{}", card, device, stream_type)};

    BOOST_ASSERT(std::filesystem::exists(device_path));

    unsigned num_channels{};

    try
    {
        num_channels = get_num_channels(device_path);
    }
    catch (std::system_error const& e)
    {
        if (e.code() !=
            std::make_error_code(std::errc::device_or_resource_busy))
        {
            spdlog::error("make_sound_card_stream_descriptor: {}", e.what());
        }
        else
        {
            num_channels =
                get_num_channels_from_procfs(card, device, stream_type);
        }
    }

    return {device_path, num_channels};
}

auto
get_sound_card_descriptors(sound_card_info const& sc)
    -> std::vector<sound_card_descriptor>
{
    std::vector<sound_card_descriptor> result;

    system::device fd(sc.control_path);

    int device{-1};
    do
    {
        if (auto err = fd.ioctl(SNDRV_CTL_IOCTL_PCM_NEXT_DEVICE, device))
        {
            auto const message = err.message();
            spdlog::error("get_sound_card_pcm_infos: {}", message);
            break;
        }

        if (device == -1)
        {
            break;
        }

        sound_card_descriptor desc{};

        auto get_stream =
            [&](int stream_type) -> std::pair<std::filesystem::path, unsigned> {
            snd_pcm_info info{};
            info.card = sc.info.card;
            info.device = static_cast<unsigned>(device);
            info.subdevice = 0;
            info.stream = stream_type;

            if (auto err = fd.ioctl(SNDRV_CTL_IOCTL_PCM_INFO, info))
            {
                static auto const error_to_ignore =
                    std::make_error_code(std::errc::no_such_file_or_directory);
                if (err != error_to_ignore)
                {
                    auto const message = err.message();
                    spdlog::error("get_sound_card_pcm_infos: {}", message);
                }

                return {};
            }

            desc.name = std::format(
                "{} - {}",
                reinterpret_cast<char const*>(sc.info.name),
                reinterpret_cast<char const*>(info.name));

            return make_sound_card_stream_descriptor(
                sc.info.card,
                info.device,
                stream_type == SNDRV_PCM_STREAM_CAPTURE ? 'c' : 'p');
        };

        std::filesystem::path in_path;
        std::filesystem::path out_path;

        std::tie(in_path, desc.num_channels.in()) =
            get_stream(SNDRV_PCM_STREAM_CAPTURE);
        std::tie(out_path, desc.num_channels.out()) =
            get_stream(SNDRV_PCM_STREAM_PLAYBACK);
        desc.impl_data =
            stream_descriptors{std::move(in_path), std::move(out_path)};
        result.emplace_back(std::move(desc));

    } while (device != -1);

    return result;
}

} // namespace

auto
get_sound_cards() -> sound_cards
{
    std::vector<sound_card_descriptor> result;

    for (sound_card_info const& sc_info : scan_for_sound_cards())
    {
        // TODO: use result.append_range when available (C++23)
        auto scs = get_sound_card_descriptors(sc_info);
        result.insert(
            result.end(),
            std::make_move_iterator(scs.begin()),
            std::make_move_iterator(scs.end()));
    }

    return result;
}

} // namespace piejam::audio::alsa
