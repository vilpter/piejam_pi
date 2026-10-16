// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "io_process_config.h"

#include <piejam/audio/fwd.h>
#include <piejam/audio/io_process.h>

#include <piejam/system/device.h>

#include <atomic>
#include <filesystem>
#include <memory>

namespace piejam::audio::alsa
{

class pcm_io final : public piejam::audio::io_process
{
public:
    pcm_io() noexcept;
    pcm_io(
        std::filesystem::path const& in,
        std::filesystem::path const& out,
        sound_card_config const& sc_config);
    ~pcm_io() override;

    [[nodiscard]]
    auto is_open() const noexcept -> bool override;
    void close() override;

    [[nodiscard]]
    auto is_running() const noexcept -> bool override;

    void start(
        thread::configuration const&,
        init_process_function const&,
        process_function) override;

    void stop() override;

    [[nodiscard]]
    auto cpu_load() const noexcept -> float override
    {
        return m_cpu_load.load(std::memory_order_relaxed);
    }

    [[nodiscard]]
    auto xruns() const noexcept -> std::size_t override
    {
        return m_xruns.load(std::memory_order_relaxed);
    }

private:
    system::device m_input_fd;
    system::device m_output_fd;
    io_process_config m_io_config;

    std::atomic<float> m_cpu_load{};
    std::atomic_size_t m_xruns{};

    std::unique_ptr<process_thread> m_process_thread;
};

} // namespace piejam::audio::alsa
