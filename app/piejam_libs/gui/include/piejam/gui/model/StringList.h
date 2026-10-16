// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/model/ValueListModel.h>

#include <QObject>
#include <QString>

namespace piejam::gui::model
{

class StringList final : public ValueListModel<QString>
{
    using Base = ValueListModel<QString>;

    Q_OBJECT

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)

public:
    using Base::Base;

    Q_INVOKABLE QVariant get(int index) const
    {
        return Base::get(index, Qt::DisplayRole);
    }

    Q_SIGNAL void countChanged();

private:
    void onSizeChanged() override
    {
        emit countChanged();
    }

    auto itemToString(QString const& item) const -> QString override
    {
        return item;
    }
};

} // namespace piejam::gui::model
