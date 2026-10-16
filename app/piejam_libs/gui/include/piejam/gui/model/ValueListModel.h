// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/indexed_access.h>

#include <QAbstractListModel>

#include <boost/assert.hpp>

#include <vector>

namespace piejam::gui::model
{

template <class ListItem>
class ValueListModel : public QAbstractListModel
{
public:
    enum Roles : int
    {
        ValueRole = Qt::UserRole
    };

    using QAbstractListModel::QAbstractListModel;

    auto rowCount(QModelIndex const& = QModelIndex()) const -> int override
    {
        return static_cast<int>(m_list.size());
    }

    auto data(QModelIndex const& index, int const role = Qt::DisplayRole) const
        -> QVariant override
    {
        BOOST_ASSERT(static_cast<std::size_t>(index.row()) < m_list.size());
        switch (role)
        {
            case Qt::DisplayRole:
                return itemToString(m_list[index.row()]);
            case ValueRole:
                return QVariant::fromValue(m_list[index.row()]);
            default:
                return {};
        }
    }

    auto roleNames() const -> QHash<int, QByteArray> override
    {
        static QHash<int, QByteArray> s_roles = {
            {Qt::DisplayRole, "text"},
            {ValueRole, "value"},
        };
        return s_roles;
    }

    auto get(int index, int roleId) const -> QVariant
    {
        if (index < 0 || rowCount() <= index)
        {
            return {};
        }

        return data(this->index(index), roleId);
    }

    auto get(int index, QString const& roleName) const -> QVariant
    {
        return get(index, roleNames().key(roleName, -1));
    }

    [[nodiscard]]
    auto size() const -> std::size_t
    {
        return m_list.size();
    }

    void set(std::vector<ListItem> values)
    {
        beginResetModel();
        m_list = std::move(values);
        endResetModel();
        onSizeChanged();
    }

    void add(std::size_t const pos, ListItem item)
    {
        BOOST_ASSERT(pos <= m_list.size());
        beginInsertRows(
            QModelIndex(),
            static_cast<int>(pos),
            static_cast<int>(pos));
        insert_at(m_list, pos, std::move(item));
        endInsertRows();
        onSizeChanged();
    }

    void append(ListItem item)
    {
        add(m_list.size(), std::move(item));
    }

    void remove(std::size_t const pos)
    {
        BOOST_ASSERT(pos < m_list.size());
        beginRemoveRows(
            QModelIndex(),
            static_cast<int>(pos),
            static_cast<int>(pos));
        BOOST_ASSERT(!m_list.empty());
        erase_at(m_list, pos);
        endRemoveRows();
        onSizeChanged();
    }

    auto at(std::size_t pos) const -> ListItem const&
    {
        BOOST_ASSERT(pos < m_list.size());
        return m_list[pos];
    }

protected:
    virtual void onSizeChanged()
    {
    }

    virtual auto itemToString(ListItem const&) const -> QString = 0;

private:
    std::vector<ListItem> m_list;
};

} // namespace piejam::gui::model
