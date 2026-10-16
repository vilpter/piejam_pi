// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/numeric/simd/lrot_n.h>

#include <gtest/gtest.h>

namespace piejam::numeric::test
{

namespace
{

template <typename T>
static std::vector<T>
to_vector(mipp::Reg<T> reg)
{
    alignas(mipp::N<T>() * sizeof(T)) T buf[mipp::N<T>()];
    reg.store(buf);
    return std::vector<T>(buf, buf + mipp::N<T>());
}

template <typename T>
static mipp::Reg<T>
from_vector(std::vector<T> const& v)
{
    EXPECT_EQ(v.size(), mipp::N<T>());
    return mipp::Reg<T>(v.data());
}

template <typename T>
static std::vector<T>
expected_rotation(std::vector<T> const& v, std::size_t n)
{
    std::size_t N = v.size();
    std::vector<T> result(N);
    for (std::size_t i = 0; i < N; ++i)
    {
        result[i] = v[(i + n) % N];
    }
    return result;
}

using test_types = ::testing::Types<
    float,
    double,
    std::int32_t,
    std::int64_t,
    std::uint32_t,
    std::uint64_t>;

} // namespace

template <typename T>
class lrot_n_test : public ::testing::Test
{
};

TYPED_TEST_SUITE(lrot_n_test, test_types);

TYPED_TEST(lrot_n_test, rotate_by_zero)
{
    using T = TypeParam;
    constexpr std::size_t N = mipp::N<T>();

    std::vector<T> input(N);
    for (std::size_t i = 0; i < N; ++i)
    {
        input[i] = static_cast<T>(i);
    }

    auto reg = from_vector(input);
    auto rotated = to_vector(simd::lrot_n(reg, 0));

    EXPECT_EQ(rotated, input);
}

TYPED_TEST(lrot_n_test, rotate_by_one)
{
    using T = TypeParam;
    constexpr std::size_t N = mipp::N<T>();

    std::vector<T> input(N);
    for (std::size_t i = 0; i < N; ++i)
    {
        input[i] = static_cast<T>(i);
    }

    auto reg = from_vector(input);
    auto rotated = to_vector(simd::lrot_n(reg, 1));

    EXPECT_EQ(rotated, expected_rotation(input, 1));
}

TYPED_TEST(lrot_n_test, rotate_full_cycle)
{
    using T = TypeParam;
    constexpr std::size_t N = mipp::N<T>();

    std::vector<T> input(N);
    for (std::size_t i = 0; i < N; ++i)
    {
        input[i] = static_cast<T>(i * 2);
    }

    auto reg = from_vector(input);
    auto rotated = to_vector(simd::lrot_n(reg, N));

    EXPECT_EQ(rotated, input);
}

TYPED_TEST(lrot_n_test, rotate_random_offsets)
{
    using T = TypeParam;
    constexpr std::size_t N = mipp::N<T>();

    std::vector<T> input(N);
    for (std::size_t i = 0; i < N; ++i)
    {
        input[i] = static_cast<T>(i * 10);
    }

    auto reg = from_vector(input);

    for (std::size_t n :
         {std::size_t{2}, std::size_t{3}, std::size_t{5}, N - 1})
    {
        auto rotated = to_vector(simd::lrot_n(reg, n));
        EXPECT_EQ(rotated, expected_rotation(input, n));
    }
}

} // namespace piejam::numeric::test
