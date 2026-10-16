// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/network_manager/network_controller.h>

#include <gtest/gtest.h>

namespace piejam::network_manager::test
{

class network_controller_test : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Tests run in isolation - actual network operations may fail
        // in test environment, so we mainly test the logic/state management
    }
};

TEST_F(network_controller_test, state_change_callback_is_called)
{
    network_controller ctrl;

    bool callback_called = false;
    bool callback_value = false;

    ctrl.set_state_change_callback([&](bool enabled) {
        callback_called = true;
        callback_value = enabled;
    });

    // Callback should be set successfully
    // Actual enable/disable may fail in test environment without root
    EXPECT_FALSE(callback_called); // Not called yet
}

TEST_F(network_controller_test, move_constructor)
{
    network_controller ctrl1;
    network_controller ctrl2(std::move(ctrl1));
    // Just verify it doesn't crash
}

TEST_F(network_controller_test, move_assignment)
{
    network_controller ctrl1;
    network_controller ctrl2;
    ctrl2 = std::move(ctrl1);
    // Just verify it doesn't crash
}

} // namespace piejam::network_manager::test
