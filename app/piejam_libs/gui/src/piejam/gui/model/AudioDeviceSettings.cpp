// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/gui/model/AudioDeviceSettings.h>

#include <piejam/gui/model/SoundCardInfo.h>
#include <piejam/gui/model/StringList.h>
#include <piejam/gui/model/ValueListModel.h>

#include <piejam/gui/ListModelEditScriptProcessor.h>

#include <piejam/algorithm/index_of.h>
#include <piejam/runtime/actions/initiate_sound_card_selection.h>
#include <piejam/runtime/actions/scan_for_sound_cards.h>
#include <piejam/runtime/actions/select_period_size.h>
#include <piejam/runtime/actions/select_sample_rate.h>
#include <piejam/runtime/selectors.h>
#include <piejam/runtime/ui/thunk_action.h>

#include <boost/polymorphic_cast.hpp>

namespace piejam::gui::model
{

namespace
{

class SoundCardInfoList : public ValueListModel<SoundCardInfo>
{
    using Base = ValueListModel<SoundCardInfo>;

public:
    using Base::Base;

private:
    auto itemToString(SoundCardInfo const& item) const -> QString override
    {
        return item.name;
    }
};

} // namespace

struct AudioDeviceSettings::Impl
{
    audio::sample_rates_t sample_rates;
    audio::period_sizes_t period_sizes;
    std::vector<runtime::selectors::sound_card_info> sound_cards;
};

AudioDeviceSettings::AudioDeviceSettings(
    runtime::state_access const& state_access)
    : CompositeSubscribableModel(state_access)
    , m_impl{make_pimpl<Impl>()}
    , m_soundCards{&addQObject<SoundCardInfoList>()}
    , m_selectedSoundCardIndex{-1}
    , m_sampleRates{&addQObject<StringList>()}
{
}

void
AudioDeviceSettings::onSubscribe()
{
    namespace selectors = runtime::selectors;

    observe(
        selectors::select_sound_card,
        [this](selectors::sound_card_choice const& choice) {
            algorithm::apply_edit_script(
                algorithm::edit_script(m_impl->sound_cards, choice.available),
                ListModelEditScriptProcessor{
                    boost::polymorphic_downcast<SoundCardInfoList&>(
                        *m_soundCards),
                    [](runtime::selectors::sound_card_info const& info) {
                        return SoundCardInfo{
                            .name = QString::fromStdString(info.name),
                            .numIns = static_cast<int>(info.num_ins),
                            .numOuts = static_cast<int>(info.num_outs),
                        };
                    }});
            m_impl->sound_cards = choice.available;
            setSelectedSoundCardIndex(static_cast<int>(choice.current));
        });

    observe(
        selectors::select_sample_rate,
        [this](selectors::sample_rate_choice const& sample_rate) {
            auto const index =
                algorithm::index_of(sample_rate.available, sample_rate.current);

            algorithm::apply_edit_script(
                algorithm::edit_script(
                    m_impl->sample_rates,
                    sample_rate.available),
                ListModelEditScriptProcessor{
                    boost::polymorphic_downcast<StringList&>(*m_sampleRates),
                    [](auto const sr) { return QString::number(sr.value()); }});
            m_impl->sample_rates = sample_rate.available;

            setSelectedSampleRate(static_cast<int>(index));
        });

    observe(
        selectors::select_period_size,
        [this](selectors::period_size_choice const& period_size) {
            auto const index =
                algorithm::index_of(period_size.available, period_size.current);
            m_impl->period_sizes = period_size.available;
            setPeriodSizesCount(static_cast<int>(period_size.available.size()));
            setSelectedPeriodSizeIndex(static_cast<int>(index));
            setSelectedPeriodSize(
                static_cast<int>(period_size.current.value()));
        });

    observe(selectors::select_buffer_latency, [this](float const x) {
        setBufferLatency(static_cast<double>(x));
    });

    requestUpdates(std::chrono::seconds{1}, [this]() {
        refreshSoundCardLists();
    });
}

void
AudioDeviceSettings::refreshSoundCardLists()
{
    dispatch(runtime::actions::scan_for_sound_cards());
}

void
AudioDeviceSettings::selectSoundCard(unsigned const index)
{
    runtime::actions::initiate_sound_card_selection action;
    action.index = index;
    dispatch(action);
}

void
AudioDeviceSettings::selectSampleRate(unsigned const index)
{
    runtime::actions::select_sample_rate action;
    action.index = index;
    dispatch(action);
}

void
AudioDeviceSettings::selectPeriodSize(unsigned const index)
{
    runtime::actions::select_period_size action;
    action.index = index;
    dispatch(action);
}

} // namespace piejam::gui::model
