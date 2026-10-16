// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/SubscribableModel.h>
#include <piejam/gui/model/fwd.h>
#include <piejam/network_manager/fwd.h>

#include <filesystem>
#include <memory>

namespace piejam::gui::model
{

class Root final : public CompositeSubscribableModel
{
    Q_OBJECT

    PIEJAM_GUI_PROPERTY(int, displayRotation, setDisplayRotation)

    PIEJAM_GUI_CONSTANT_PROPERTY(
        piejam::gui::model::AudioDeviceSettings*,
        audioDeviceSettings)

    PIEJAM_GUI_CONSTANT_PROPERTY(
        piejam::gui::model::AudioInputOutputSettings*,
        audioInputSettings)

    PIEJAM_GUI_CONSTANT_PROPERTY(
        piejam::gui::model::AudioInputOutputSettings*,
        audioOutputSettings)

    PIEJAM_GUI_CONSTANT_PROPERTY(
        piejam::gui::model::MidiInputSettings*,
        midiInputSettings)

    PIEJAM_GUI_CONSTANT_PROPERTY(
        piejam::gui::model::DisplaySettings*,
        displaySettings)

    PIEJAM_GUI_CONSTANT_PROPERTY(
        piejam::gui::model::SessionSettings*,
        sessionSettings)

    PIEJAM_GUI_CONSTANT_PROPERTY(
        piejam::gui::model::NetworkSettings*,
        networkSettings)

    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::Mixer*, mixer)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::Info*, info)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::Log*, log)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::FxBrowser*, fxBrowser)
    PIEJAM_GUI_CONSTANT_PROPERTY(piejam::gui::model::FxModuleView*, fxModule)

public:
    enum class Mode : int
    {
        Mixer,
        Info,
        Settings,
        Power,
        FxBrowser,
        FxModule,
    };

    Q_ENUM(Mode)

private:
    PIEJAM_GUI_PROPERTY(bool, canShowFxModule, setCanShowFxModule)
    PIEJAM_GUI_PROPERTY(Mode, mode, setMode)

public:
    explicit Root(
        runtime::state_access const&,
        std::filesystem::path sessions_dir,
        std::shared_ptr<network_manager::network_controller> = nullptr,
        std::shared_ptr<network_manager::wifi_manager> = nullptr,
        std::shared_ptr<network_manager::nfs_server> = nullptr,
        std::shared_ptr<network_manager::nfs_client> = nullptr);

    Q_INVOKABLE void showMixer();
    Q_INVOKABLE void showFxModule();
    Q_INVOKABLE void showInfo();
    Q_INVOKABLE void showSettings();
    Q_INVOKABLE void showPower();

private:
    void onSubscribe() override;

    void switchRootViewMode(runtime::root_view_mode);
};

} // namespace piejam::gui::model
