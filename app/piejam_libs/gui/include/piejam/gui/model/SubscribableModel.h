// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/entity_id_hash.h>
#include <piejam/pimpl.h>
#include <piejam/redux/subscriber.h>
#include <piejam/runtime/state_access.h>
#include <piejam/runtime/subscriber.h>

#include <QObject>
#include <QTimerEvent>

#include <boost/assert.hpp>

#include <chrono>
#include <functional>
#include <utility>
#include <vector>

namespace piejam::gui::model
{

class SubscribableModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(
        bool subscribed READ subscribed WRITE setSubscribed NOTIFY
            subscribedChanged)

public:
    auto subscribed() const -> bool
    {
        return m_subscribed;
    }

    void setSubscribed(bool subs)
    {
        if (m_subscribed != subs)
        {
            if (subs)
            {
                subscribe();
            }
            else
            {
                unsubscribe();
            }

            m_subscribed = subs;

            emit subscribedChanged();
        }
    }

signals:
    void subscribedChanged();

protected:
    SubscribableModel(runtime::state_access state_access)
        : m_state_access(state_access)
    {
    }

    auto state_access() const noexcept -> runtime::state_access const&
    {
        return m_state_access;
    }

    auto dispatch(runtime::action const& a) const
    {
        m_state_access.dispatch(a);
    }

    template <class Value>
    auto observe_once(runtime::selector<Value> const& sel)
    {
        return m_state_access.observe_once(sel);
    }

    template <class Value, class Handler>
    void observe(runtime::selector<Value> sel, Handler&& h)
    {
        m_subs.emplace_back(
            m_state_access.observe(std::move(sel), std::forward<Handler>(h)));
    }

    template <class F>
    void requestUpdates(std::chrono::milliseconds t, F&& f)
    {
        BOOST_ASSERT(!m_updateTimerId);
        m_requestUpdate = std::forward<F>(f);
        m_updateTimerId = QObject::startTimer(t);
    }

    virtual void onSubscribe() = 0;

private:
    void subscribe()
    {
        onSubscribe();
    }

    void unsubscribe()
    {
        m_subs.clear();

        if (m_updateTimerId != 0)
        {
            QObject::killTimer(m_updateTimerId);
            m_updateTimerId = 0;
        }
    }

    void timerEvent([[maybe_unused]] QTimerEvent* const event) final
    {
        BOOST_ASSERT(event->timerId() == m_updateTimerId);
        BOOST_ASSERT(m_requestUpdate);
        m_requestUpdate();
    }

    runtime::state_access m_state_access;

    bool m_subscribed{};
    std::vector<redux::subscription> m_subs;

    int m_updateTimerId{};
    std::function<void()> m_requestUpdate;
};

class CompositeSubscribableModel : public SubscribableModel
{
public:
    using SubscribableModel::SubscribableModel;

protected:
    auto attachChildModel(SubscribableModel& child)
    {
        return QObject::connect(
            this,
            &SubscribableModel::subscribedChanged,
            &child,
            [this, &child]() { child.setSubscribed(subscribed()); });
    }

    template <class Object, class... Args>
    auto addQObject(Args&&... args) -> Object&
    {
        auto child = make_pimpl<Object>(std::forward<Args>(args)...);
        auto result = child.get();
        m_objects.emplace_back(std::move(child));
        return *result;
    }

    template <class Model, class... Args>
    auto addModel(Args&&... args) -> Model&
    {
        return addQObject<Model>(state_access(), std::forward<Args>(args)...);
    }

    template <class Model, class... Args>
    auto addAttachedModel(Args&&... args) -> Model&
    {
        auto& child = addModel<Model>(std::forward<Args>(args)...);
        attachChildModel(child);
        return child;
    }

private:
    std::vector<pimpl<QObject>> m_objects;
};

} // namespace piejam::gui::model
