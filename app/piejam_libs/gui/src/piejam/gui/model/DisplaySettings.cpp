// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/DisplaySettings.h>

#include <piejam/gui/model/StringList.h>

#include <piejam/algorithm/index_of.h>
#include <piejam/runtime/actions/display_actions.h>
#include <piejam/runtime/selectors.h>

#include <boost/assert.hpp>
#include <boost/polymorphic_cast.hpp>

namespace piejam::gui::model
{

namespace
{

constexpr auto display_rotations = std::array{0uz, 90uz, 180uz, 270uz};

} // namespace

DisplaySettings::DisplaySettings(runtime::state_access const& state_access)
    : CompositeSubscribableModel{state_access}
    , m_rotations{&addQObject<StringList>()}
{
    auto& rotations = boost::polymorphic_downcast<StringList&>(*m_rotations);
    rotations.append("0");
    rotations.append("90");
    rotations.append("180");
    rotations.append("270");
}

void
DisplaySettings::onSubscribe()
{
    observe(
        runtime::selectors::select_display_rotation,
        [this](std::size_t rotation) {
            auto index = algorithm::index_of(display_rotations, rotation);
            if (index != algorithm::npos)
            {
                setRotation(static_cast<int>(index));
            }
        });
}

void
DisplaySettings::selectRotation(unsigned index)
{
    BOOST_ASSERT(index < display_rotations.size());

    runtime::actions::set_display_rotation action;
    action.display_rotation = display_rotations[index];
    dispatch(action);
}

} // namespace piejam::gui::model
