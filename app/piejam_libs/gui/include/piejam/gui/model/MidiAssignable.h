// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/SubscribableModel.h>

namespace piejam::gui::model
{

class MidiAssignable final : public SubscribableModel
{
    Q_OBJECT

    PIEJAM_GUI_PROPERTY(bool, learning, setLearning)
    PIEJAM_GUI_PROPERTY(QString, assignment, setAssignment)

public:
    MidiAssignable(runtime::state_access const&, runtime::parameter_id const&);

    Q_INVOKABLE void startLearn();
    Q_INVOKABLE void stopLearn();
    Q_INVOKABLE void clearAssignment();

private:
    void onSubscribe() override;

    runtime::parameter_id m_assignment_id;
};

} // namespace piejam::gui::model
