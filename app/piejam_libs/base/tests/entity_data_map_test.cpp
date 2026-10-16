// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/entity_data_map.h>

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

namespace piejam::test
{

namespace
{

using int_id_t = entity_id<int>;

} // namespace

TEST(entity_data_map, default_ctor)
{
    entity_data_map<int_id_t, int> sut;
    EXPECT_TRUE(sut.empty());
    EXPECT_EQ(0, sut.size());
}

TEST(entity_data_map, insert)
{
    entity_data_map<int_id_t, int> sut;
    auto id = int_id_t::generate();

    ASSERT_TRUE(sut.insert(id, 5));

    EXPECT_FALSE(sut.empty());
    EXPECT_EQ(1, sut.size());
    EXPECT_TRUE(sut.contains(id));

    auto found = sut.find(id);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(*found, 5);

    auto cached = sut.cached(id);
    ASSERT_NE(cached, nullptr);
    EXPECT_EQ(*cached, 5);
}

TEST(entity_data_map, insert_and_insert_same_again)
{
    entity_data_map<int_id_t, int> sut;
    auto id = int_id_t::generate();

    EXPECT_TRUE(sut.insert(id, 5));
    EXPECT_FALSE(sut.insert(id, 7));
}

TEST(entity_data_map, insert_remove)
{
    entity_data_map<int_id_t, int> sut;
    auto id = insert(sut, 5);

    EXPECT_FALSE(sut.empty());

    sut.erase(id);
    EXPECT_TRUE(sut.empty());
}

TEST(entity_data_map, emplace)
{
    entity_data_map<int_id_t, int> sut;
    auto id = int_id_t::generate();

    ASSERT_TRUE(sut.emplace(id, 5));

    EXPECT_FALSE(sut.empty());
    EXPECT_EQ(1, sut.size());
    EXPECT_TRUE(sut.contains(id));

    auto found = sut.find(id);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(*found, 5);

    auto cached = sut.cached(id);
    ASSERT_NE(cached, nullptr);
    EXPECT_EQ(*cached, 5);
}

TEST(entity_data_map, iterators)
{
    entity_data_map<int_id_t, int> sut;
    auto id = int_id_t::generate();

    ASSERT_TRUE(sut.emplace(id, 5));

    entity_data_map<int_id_t, int>::const_iterator it = sut.begin();

    auto&& [key, value] = *it;
    EXPECT_EQ(key, id);
    EXPECT_EQ(value, 5);

    static_assert(std::is_same_v<int const&, decltype(value)>);

    EXPECT_EQ(it->first, id);
    EXPECT_EQ(it->second, 5);
}

} // namespace piejam::test
