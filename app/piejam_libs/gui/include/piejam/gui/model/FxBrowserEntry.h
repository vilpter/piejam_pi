// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/SubscribableModel.h>

#include <piejam/runtime/mixer_fwd.h>

namespace piejam::gui::model
{

class FxBrowserEntry : public SubscribableModel
{
    Q_OBJECT

    PIEJAM_GUI_PROPERTY(QString, name, setName)
    PIEJAM_GUI_PROPERTY(QString, section, setSection)
    PIEJAM_GUI_PROPERTY(QString, description, setDescription)
    PIEJAM_GUI_PROPERTY(QString, author, setAuthor)

public:
    using SubscribableModel::SubscribableModel;

    Q_INVOKABLE virtual void appendModule() = 0;
};

} // namespace piejam::gui::model
