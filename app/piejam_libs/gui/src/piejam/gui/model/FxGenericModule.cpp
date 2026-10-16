// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/FxGenericModule.h>

#include <piejam/gui/model/BoolParameter.h>
#include <piejam/gui/model/EnumParameter.h>
#include <piejam/gui/model/FloatParameter.h>
#include <piejam/gui/model/IntParameter.h>
#include <piejam/gui/model/ObjectListModel.h>
#include <piejam/gui/model/Parameter.h>

#include <piejam/runtime/parameter/map.h>
#include <piejam/runtime/parameters.h>
#include <piejam/runtime/selectors.h>

#include <boost/container/flat_map.hpp>
#include <boost/mp11/map.hpp>
#include <boost/polymorphic_cast.hpp>

namespace piejam::gui::model
{

namespace
{

using IdToModel = boost::mp11::mp_list<
    boost::mp11::mp_list<runtime::bool_parameter_id, BoolParameter>,
    boost::mp11::mp_list<runtime::enum_parameter_id, EnumParameter>,
    boost::mp11::mp_list<runtime::float_parameter_id, FloatParameter>,
    boost::mp11::mp_list<runtime::int_parameter_id, IntParameter>>;

auto
makeParameter(
    runtime::state_access const& state_access,
    runtime::parameter_id const& paramId) -> std::unique_ptr<Parameter>
{
    return std::visit(
        [&]<class ParamId>(
            ParamId const typed_param_id) -> std::unique_ptr<Parameter> {
            using ParameterModel = boost::mp11::mp_second<
                boost::mp11::mp_map_find<IdToModel, ParamId>>;
            return std::make_unique<ParameterModel>(
                state_access,
                typed_param_id);
        },
        paramId);
}

} // namespace

FxGenericModule::FxGenericModule(
    runtime::state_access const& state_access,
    runtime::fx::module_id const fx_mod_id)
    : FxModule{state_access, fx_mod_id}
    , m_parametersList{&addQObject<FxParametersList>()}
{
    auto& parametersList =
        boost::polymorphic_downcast<FxParametersList&>(*m_parametersList);
    for (auto const& [key, paramId] : parameters())
    {
        parametersList.add(
            parametersList.size(),
            model::makeParameter(state_access, paramId));
    }
}

void
FxGenericModule::onSubscribe()
{
}

} // namespace piejam::gui::model
