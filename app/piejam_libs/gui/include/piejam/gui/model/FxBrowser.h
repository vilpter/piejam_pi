// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/SubscribableModel.h>
#include <piejam/gui/model/Types.h>
#include <piejam/gui/model/fwd.h>

namespace piejam::gui::model
{

class FxBrowser final : public CompositeSubscribableModel
{
    Q_OBJECT

    PIEJAM_GUI_MODEL_PIMPL

    PIEJAM_GUI_CONSTANT_PROPERTY(QAbstractListModel*, entries)

public:
    explicit FxBrowser(runtime::state_access const&);

    Q_INVOKABLE void showMixer();

private:
    void onSubscribe() override;
};

} // namespace piejam::gui::model
