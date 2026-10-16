// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/SubscribableModel.h>
#include <piejam/gui/model/fwd.h>

#include <piejam/io_direction.h>
#include <piejam/runtime/mixer_fwd.h>

namespace piejam::gui::model
{

class AudioRouting final : public CompositeSubscribableModel
{
    Q_OBJECT

    PIEJAM_GUI_MODEL_PIMPL

    PIEJAM_GUI_CONSTANT_PROPERTY(bool, mixIsAvailable)
    PIEJAM_GUI_PROPERTY(bool, mixIsValid, setMixIsValid)
    PIEJAM_GUI_CONSTANT_PROPERTY(
        piejam::gui::model::AudioRoutingSelection*,
        selected)
    PIEJAM_GUI_CONSTANT_PROPERTY(QAbstractListModel*, devices)
    PIEJAM_GUI_CONSTANT_PROPERTY(QAbstractListModel*, channels)

public:
    AudioRouting(
        runtime::state_access const&,
        runtime::mixer::channel_id,
        io_direction io_dir);

    Q_INVOKABLE void changeToNone();
    Q_INVOKABLE void changeToMix();
    Q_INVOKABLE void changeToDevice(unsigned index);
    Q_INVOKABLE void changeToChannel(unsigned index);

private:
    void onSubscribe() override;
};

} // namespace piejam::gui::model
