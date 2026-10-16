// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/Log.h>

#include <piejam/redux/store.h>
#include <piejam/redux/subscriber.h>
#include <piejam/runtime/state.h>
#include <piejam/runtime/state_access.h>
#include <piejam/runtime/store.h>
#include <piejam/runtime/subscriber.h>

#include <spdlog/spdlog.h>

#include <QAbstractListModel>

#include <gtest/gtest.h>

namespace piejam::gui::model::test
{

TEST(Log, info_page_lists_info_and_above_only)
{
    runtime::store store(
        [](runtime::state&, runtime::action const&) {},
        runtime::make_initial_state());
    runtime::subscriber subscriber(
        [&store]() -> runtime::state const& { return store.state(); });

    auto const saved_sinks = spdlog::default_logger()->sinks();
    auto const saved_level = spdlog::default_logger()->level();
    spdlog::set_level(spdlog::level::debug); // as the app does

    {
        Log log{runtime::state_access{store, subscriber}};
        auto const& messages = *log.logMessages();

        spdlog::debug("scene graph details");
        spdlog::info("Connected to WiFi network: Home");
        spdlog::warn("could not lock memory");
        spdlog::error("device open failed");

        EXPECT_EQ(messages.rowCount(), 3);
        EXPECT_EQ(
            messages.index(0).data().toString().toStdString(),
            "[info] Connected to WiFi network: Home");

        // The sink refers to the model; remove it before the model goes.
        spdlog::default_logger()->sinks() = saved_sinks;
    }

    spdlog::set_level(saved_level);
}

} // namespace piejam::gui::model::test
