// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/SubscribableModel.h>
#include <piejam/gui/model/fwd.h>

#include <piejam/pimpl.h>
#include <piejam/runtime/fx/fwd.h>
#include <piejam/runtime/mixer_fwd.h>

namespace piejam::gui::model
{

class FxChainModule : public SubscribableModel
{
    Q_OBJECT

    PIEJAM_GUI_PROPERTY(QString, name, setName)
    PIEJAM_GUI_PROPERTY(bool, focused, setFocused)

public:
    FxChainModule(
        runtime::state_access const&,
        runtime::mixer::channel_id fx_chain_id,
        runtime::fx::module_id);

    Q_INVOKABLE void remove();
    Q_INVOKABLE void focus();
    Q_INVOKABLE void showFxModule();

private:
    void onSubscribe() override;

    struct Impl;
    pimpl<Impl> m_impl;
};

} // namespace piejam::gui::model
