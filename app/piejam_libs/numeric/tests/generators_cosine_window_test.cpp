// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/numeric/generators/cosine_window.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>

namespace piejam::numeric::generators::test
{

TEST(hamming, generates_known_values_float)
{
    constexpr std::size_t size = 4;
    hamming<float> gen(size);

    // Expected values computed once from formula:
    std::array<float, size> expected = {0.08f, 0.77f, 0.77f, 0.08f};

    for (std::size_t i = 0; i < size; ++i)
    {
        float actual = gen();
        EXPECT_NEAR(expected[i], actual, 1e-6f) << " at index " << i;
    }
}

TEST(hamming, generates_known_values_double)
{
    constexpr std::size_t size = 8;
    hamming<double> gen(size);

    std::array<double, size> expected = {
        0.08000000000000007,
        0.25319469114498255,
        0.6423596296199047,
        0.9544456792351129,
        0.9544456792351129,
        0.6423596296199048,
        0.25319469114498266,
        0.08000000000000007};

    for (std::size_t i = 0; i < size; ++i)
    {
        double actual = gen();
        EXPECT_NEAR(expected[i], actual, 1e-12) << " at index " << i;
    }
}

TEST(hamming, is_symmetric)
{
    constexpr std::size_t size = 128;
    std::array<float, size> values{};

    std::ranges::generate(values, hamming<float>(size));

    for (std::size_t i = 0; i < size / 2; ++i)
    {
        EXPECT_NEAR(values[i], values[size - 1 - i], 1e-5f)
            << " symmetry check failed at index " << i;
    }
}

TEST(hann_generator, generates_known_values_float)
{
    constexpr std::size_t N = 8;
    hann<float> gen(N);

    float expected[N] = {
        0.0f,
        0.188255f,
        0.611260f,
        0.950484f,
        0.950484f,
        0.611260f,
        0.188255f,
        0.0f};

    for (std::size_t i = 0; i < N; ++i)
    {
        EXPECT_NEAR(gen(), expected[i], 1e-6f);
    }
}

TEST(hann_generator, generates_known_values_double)
{
    constexpr std::size_t N = 8;
    hann<double> gen(N);

    double expected[N] = {
        0.0,
        0.188255102,
        0.611260466,
        0.950484433,
        0.950484433,
        0.611260466,
        0.188255102,
        0.0};

    for (std::size_t i = 0; i < N; ++i)
    {
        EXPECT_NEAR(gen(), expected[i], 1e-8);
    }
}

TEST(hann_generator, is_symmetric)
{
    constexpr std::size_t N = 128;
    hann<float> gen(N);

    std::vector<float> values;
    for (std::size_t i = 0; i < N; ++i)
    {
        values.push_back(gen());
    }

    for (std::size_t i = 0; i < N / 2; ++i)
    {
        EXPECT_NEAR(values[i], values[N - 1 - i], 1e-5f);
    }
}

} // namespace piejam::numeric::generators::test
