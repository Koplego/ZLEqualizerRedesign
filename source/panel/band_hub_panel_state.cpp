// Copyright (C) 2026 - zsliu98
// Glass EQ contextual band workspace

#include "band_hub_panel.hpp"
#include <array>

namespace zlpanel {
    void BandHubPanel::clearAttachments() {
        channel_attachment_.reset(); threshold_attachment_.reset(); range_attachment_.reset();
        attack_attachment_.reset(); release_attachment_.reset(); knee_attachment_.reset();
        rms_length_attachment_.reset(); rms_mix_attachment_.reset(); smooth_attachment_.reset();
        learn_attachment_.reset(); relative_attachment_.reset(); dyn_bypass_attachment_.reset();
        side_type_attachment_.reset(); side_order_attachment_.reset(); side_freq_attachment_.reset();
        side_q_attachment_.reset(); side_link_attachment_.reset(); side_swap_attachment_.reset();
        dynamic_on_ptr_ = nullptr;
    }

    void BandHubPanel::updateBand() {
        const auto band = base_.getSelectedBand();
        if (band >= zlp::kBandNum) {
            clearAttachments();
            attached_band_ = zlp::kBandNum;
            expanded_ = false;
            reveal_ = reveal_target_ = 0.f;
            stopTimer();
            setVisible(false);
            return;
        }

        const auto firstSelection = attached_band_ >= zlp::kBandNum;
        const auto changed = band != attached_band_;
        attached_band_ = band;
        const auto s = std::to_string(band);
        clearAttachments();

        channel_attachment_ = std::make_unique<zlgui::attachment::ComboBoxAttachment<true>>(
            channel_box_.getBox(), p_ref_.parameters_, zlp::PLRMode::kID + s, updater_);
        threshold_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
            threshold_slider_.getSlider(), p_ref_.parameters_, zlp::PThreshold::kID + s, updater_);
        range_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
            range_slider_.getSlider(), p_ref_.parameters_, zlp::PTargetGain::kID + s, updater_);
        attack_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
            attack_slider_.getSlider(), p_ref_.parameters_, zlp::PAttack::kID + s, updater_);
        release_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
            release_slider_.getSlider(), p_ref_.parameters_, zlp::PRelease::kID + s, updater_);
        knee_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
            knee_slider_.getSlider(), p_ref_.parameters_, zlp::PKneeW::kID + s, updater_);
        rms_length_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
            rms_length_slider_.getSlider(), p_ref_.parameters_, zlp::PDynamicRMSLength::kID + s, updater_);
        rms_mix_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
            rms_mix_slider_.getSlider(), p_ref_.parameters_, zlp::PDynamicRMSMix::kID + s, updater_);
        smooth_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
            smooth_slider_.getSlider(), p_ref_.parameters_, zlp::PDynamicSmooth::kID + s, updater_);
        learn_attachment_ = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(
            learn_button_.getButton(), p_ref_.parameters_, zlp::PDynamicLearn::kID + s, updater_, juce::dontSendNotification);
        relative_attachment_ = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(
            relative_button_.getButton(), p_ref_.parameters_, zlp::PDynamicRelative::kID + s, updater_, juce::dontSendNotification);
        dyn_bypass_attachment_ = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(
            dyn_bypass_button_.getButton(), p_ref_.parameters_, zlp::PDynamicBypass::kID + s, updater_, juce::dontSendNotification);
        side_type_attachment_ = std::make_unique<zlgui::attachment::ComboBoxAttachment<true>>(
            side_type_box_.getBox(), p_ref_.parameters_, zlp::PSideFilterType::kID + s, updater_);
        side_order_attachment_ = std::make_unique<zlgui::attachment::ComboBoxAttachment<true>>(
            side_order_box_.getBox(), p_ref_.parameters_, zlp::PSideOrder::kID + s, updater_);
        side_freq_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
            side_freq_slider_.getSlider(), p_ref_.parameters_, zlp::PSideFreq::kID + s, updater_);
        side_q_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
            side_q_slider_.getSlider(), p_ref_.parameters_, zlp::PSideQ::kID + s, updater_);
        side_link_attachment_ = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(
            side_link_button_.getButton(), p_ref_.parameters_, zlp::PSideLink::kID + s, updater_, juce::dontSendNotification);
        side_swap_attachment_ = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(
            side_swap_button_.getButton(), p_ref_.parameters_, zlp::PSideSwap::kID + s, updater_, juce::dontSendNotification);

        dynamic_on_ptr_ = p_ref_.parameters_.getRawParameterValue(zlp::PDynamicON::kID + s);
        updater_.updateComponents();
        syncDynamicState();
        setVisible(!suppressed_);

        if (changed) page_ = Page::dynamics;
        if (firstSelection) {
            expanded_ = false;
            reveal_ = reveal_target_ = 0.f;
            stopTimer();
        }

        updateVisibility();
        repaint();
    }

    void BandHubPanel::syncDynamicState() {
        dynamic_on_ = dynamic_on_ptr_ != nullptr && dynamic_on_ptr_->load(std::memory_order::relaxed) > .5f;
        dynamic_button_.getButton().setToggleState(dynamic_on_, juce::dontSendNotification);

        bool gainCapable = false;
        if (attached_band_ < zlp::kBandNum) {
            if (const auto* type = p_ref_.parameters_.getRawParameterValue(
                    zlp::PFilterType::kID + std::to_string(attached_band_))) {
                const auto t = static_cast<int>(std::round(type->load(std::memory_order::relaxed)));
                gainCapable = t == static_cast<int>(zldsp::filter::kPeak)
                    || t == static_cast<int>(zldsp::filter::kLowShelf)
                    || t == static_cast<int>(zldsp::filter::kHighShelf)
                    || t == static_cast<int>(zldsp::filter::kTiltShelf)
                    || t == static_cast<int>(zldsp::filter::kFlatTilt)
                    || t == static_cast<int>(zldsp::filter::kFlatGain);
            }
        }
        dynamic_button_.setAlpha(gainCapable ? 1.f : .38f);
        dynamic_button_.setInterceptsMouseClicks(true, gainCapable);
    }

    void BandHubPanel::updateVisibility() {
        const auto childrenVisible = !suppressed_ && attached_band_ < zlp::kBandNum && reveal_ > .18f;
        const auto controlsAlpha = juce::jlimit(0.f, 1.f, (reveal_ - .14f) / .70f);

        const std::array<juce::Component*, 6> headerComponents{
            &collapse_button_, &dynamic_button_, &dynamics_page_button_,
            &detector_page_button_, &sidechain_page_button_, &channel_box_};
        for (auto* c : headerComponents) {
            c->setVisible(childrenVisible);
            c->setAlpha(controlsAlpha);
        }

        const auto dyn = childrenVisible && dynamic_on_ && reveal_ > .30f;
        const auto showDynamics = dyn && page_ == Page::dynamics;
        const std::array<juce::Component*, 4> dynamicsComponents{
            &threshold_slider_, &range_slider_, &attack_slider_, &release_slider_};
        for (auto* c : dynamicsComponents) {
            c->setVisible(showDynamics);
            c->setAlpha(controlsAlpha);
        }

        const auto showDetector = dyn && page_ == Page::detector;
        const std::array<juce::Component*, 7> detectorComponents{
            &knee_slider_, &rms_length_slider_, &rms_mix_slider_, &smooth_slider_,
            &learn_button_, &relative_button_, &dyn_bypass_button_};
        for (auto* c : detectorComponents) {
            c->setVisible(showDetector);
            c->setAlpha(controlsAlpha);
        }

        const auto showSide = dyn && page_ == Page::sidechain;
        const std::array<juce::Component*, 6> sidechainComponents{
            &side_type_box_, &side_order_box_, &side_freq_slider_, &side_q_slider_,
            &side_link_button_, &side_swap_button_};
        for (auto* c : sidechainComponents) {
            c->setVisible(showSide);
            c->setAlpha(controlsAlpha);
        }

        dynamics_page_button_.getButton().setToggleState(page_ == Page::dynamics, juce::dontSendNotification);
        detector_page_button_.getButton().setToggleState(page_ == Page::detector, juce::dontSendNotification);
        sidechain_page_button_.getButton().setToggleState(page_ == Page::sidechain, juce::dontSendNotification);
    }

    void BandHubPanel::repaintCallbackSlow() {
        if (attached_band_ >= zlp::kBandNum) return;
        updater_.updateComponents();
        syncDynamicState();
        for (auto* s : {&threshold_slider_, &range_slider_, &attack_slider_, &release_slider_, &knee_slider_,
                        &rms_length_slider_, &rms_mix_slider_, &smooth_slider_, &side_freq_slider_, &side_q_slider_})
            s->updateDisplayValue();
        updateVisibility();
        repaint();
    }

    void BandHubPanel::setSuppressed(const bool suppressed) {
        suppressed_ = suppressed;
        setVisible(!suppressed_ && attached_band_ < zlp::kBandNum);
        updateVisibility();
    }

    void BandHubPanel::timerCallback() {
        reveal_ += (reveal_target_ - reveal_) * .28f;
        if (std::abs(reveal_target_ - reveal_) < .008f) {
            reveal_ = reveal_target_;
            stopTimer();
        }
        updateVisibility();
        repaint();
    }
}
