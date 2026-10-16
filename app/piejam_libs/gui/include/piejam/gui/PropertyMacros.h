// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/pimpl.h>

#include <qobjectdefs.h>

#include <type_traits>

class QAbstractListModel;

namespace piejam::gui::detail
{

template <class T>
using property_param_t = std::conditional_t<
    std::is_trivially_copyable_v<T> && sizeof(T) <= 16,
    T,
    T const&>;

} // namespace piejam::gui::detail

#define PIEJAM_GUI_MODEL_PIMPL                                                 \
    struct Impl;                                                               \
    pimpl<Impl> m_impl;

#define PIEJAM_GUI_PROPERTY(type, name, setterName)                            \
private:                                                                       \
    Q_PROPERTY(type name READ name NOTIFY name##Changed FINAL)                 \
    type m_##name{};                                                           \
                                                                               \
public:                                                                        \
    using name##_property_t = piejam::gui::detail::property_param_t<type>;     \
                                                                               \
    Q_SIGNAL void name##Changed();                                             \
    auto name() const noexcept -> name##_property_t                            \
    {                                                                          \
        return m_##name;                                                       \
    }                                                                          \
                                                                               \
    void setterName(name##_property_t x)                                       \
    {                                                                          \
        if (m_##name != x)                                                     \
        {                                                                      \
            m_##name = x;                                                      \
            Q_EMIT name##Changed();                                            \
        }                                                                      \
    }                                                                          \
                                                                               \
private:

#define PIEJAM_GUI_CONSTANT_PROPERTY(type, name)                               \
private:                                                                       \
    Q_PROPERTY(type name READ name CONSTANT FINAL)                             \
    type m_##name{};                                                           \
                                                                               \
public:                                                                        \
    using name##_property_t = piejam::gui::detail::property_param_t<type>;     \
                                                                               \
    auto name() const noexcept -> name##_property_t                            \
    {                                                                          \
        return m_##name;                                                       \
    }                                                                          \
                                                                               \
private:

#define PIEJAM_GUI_WRITABLE_PROPERTY(type, name, setterName)                   \
private:                                                                       \
    Q_PROPERTY(                                                                \
        type name READ name WRITE setterName NOTIFY name##Changed FINAL)       \
    type m_##name{};                                                           \
                                                                               \
public:                                                                        \
    using name##_property_t = piejam::gui::detail::property_param_t<type>;     \
                                                                               \
    Q_SIGNAL void name##Changed();                                             \
    auto name() const noexcept -> name##_property_t                            \
    {                                                                          \
        return m_##name;                                                       \
    }                                                                          \
                                                                               \
    void setterName(name##_property_t x)                                       \
    {                                                                          \
        if (m_##name != x)                                                     \
        {                                                                      \
            m_##name = x;                                                      \
            Q_EMIT name##Changed();                                            \
        }                                                                      \
    }                                                                          \
                                                                               \
private:
