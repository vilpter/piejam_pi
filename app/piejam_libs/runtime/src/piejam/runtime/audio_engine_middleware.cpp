// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/runtime/audio_engine_middleware.h>

#include <piejam/runtime/actions/activate_midi_device.h>
#include <piejam/runtime/actions/apply_app_config.h>
#include <piejam/runtime/actions/audio_engine_sync.h>
#include <piejam/runtime/actions/control_midi_assignment.h>
#include <piejam/runtime/actions/deactivate_midi_device.h>
#include <piejam/runtime/actions/external_audio_device_actions.h>
#include <piejam/runtime/actions/initiate_sound_card_selection.h>
#include <piejam/runtime/actions/recording.h>
#include <piejam/runtime/actions/select_period_size.h>
#include <piejam/runtime/actions/select_sample_rate.h>
#include <piejam/runtime/actions/set_parameter_value.h>
#include <piejam/runtime/audio_engine.h>
#include <piejam/runtime/fwd.h>
#include <piejam/runtime/middleware_functors.h>
#include <piejam/runtime/midi_input_controller.h>
#include <piejam/runtime/state.h>

#include <piejam/algorithm/find_or_get_first.h>
#include <piejam/algorithm/for_each_visit.h>
#include <piejam/algorithm/index_of.h>
#include <piejam/algorithm/transform_to_vector.h>
#include <piejam/audio/engine/processor.h>
#include <piejam/audio/engine/rt_task_executor.h>
#include <piejam/audio/io_process.h>
#include <piejam/audio/multichannel_buffer.h>
#include <piejam/audio/sound_card_config.h>
#include <piejam/audio/sound_card_descriptor.h>
#include <piejam/audio/sound_card_hw_params.h>
#include <piejam/audio/sound_card_manager.h>
#include <piejam/functional/operators.h>
#include <piejam/ladspa/processor_factory.h>
#include <piejam/midi/event.h>
#include <piejam/midi/input_event_handler.h>
#include <piejam/tuple.h>
#include <piejam/tuple_element_compare.h>

#include <spdlog/spdlog.h>

#include <boost/assert.hpp>
#include <boost/mp11/tuple.hpp>
#include <boost/range/algorithm_ext/erase.hpp>

namespace piejam::runtime
{

namespace
{

struct update_devices final
    : ui::cloneable_action<update_devices, reducible_action>
{
    box<piejam::audio::sound_cards> sound_cards;

    selected_sound_card selected_sc;

    audio::sample_rate sample_rate{};
    audio::period_size period_size{};

    void reduce(state& st) const override
    {
        st.sound_cards = sound_cards;
        st.selected_sound_card = selected_sc;
        st.sample_rate = sample_rate;
        st.period_size = period_size;
    }
};

struct update_info final : ui::cloneable_action<update_info, reducible_action>
{
    std::size_t xruns{};
    float cpu_load{};

    void reduce(state& st) const override
    {
        st.xruns = xruns;
        st.cpu_load = cpu_load;
    }
};

static auto
current_rebuild_tracker_state(state const& st)
{
    return std::tuple{
        st.mixer_state.io_map,
        st.mixer_state.fx_chains,
        st.external_audio_state.device_channels,
        st.midi_learning.has_value(),
        st.audio_graph_update_count};
}

} // namespace

struct audio_engine_middleware::rebuild_tracker
{
    auto check(state const& st) -> bool
    {
        auto current = current_rebuild_tracker_state(st);

        if (last != current)
        {
            last = std::move(current);
            return true;
        }

        return false;
    }

private:
    using last_t =
        tuple::decay_elements_t<decltype(current_rebuild_tracker_state(
            std::declval<state>()))>;

