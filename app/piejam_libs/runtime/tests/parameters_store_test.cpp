// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/parameter/store.h>

#include <piejam/runtime/bool_parameter.h>

#include <piejam/boxed_string.h>
#include <piejam/entity_id.h>

#include <gtest/gtest.h>

namespace piejam::runtime::parameter::test
{

TEST(store, add_default_get)
{
    using namespace std::string_literals;

    store sut;

    auto id = id_t<bool_parameter>::generate();
    sut.emplace(id, make_bool_parameter({"foo", true}));

    EXPECT_EQ(true, sut.at(id).get());
}

TEST(store, add_remove)
{
    store sut;

    auto id = id_t<bool_parameter>::generate();
    sut.emplace(id, make_bool_parameter({"foo"}));
    ASSERT_TRUE(sut.contains(id));
    sut.remove(id);

    EXPECT_FALSE(sut.contains(id));
}

TEST(store, get_existing_parameter)
{
    store sut;

    auto id = id_t<bool_parameter>::generate();
    sut.emplace(id, make_bool_parameter({"foo"}));
    EXPECT_NE(nullptr, sut.find(id));
}

TEST(store, find_for_non_existing_parameter)
{
    store sut;

    EXPECT_EQ(nullptr, sut.find(parameter::id_t<bool_parameter>::generate()));
}

TEST(store, get_cached_for_existing_parameter)
{
    using namespace std::string_literals;

    store sut;

    auto id = id_t<bool_parameter>::generate();
    sut.emplace(id, make_bool_parameter({"foo", true}));
    auto desc = sut.find(id);
    ASSERT_NE(nullptr, desc);
    EXPECT_EQ(true, *desc->cached());
}

TEST(store, set_value)
{
    store sut;

    auto id = id_t<bool_parameter>::generate();
    sut.emplace(id, make_bool_parameter({"foo"}));

    sut.at(id).set(true);
    EXPECT_EQ(true, sut.at(id).get());
}

TEST(store, cached_is_updated_after_set)
{
    store sut;

    auto id = id_t<bool_parameter>::generate();
    sut.emplace(id, make_bool_parameter({"foo"}));

    auto cached = sut.at(id).cached();
    ASSERT_NE(nullptr, cached);
    EXPECT_EQ(false, *cached);

    sut.at(id).set(true);

    EXPECT_EQ(true, *cached);
}

TEST(store, equality)
{
    store m1;

    auto id = id_t<bool_parameter>::generate();
    m1.emplace(id, make_bool_parameter({"foo"}));

    EXPECT_EQ(m1, m1);

    store m2;

    m2.emplace(id, make_bool_parameter({"foo"}));

    EXPECT_NE(m1, m2);

    auto m3 = m1;

    EXPECT_EQ(m1, m3);
}

} // namespace piejam::runtime::parameter::test
