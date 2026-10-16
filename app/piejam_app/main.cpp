// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/algorithm/transform_to_vector.h>
#include <piejam/audio/engine/processor.h>
#include <piejam/audio/sound_card_manager.h>
#include <piejam/fx_modules/init.h>
#include <piejam/gui/init.h>
#include <piejam/gui/model/Info.h>
#include <piejam/gui/model/Root.h>
#include <piejam/gui/qt_log.h>
#include <piejam/ladspa/instance_manager_processor_factory.h>
#include <piejam/ladspa/plugin.h>
#include <piejam/midi/device_manager.h>
#include <piejam/midi/device_update.h>
#include <piejam/midi/input_event_handler.h>
#include <piejam/network_manager/network_controller.h>
#include <piejam/network_manager/nfs_client.h>
#include <piejam/network_manager/nfs_server.h>
#include <piejam/network_manager/wifi_manager.h>
#include <piejam/range/iota.h>
#include <piejam/redux/middleware_factory.h>
#include <piejam/redux/queueing_middleware.h>
#include <piejam/redux/store.h>
#include <piejam/redux/subscriber.h>
#include <piejam/redux/thread_delegate_middleware.h>
#include <piejam/redux/thunk_middleware.h>
#include <piejam/runtime/actions/audio_engine_sync.h>
#include <piejam/runtime/actions/load_app_config.h>
#include <piejam/runtime/actions/recording.h>
#include <piejam/runtime/actions/refresh_midi_devices.h>
#include <piejam/runtime/actions/refresh_sound_cards.h>
#include <piejam/runtime/actions/save_app_config.h>
#include <piejam/runtime/actions/scan_ladspa_fx_plugins.h>
#include <piejam/runtime/actions/session_actions.h>
#include <piejam/runtime/actions/shutdown.h>
#include <piejam/runtime/audio_engine_middleware.h>
#include <piejam/runtime/exception_middleware.h>
#include <piejam/runtime/ladspa_fx_middleware.h>
#include <piejam/runtime/locations.h>
#include <piejam/runtime/midi_control_middleware.h>
#include <piejam/runtime/midi_input_controller.h>
#include <piejam/runtime/persistence_middleware.h>
#include <piejam/runtime/recorder_middleware.h>
#include <piejam/runtime/state.h>
#include <piejam/runtime/store.h>
#include <piejam/runtime/subscriber.h>
#include <piejam/runtime/ui/action.h>
#include <piejam/runtime/ui/thunk_action.h>
#include <piejam/system/avg_cpu_load_tracker.h>
#include <piejam/system/cpu_temp.h>
#include <piejam/system/disk_usage.h>
#include <piejam/system/memory.h>
#include <piejam/thread/affinity.h>

#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTimer>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <boost/core/demangle.hpp>
#include <boost/polymorphic_cast.hpp>

#include <filesystem>

namespace
{

auto
create_directories_if_missing(piejam::runtime::locations const& locs)
{
    auto create_directory_if_missing = [](auto const& dir) {
        BOOST_ASSERT(!dir.empty());

        try
        {
            if (!std::filesystem::exists(dir))
            {
                std::filesystem::create_directories(dir);
            }
        }
        catch (std::exception const& err)
        {
            spdlog::error(
                "could not create directory: {} - {}",
                dir.string(),
                err.what());
        }
    };

    create_directory_if_missing(locs.config_dir);
    create_directory_if_missing(locs.sessions_dir);
    create_directory_if_missing(locs.recordings_dir);

    return;
}

constexpr int realtime_priority = 96;

struct QtThreadDelegator
{
    template <std::invocable F>
    void operator()(F&& f)
    {
        QMetaObject::invokeMethod(app, std::forward<F>(f));
    }

    QGuiApplication* app{};
};

} // namespace

namespace piejam::runtime::ui
{

auto
as_thunk_action(action const& a) -> thunk_action<runtime::state> const*
{
    return dynamic_cast<thunk_action<runtime::state> const*>(&a);
}

} // namespace piejam::runtime::ui

