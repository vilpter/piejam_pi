// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/FxModuleView.h>

#include <piejam/gui/model/BoolParameter.h>
#include <piejam/gui/model/FxGenericModule.h>
#include <piejam/gui/model/FxModuleFactory.h>

#include <piejam/runtime/actions/fx_chain_actions.h>
#include <piejam/runtime/fx/module.h>
#include <piejam/runtime/selectors.h>

#include <boost/assert.hpp>
#include <boost/hof/match.hpp>

namespace piejam::gui::model
{

struct FxModuleView::Impl
{
    runtime::fx::module_id fx_mod_id;
    runtime::bool_parameter_id active_id;

    std::unique_ptr<FxModule> content;
    std::unique_ptr<BoolParameter> active;
};

auto
makeModuleContent(
    runtime::state_access const& state_access,
    runtime::fx::module_id const fx_mod_id) -> std::unique_ptr<FxModule>
{
    auto const fx_instance_id = state_access.observe_once(
        runtime::selectors::make_fx_module_instance_id_selector(fx_mod_id));

    return std::visit(
        boost::hof::match(
            [&](runtime::fx::internal_id fx_type) -> std::unique_ptr<FxModule> {
                return FxModuleFactories::lookup(
                    fx_type)(state_access, fx_mod_id);
            },
            [&](auto const&) -> std::unique_ptr<FxModule> {
                return std::make_unique<FxGenericModule>(
                    state_access,
                    fx_mod_id);
            }),
        fx_instance_id);
}

FxModuleView::FxModuleView(runtime::state_access const& state_access)
    : SubscribableModel(state_access)
    , m_impl{make_pimpl<Impl>()}
{
}

void
FxModuleView::onSubscribe()
{
    setColor(
        static_cast<MaterialColor>(
            observe_once(runtime::selectors::select_focused_fx_module_color)));

    setChainName(
        QString::fromStdString(observe_once(
            runtime::selectors::make_mixer_channel_name_string_selector(
                observe_once(runtime::selectors::select_focused_fx_chain)))));

    observe(
        runtime::selectors::select_focused_fx_module_name,
        [this](std::string const& name) {
            setName(QString::fromStdString(name));
        });

    observe(
        runtime::selectors::select_focused_fx_module,
        [this](runtime::fx::module_id const fx_mod_id) {
            if (m_impl->fx_mod_id != fx_mod_id)
            {
                m_impl->fx_mod_id = fx_mod_id;

                auto content = makeModuleContent(state_access(), fx_mod_id);
                std::swap(m_impl->content, content);
                setContent(m_impl->content.get());

                auto active = std::make_unique<BoolParameter>(
                    state_access(),
                    observe_once(
                        runtime::selectors::make_fx_module_active_selector(
                            fx_mod_id)));
                std::swap(m_impl->active, active);
                setActive(m_impl->active.get());
            }
        });

    observe(
        runtime::selectors::select_can_show_prev_fx_module,
        [this](bool const can_show_prev) { setCanShowPrev(can_show_prev); });

    observe(
        runtime::selectors::select_can_show_next_fx_module,
        [this](bool const can_show_next) { setCanShowNext(can_show_next); });
}

void
FxModuleView::showPrevModule()
{
    BOOST_ASSERT(canShowPrev());

    dispatch(runtime::actions::show_prev_fx_module{});
}

void
FxModuleView::showNextModule()
{
    BOOST_ASSERT(canShowNext());

    dispatch(runtime::actions::show_next_fx_module{});
}

} // namespace piejam::gui::model
