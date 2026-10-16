// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/Log.h>

#include <piejam/gui/model/StringList.h>

#include <piejam/log/generic_log_sink.h>

#include <spdlog/spdlog.h>

#include <boost/polymorphic_cast.hpp>

namespace piejam::gui::model
{

Log::Log(runtime::state_access const& state_access)
    : CompositeSubscribableModel{state_access}
    , m_logMessages{&addQObject<StringList>()}
{
    auto sink = std::make_shared<log::generic_log_sink_mt>(
        [this](spdlog::details::log_msg const& msg) {
            auto& logMessages =
                boost::polymorphic_downcast<StringList&>(*m_logMessages);
            auto qtMsg = QString::fromStdString(
                std::format(
                    "[{}] {}",
                    spdlog::level::to_string_view(msg.level),
                    msg.payload));
            logMessages.add(logMessages.size(), qtMsg);
        },
        []() {});

    // The Info page lists what a user may act on. Debug output (such as
    // Qt's scene-graph details) still goes to the log file.
    sink->set_level(spdlog::level::info);

    spdlog::default_logger()->sinks().push_back(std::move(sink));
}

void
Log::onSubscribe()
{
}

} // namespace piejam::gui::model
