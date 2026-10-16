// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <boost/assert.hpp>
#include <boost/outcome/std_result.hpp>

#include <chrono>
#include <filesystem>
#include <span>
#include <system_error>

namespace outcome = boost::outcome_v2;

namespace piejam::system
{

class device
{
public:
    enum class blocking : bool
    {
        off,
        on,
    };

    device() noexcept = default;
    device(std::filesystem::path const& pathname, blocking = blocking::on);
    device(device const&) = delete;
    device(device&& other) noexcept;

    ~device();

    auto operator=(device const&) -> device& = delete;
    auto operator=(device&& other) noexcept -> device&;

    explicit operator bool() const noexcept
    {
        return m_fd != invalid;
    }

    [[nodiscard]]
    auto ioctl(unsigned long request) noexcept -> std::error_code;

    template <class T>
    [[nodiscard]]
    auto ioctl(unsigned long request, T& x) noexcept -> std::error_code
    {
        return ioctl(request, &x, sizeof(T));
    }

    [[nodiscard]]
    auto read(std::span<std::byte> buffer) noexcept
        -> outcome::std_result<std::size_t>;

    template <class T, std::size_t Extent>
    [[nodiscard]]
    auto read(std::span<T, Extent> buffer) noexcept
        -> outcome::std_result<std::span<T>>
    {
        auto res = read(
            std::span<std::byte>{
                reinterpret_cast<std::byte*>(buffer.data()),
                buffer.size_bytes()});
        if (!res)
        {
            return res.error();
        }

        // Number of complete T elements that fit into *res bytes
        std::size_t n = res.value() / sizeof(T);
        BOOST_ASSERT(res.value() % sizeof(T) == 0);

        return buffer.first(n);
    }

    template <class T>
        requires std::is_trivially_copyable_v<T>
    [[nodiscard]]
    auto read(T& x) noexcept -> outcome::std_result<void>
    {
        return read_fully(
            std::span<std::byte>{reinterpret_cast<std::byte*>(&x), sizeof(T)});
    }

    [[nodiscard]]
    auto set_nonblock(bool set = true) -> std::error_code;

    //! Polls the device for input. Returns true if input is available, false if
    //! timed out.
    [[nodiscard]]
    auto poll(std::chrono::milliseconds timeout) noexcept
        -> outcome::std_result<bool>;

private:
    [[nodiscard]]
    auto ioctl(unsigned long request, void* p, std::size_t size) noexcept
        -> std::error_code;

    auto read_fully(std::span<std::byte> buffer) noexcept
        -> outcome::std_result<void>;

    static constexpr int invalid = -1;

    int m_fd{invalid};
};

template <>
auto device::ioctl(unsigned long request, device&) noexcept
    -> std::error_code = delete;

template <>
[[nodiscard]]
auto device::ioctl(unsigned long request, device const& other) noexcept
    -> std::error_code;

} // namespace piejam::system
