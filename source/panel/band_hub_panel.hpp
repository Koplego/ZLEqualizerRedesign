// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// Glass EQ contextual Band Hub.  The graph bubble owns only fast static-EQ
// edits; deeper per-band controls live here in a stable, bottom-anchored surface.

#pragma once

#include "../PluginProcessor.hpp"
#include "../gui/gui.hpp"
#include "helper/helper.hpp"

#include <array>
#include <memory>

namespace zlpanel {
    class BandHubPanel final : public juce::Component,
                               private juce::Timer,
                               private juce::ValueTree::Listener {
    public:
        explicit BandHubPanel(PluginProcessor& p, zlgui::UIBase& base);
        ~BandHubPanel() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

        void updateBand();
        void repaintCallbackSlow();

        void setExpanded(bool should_expand);
        bool isExpanded() const noexcept { return expanded_; }

    private:
        enum class Page { dynamics, detector, sidechain, actions };

        static constexpr std::array kCopyIDs{
            zlp::PFilterStatus::kID,
            zlp::PFilterType::kID,
            zlp::POrder::kID, zlp::PLRMode::kID,
            zlp::PFreq::kID,
            zlp::PGain::kID, zlp::PTargetGain::kID,
            zlp::PQ::kID,
            zlp::PDynamicON::kID, zlp::PDynamicLearn::kID,
            zlp::PDynamicBypass::kID, zlp::PDynamicRelative::kID,
            zlp::PSideSwap::kID, zlp::PSideLink::kID,
            zlp::PThreshold::kID, zlp::PKneeW::kID,
            zlp::PAttack::kID, zlp::PRelease::kID,
            zlp::PDynamicRMSLength::kID, zlp::PDynamicRMSMix::kID,
            zlp::PDynamicSmooth::kID,
            zlp::PSideFilterType::kID, zlp::PSideOrder::kID,
            zlp::PSideFreq::kID, zlp::PSideQ::kID
        };

        PluginProcessor& p_ref_;
        zlgui::UIBase& base_;
        zlgui::attachment::ComponentUpdater updater_{};

        size_t band_{zlp::kBandNum};
        Page page_{Page::dynamics};
        bool expanded_{false};
        float reveal_{0.f};
        float reveal_target_{0.f};

        std::atomic<float>* dynamic_on_ptr_{nullptr};
        std::atomic<float>* filter_type_ptr_{nullptr};

        // Stable hub header.
        zlgui::combobox::CompactCombobox channel_box_;
        std::unique_ptr<zlgui::attachment::ComboBoxAttachment<true>> channel_attach_;

        zlgui::button::ClickTextButton dynamic_toggle_;
        std::unique_ptr<zlgui::attachment::ButtonAttachment<true>> dynamic_attach_;

        zlgui::button::ClickTextButton expand_button_;
        zlgui::button::ClickTextButton dynamics_page_button_;
        zlgui::button::ClickTextButton detector_page_button_;
        zlgui::button::ClickTextButton sidechain_page_button_;
        zlgui::button::ClickTextButton actions_page_button_;

        // Dynamic primary controls — these intentionally mirror the approved mockup.
        zlgui::slider::CompactLinearSlider<false, false, false> threshold_slider_;
        zlgui::slider::CompactLinearSlider<false, false, false> range_slider_;
        zlgui::slider::CompactLinearSlider<false, false, false> attack_slider_;
        zlgui::slider::CompactLinearSlider<false, false, false> release_slider_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> threshold_attach_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> range_attach_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> attack_attach_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> release_attach_;

        // Detector / advanced dynamics.
        zlgui::slider::CompactLinearSlider<false, false, false> knee_slider_;
        zlgui::slider::CompactLinearSlider<false, false, false> rms_length_slider_;
        zlgui::slider::CompactLinearSlider<false, false, false> rms_mix_slider_;
        zlgui::slider::CompactLinearSlider<false, false, false> smooth_slider_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> knee_attach_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> rms_length_attach_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> rms_mix_attach_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> smooth_attach_;

        zlgui::button::ClickTextButton learn_button_;
        zlgui::button::ClickTextButton relative_button_;
        zlgui::button::ClickTextButton dynamic_bypass_button_;
        std::unique_ptr<zlgui::attachment::ButtonAttachment<true>> learn_attach_;
        std::unique_ptr<zlgui::attachment::ButtonAttachment<true>> relative_attach_;
        std::unique_ptr<zlgui::attachment::ButtonAttachment<true>> dynamic_bypass_attach_;

        // Dynamic sidechain.
        zlgui::combobox::CompactCombobox side_type_box_;
        zlgui::combobox::CompactCombobox side_order_box_;
        zlgui::slider::CompactLinearSlider<false, false, false> side_freq_slider_;
        zlgui::slider::CompactLinearSlider<false, false, false> side_q_slider_;
        std::unique_ptr<zlgui::attachment::ComboBoxAttachment<true>> side_type_attach_;
        std::unique_ptr<zlgui::attachment::ComboBoxAttachment<true>> side_order_attach_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> side_freq_attach_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> side_q_attach_;

        zlgui::button::ClickTextButton side_link_button_;
        zlgui::button::ClickTextButton side_swap_button_;
        zlgui::button::ClickTextButton external_button_;
        std::unique_ptr<zlgui::attachment::ButtonAttachment<true>> side_link_attach_;
        std::unique_ptr<zlgui::attachment::ButtonAttachment<true>> side_swap_attach_;
        zlgui::attachment::ButtonAttachment<true> external_attach_;

        // Actions.  Context-click remains a fast shortcut; this page is the discoverable home.
        zlgui::button::ClickTextButton solo_button_;
        zlgui::button::ClickTextButton invert_button_;
        zlgui::button::ClickTextButton split_lr_button_;
        zlgui::button::ClickTextButton split_ms_button_;
        zlgui::button::ClickTextButton copy_button_;
        zlgui::button::ClickTextButton paste_button_;
        zlgui::button::ClickTextButton delete_button_;

        juce::Rectangle<float> hub_surface_{};
        juce::Rectangle<int> identity_bound_{};
        juce::Rectangle<int> page_title_bound_{};
        std::array<juce::Rectangle<int>, 4> value_label_bounds_{};

        void timerCallback() override;
        void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override;

        void setPage(Page page);
        void updatePageVisibility();
        void stylePill(zlgui::button::ClickTextButton& button, bool quiet = false);
        void clearBandAttachments();
        void layoutForReveal();

        void invertGain();
        void splitBand(zlp::FilterStereo stereo1, zlp::FilterStereo stereo2);
        void copyBand();
        void pasteBand();
        void deleteBand();
    };
}