auto
main(int argc, char* argv[]) -> int
{
    using namespace piejam;

    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

    std::size_t const hw_threads = std::thread::hardware_concurrency();

    piejam::gui::init();
    piejam::fx_modules::init();

    gui::qt_log::install_handler();
    spdlog::set_level(spdlog::level::level_enum::debug);

    if (auto err = piejam::system::mlockall())
    {
        spdlog::warn("could not lock memory: {}", err.message());
    }

    runtime::locations locs{
        .home_dir = QStandardPaths::writableLocation(
                        QStandardPaths::StandardLocation::HomeLocation)
                        .toStdString(),
        .config_dir = QStandardPaths::writableLocation(
                          QStandardPaths::StandardLocation::ConfigLocation)
                          .toStdString(),
    };

    create_directories_if_missing(locs);

    // Remove stdout sink — on EGLFS stdout goes to /dev/console
    // which shares a kernel lock with DRM; writing to it from any
    // thread while Qt renders causes a deadlock.
    spdlog::default_logger()->sinks().clear();
    spdlog::default_logger()->sinks().push_back(
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            locs.log_file,
            true));

    // Flush log every 5 seconds so logs survive unexpected power loss
    spdlog::flush_every(std::chrono::seconds(5));

    QGuiApplication app(argc, argv);

    QQuickStyle::setStyle("Material");

    auto midi_device_manager = midi::make_device_manager();
    ladspa::instance_manager_processor_factory ladspa_manager;

    using middleware_factory =
        redux::middleware_factory<runtime::state, runtime::action>;

    runtime::store store(
        [](runtime::state& st, runtime::action const& a) -> void {
            if (auto const* const ra =
                    dynamic_cast<runtime::reducible_action const*>(&a);
                ra)
            {
                ra->reduce(st);
            }
            else
            {
                auto const action_name =
                    boost::core::demangle(typeid(a).name());
                spdlog::warn("non-reducible action detected: {}", action_name);
            }
        },
        runtime::make_initial_state());

    store.apply_middleware(
        middleware_factory::make<runtime::persistence_middleware>(
            locs.home_dir,
            locs.sessions_dir));

    store.apply_middleware(
        middleware_factory::make<runtime::recorder_middleware>(
            locs.recordings_dir));

    auto network_ctrl =
        std::make_shared<network_manager::network_controller>();
    auto wifi_mgr = std::make_shared<network_manager::wifi_manager>();
    auto nfs_srv = std::make_shared<network_manager::nfs_server>();
    auto nfs_cli = std::make_shared<network_manager::nfs_client>(
        locs.config_dir / "nfs_mounts.json");

    auto audio_workers = piejam::algorithm::transform_to_vector(
        piejam::range::iota(hw_threads - 1),
        [=](std::size_t const i) {
            std::size_t const cpu = (2 + i) % hw_threads;
            return thread::configuration{
                .affinity = cpu,
                .realtime_priority = realtime_priority,
                .name = std::format("audio_worker_{}", i)};
        });

    store.apply_middleware(
        middleware_factory::make<runtime::audio_engine_middleware>(
            thread::configuration{
                .affinity = hw_threads > 1 ? 1 : 0,
                .realtime_priority = realtime_priority,
                .name = "audio_main"},
            audio_workers,
            audio::get_default_sound_card_manager(),
            ladspa_manager,
            runtime::make_midi_input_controller(*midi_device_manager)));

    store.apply_middleware(
        middleware_factory::make<runtime::midi_control_middleware>(
            [&midi_device_manager]() {
                return midi_device_manager->update_devices();
            }));

    store.apply_middleware(
        middleware_factory::make<runtime::ladspa_fx_middleware>(
            ladspa_manager));

    store.apply_middleware(middleware_factory::make<redux::thunk_middleware>());

    store.apply_middleware(
        middleware_factory::make<
            redux::queueing_middleware<runtime::action>>());

    store.apply_middleware(
        middleware_factory::make<
            redux::thread_delegate_middleware<QtThreadDelegator>>(
            std::this_thread::get_id(),
            QtThreadDelegator{&app}));

    store.apply_middleware(
        middleware_factory::make<
            runtime::exception_middleware<runtime::action>>());

    runtime::subscriber state_change_subscriber(
        [&store]() -> runtime::state const& { return store.state(); });

    store.subscribe([&state_change_subscriber](auto const& state) {
        state_change_subscriber.notify(state);
    });

    store.dispatch(runtime::actions::scan_ladspa_fx_plugins("/usr/lib/ladspa"));

    store.dispatch(runtime::actions::refresh_sound_cards{});
    store.dispatch(runtime::actions::refresh_midi_devices{});
    store.dispatch(runtime::actions::load_app_config(locs.config_file));
    store.dispatch(runtime::actions::initiate_startup_session{});

    gui::model::Root rootModel(
        runtime::state_access{store, state_change_subscriber},
        locs.sessions_dir,
        network_ctrl,
        wifi_mgr,
        nfs_srv,
        nfs_cli);

    QQmlApplicationEngine engine;
    engine.addImportPath("qrc:/");

    engine.setInitialProperties({
        {"model", QVariant::fromValue(&rootModel)},
    });
    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    if (engine.rootObjects().isEmpty())
    {
        qFatal("Could not load Main.qml");
        return 1;
    }

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::quit,
        &QGuiApplication::quit);

    system::avg_cpu_load_tracker avg_cpu_load{hw_threads};

    // slow updates
    auto slow_updates_timer = [&] {
        auto timer = new QTimer(&app);
        QObject::connect(timer, &QTimer::timeout, [&]() {
            store.dispatch(runtime::actions::refresh_midi_devices{});

            rootModel.info()->setCpuTemp(system::cpu_temp());

            avg_cpu_load.update();
            auto cpu_load_per_core = avg_cpu_load.per_core();
            rootModel.info()->setCpuLoad(
                QList<float>(
                    cpu_load_per_core.begin(),
                    cpu_load_per_core.end()));

            rootModel.info()->setDiskUsage(
                static_cast<int>(
                    std::round(system::disk_usage(locs.home_dir) * 100)));
        });
        timer->start(std::chrono::seconds(1));
        return timer;
    }();

    // gui frame updates
    auto gui_frame_update_timer = [&] {
        auto timer = new QTimer(&app);
        QObject::connect(timer, &QTimer::timeout, [&]() {
            store.dispatch(runtime::actions::request_audio_engine_sync{});
        });
        timer->start(std::chrono::milliseconds(16));
        return timer;
    }();

    auto const app_exec_result = app.exec();

    slow_updates_timer->stop();
    gui_frame_update_timer->stop();

    store.dispatch(runtime::actions::save_app_config{locs.config_file});
    store.dispatch(runtime::actions::shutdown{});

    return app_exec_result;
}
