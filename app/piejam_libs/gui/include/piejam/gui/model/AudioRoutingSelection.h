// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/SubscribableModel.h>
#include <piejam/gui/model/fwd.h>

#include <piejam/io_direction.h>
#include <piejam/pimpl.h>
#include <piejam/runtime/mixer_fwd.h>

namespace piejam::gui::model
{

class AudioRoutingSelection final : public SubscribableModel
{
    Q_OBJECT

public:
    enum class State
    {
        Invalid,
        Valid,
        NotMixed
    };

    Q_ENUM(State)

private:
    PIEJAM_GUI_PROPERTY(State, state, setState)
    PIEJAM_GUI_PROPERTY(QString, label, setLabel)

public:
    AudioRoutingSelection(
        runtime::state_access const&,
        runtime::mixer::channel_id,
        io_direction);

private:
    void onSubscribe() override;

    struct Impl;
    pimpl<Impl> m_impl;
};

} // namespace piejam::gui::model
