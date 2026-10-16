// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/FxModule.h>

#include <piejam/runtime/selectors.h>

namespace piejam::gui::model
{

struct FxModule::Impl
{
    box<runtime::parameters_map> parameters;
    box<runtime::fx::module_streams> streams;
};

FxModule::FxModule(
    runtime::state_access const& state_access,
    runtime::fx::module_id const fx_mod_id)
    : CompositeSubscribableModel{state_access}
    , m_impl{make_pimpl<Impl>(
          observe_once(
              runtime::selectors::make_fx_module_parameters_selector(
                  fx_mod_id)),
          observe_once(
              runtime::selectors::make_fx_module_streams_selector(fx_mod_id)))}
    , m_busType{bool_enum_to<BusType>(observe_once(
          runtime::selectors::make_fx_module_bus_type_selector(fx_mod_id)))}
{
}

auto
FxModule::parameters() const -> runtime::parameters_map const&
{
    return m_impl->parameters;
}

auto
FxModule::streams() const -> runtime::fx::module_streams const&
{
    return m_impl->streams;
}

} // namespace piejam::gui::model