    last_t last;
};

audio_engine_middleware::audio_engine_middleware(
    thread::configuration const& audio_thread_config,
    std::span<thread::configuration const> const wt_configs,
    audio::sound_card_manager& sound_card_manager,
    ladspa::processor_factory& ladspa_processor_factory,
    std::unique_ptr<midi_input_controller> midi_controller)
    : m_audio_thread_config(audio_thread_config)
    , m_workers(wt_configs.begin(), wt_configs.end())
    , m_sound_card_manager(sound_card_manager)
    , m_ladspa_processor_factory(ladspa_processor_factory)
    , m_midi_controller(
          midi_controller ? std::move(midi_controller)
                          : make_dummy_midi_input_controller())
    , m_io_process(audio::make_dummy_io_process())
    , m_rebuild_tracker{make_pimpl<rebuild_tracker>()}
{
}

audio_engine_middleware::~audio_engine_middleware() = default;

static auto
make_update_devices_action(
    audio::sound_card_manager& device_manager,
    box<audio::sound_cards> const& new_devices,
    box<audio::sound_cards> const& current_devices,
    std::size_t const current_index,
    audio::sample_rate const sample_rate,
    audio::period_size const period_size) -> update_devices
{
    update_devices next_action;
    next_action.sound_cards = new_devices;

    next_action.selected_sc.index =
        current_index != npos ? algorithm::index_of(
                                    *new_devices,
                                    current_devices.get()[current_index])
                              : npos;

    if (next_action.selected_sc.index == npos)
    {
        return next_action;
    }

    auto const& selected_sc = new_devices.get()[next_action.selected_sc.index];

    next_action.selected_sc.num_channels = selected_sc.num_channels;
    next_action.selected_sc.hw_params =
        box(device_manager.get_hw_params(selected_sc, {}, {}));

    auto next_value = [](auto const& values, auto current) {
        auto const it = algorithm::find_or_get_first(values, current);
        return it != values.end() ? *it : decltype(*it){};
    };

    next_action.sample_rate = next_value(
        next_action.selected_sc.hw_params->sample_rates,
        sample_rate.invalid() ? audio::sample_rate{48000} : sample_rate);

    if (next_action.sample_rate.valid())
    {
        next_action.selected_sc.hw_params = device_manager.get_hw_params(
            selected_sc,
            next_action.sample_rate,
            {});
    }

    next_action.period_size = next_value(
        next_action.selected_sc.hw_params->period_sizes,
        period_size.invalid() ? audio::period_size{192} : period_size);

    if (next_action.period_size.valid())
    {
        next_action.selected_sc.hw_params = device_manager.get_hw_params(
            selected_sc,
            next_action.sample_rate,
            next_action.period_size);
    }

    return next_action;
}

template <>
void
audio_engine_middleware::process_device_action(
    middleware_functors const& mw_fs,
    actions::apply_app_config const& a)
{
    state const& current_state = mw_fs.get_state();

    mw_fs.next(make_update_devices_action(
        m_sound_card_manager,
        current_state.sound_cards,
        current_state.sound_cards,
        algorithm::index_of(
            current_state.sound_cards.get(),
            a.conf.sound_card,
            &audio::sound_card_descriptor::name),
        a.conf.sample_rate,
        a.conf.period_size));

    mw_fs.next(a);
}

template <>
void
audio_engine_middleware::process_device_action(
    middleware_functors const& mw_fs,
    actions::refresh_sound_cards const&)
{
    state const& current_state = mw_fs.get_state();

    mw_fs.next(make_update_devices_action(
        m_sound_card_manager,
        box(m_sound_card_manager.get_sound_cards()),
        current_state.sound_cards,
        current_state.selected_sound_card.index,
        current_state.sample_rate,
        current_state.period_size));
}

template <>
void
audio_engine_middleware::process_device_action(
    middleware_functors const& mw_fs,
    actions::initiate_sound_card_selection const& action)
{
    state const& current_state = mw_fs.get_state();

    mw_fs.next(make_update_devices_action(
        m_sound_card_manager,
        current_state.sound_cards,
        current_state.sound_cards,
        action.index,
        current_state.sample_rate,
        current_state.period_size));
}

template <>
void
audio_engine_middleware::process_device_action(
    middleware_functors const& mw_fs,
    actions::select_sample_rate const& action)
{
    state const& current_state = mw_fs.get_state();

    auto const& srs = current_state.selected_sound_card.hw_params->sample_rates;
    if (action.index < srs.size())
    {
        mw_fs.next(make_update_devices_action(
            m_sound_card_manager,
            current_state.sound_cards,
            current_state.sound_cards,
            current_state.selected_sound_card.index,
            srs[action.index],
            current_state.period_size));
    }
}

template <>
void
audio_engine_middleware::process_device_action(
    middleware_functors const& mw_fs,
    actions::select_period_size const& action)
{
    state const& current_state = mw_fs.get_state();

    auto const& pss = current_state.selected_sound_card.hw_params->period_sizes;
    if (action.index < pss.size())
    {
        mw_fs.next(make_update_devices_action(
            m_sound_card_manager,
            current_state.sound_cards,
            current_state.sound_cards,
            current_state.selected_sound_card.index,
            current_state.sample_rate,
            pss[action.index]));
    }
}

template <>
void
audio_engine_middleware::process_device_action(
    middleware_functors const& mw_fs,
    actions::activate_midi_device const& action)
{
    if (m_midi_controller->activate_input_device(action.device_id))
    {
        mw_fs.next(action);
    }
}

template <>
void
audio_engine_middleware::process_device_action(
    middleware_functors const& mw_fs,
    actions::deactivate_midi_device const& action)
{
    m_midi_controller->deactivate_input_device(action.device_id);
    mw_fs.next(action);
}

template <class Parameter>
void
audio_engine_middleware::process_engine_action(
    middleware_functors const& mw_fs,
    actions::set_parameter_value<Parameter> const& a)
{
    if (m_engine)
    {
        m_engine->set_parameter_value(a.id, a.value);
    }

    mw_fs.next(a);
}

template <std::ranges::range Parameters>
static void
collect_parameter_updates(
    Parameters&& params,
    audio_engine const& engine,
    actions::audio_engine_sync_update& action)
{
    algorithm::for_each_visit(std::forward<Parameters>(params), [&](auto id) {
        if (auto value = engine.get_parameter_update(id); value)
        {
            action.push_back(id, *value);
        }
    });
}

template <std::ranges::range Streams>
static void
collect_stream_updates(
    Streams const& streams,
    audio_engine const& engine,
    actions::audio_engine_sync_update& action)
{
    for (auto id : streams)
    {
        if (auto captured = engine.get_stream(id); !captured->empty())
        {
            action.streams.emplace(id, std::move(captured));
        }
    }
}

template <>
void
audio_engine_middleware::process_engine_action(
    middleware_functors const& mw_fs,
    actions::request_audio_engine_sync const&)
{
    if (m_engine)
    {
        state const& st = mw_fs.get_state();

        actions::audio_engine_sync_update next_action;

        collect_parameter_updates(
            st.midi_assignments.get() | std::views::keys,
            *m_engine,
            next_action);

        collect_stream_updates(
            st.streams | std::views::keys,
            *m_engine,
            next_action);

        if (!next_action.empty())
        {
            mw_fs.next(next_action);
        }
    }
}

template <>
void
audio_engine_middleware::process_engine_action(
    middleware_functors const& mw_fs,
    actions::stop_recording const& a)
{
    mw_fs.next(a);
    rebuild(mw_fs.get_state());
}

template <>
void
audio_engine_middleware::process_engine_action(
    middleware_functors const& mw_fs,
    actions::request_info_update const&)
{
    {
        update_info next_action;
        next_action.xruns = m_io_process->xruns();
        next_action.cpu_load = m_io_process->cpu_load();

        mw_fs.next(next_action);
    }

    if (m_engine)
    {
        if (auto learned_midi = m_engine->get_learned_midi())
        {
            std::visit(
                boost::hof::match(
                    [&](midi::channel_cc_event const& cc_event) {
                        actions::stop_midi_learning next_action;
                        next_action.learned = midi_assignment{
                            .channel = cc_event.channel,
                            .control_type = midi_assignment::type::cc,
                            .control_id = cc_event.data.cc};

                        mw_fs.next(next_action);
                    },
                    [&](midi::channel_pitch_bend_event const& pb_event) {
                        actions::stop_midi_learning next_action;
                        next_action.learned = midi_assignment{
                            .channel = pb_event.channel,
                            .control_type = midi_assignment::type::pitch_bend,
                        };

                        mw_fs.next(next_action);
                    }),
                learned_midi->event);
        }
    }
}

void
audio_engine_middleware::close_sound_card()
{
    m_io_process = audio::make_dummy_io_process();

    // The engine is executed by a device, we can safely destroy it after device
    // was closed.
    m_engine.reset();
}

void
audio_engine_middleware::open_sound_card(state const& st)
{
    if (st.selected_sound_card.index == npos || st.sample_rate.invalid() ||
        st.period_size.invalid())
    {
        return;
    }

    try
    {
        auto io_process = m_sound_card_manager.make_io_process(
            st.sound_cards.get()[st.selected_sound_card.index],
            audio::sound_card_config{st.sample_rate, st.period_size});
        m_io_process.swap(io_process);
    }
    catch (std::exception const& err)
    {
        spdlog::error("opening device failed: {}", err.what());
    }
}

void
audio_engine_middleware::start_engine(state const& st)
{
    BOOST_ASSERT(m_io_process);

    if (m_io_process->is_open())
    {
        m_engine = std::make_unique<audio_engine>(
            m_workers,
            st.sample_rate,
            st.selected_sound_card.num_channels.in(),
            st.selected_sound_card.num_channels.out());

        m_io_process->start(
            m_audio_thread_config,
            [engine = m_engine.get()](auto const& in, auto const& out) {
                engine->init_process(in, out);
            },
            [engine = m_engine.get()](auto const buffer_size) {
                return engine->process(buffer_size);
            });

        while (!m_io_process->is_running())
        {
            std::this_thread::yield();
        }

        rebuild(st);
    }
}

void
audio_engine_middleware::rebuild(state const& st)
{
    if (!m_engine || !m_io_process->is_running())
    {
        return;
    }

    if (!m_engine->rebuild(
            st,
            [this, sr = st.sample_rate](ladspa::instance_id id) {
                return m_ladspa_processor_factory.make_processor(id, sr);
            },
            m_midi_controller->make_input_event_handler()))
    {
        spdlog::error("Rebuilding audio engine graph failed.");
    }
}

void
audio_engine_middleware::operator()(
    middleware_functors const& mw_fs,
    action const& action)
{
    if (auto a = dynamic_cast<actions::audio_io_process_action const*>(&action))
    {
        auto const& st = mw_fs.get_state();

        if (st.recording)
        {
            process_engine_action(mw_fs, actions::stop_recording{});
        }

        close_sound_card();

        auto v =
            ui::make_action_visitor<actions::audio_io_process_action_visitor>(
                [&](auto&& a) { process_device_action(mw_fs, a); });

        a->visit(v);

        open_sound_card(st);
        start_engine(st);
    }
    else if (
        auto a = dynamic_cast<actions::audio_engine_action const*>(&action))
    {
        auto v = ui::make_action_visitor<actions::audio_engine_action_visitor>(
            [&](auto&& a) { process_engine_action(mw_fs, a); });

        a->visit(v);

        if (m_rebuild_tracker->check(mw_fs.get_state()))
        {
            rebuild(mw_fs.get_state());
        }
    }
    else
    {
        mw_fs.next(action);

        if (m_rebuild_tracker->check(mw_fs.get_state()))
        {
            rebuild(mw_fs.get_state());
        }
    }
}

} // namespace piejam::runtime
