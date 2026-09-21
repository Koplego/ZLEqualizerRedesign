// Copyright (C) 2026 - zsliu98
// Glass EQ contextual band workspace

#include "band_hub_panel.hpp"

namespace zlpanel {
    void BandHubPanel::resized() {
        auto surface = expandedSurface().toNearestInt().reduced(getPaddingSize(base_.getFontSize()) + 3);
        const auto p = getPaddingSize(base_.getFontSize());
        const auto b = getButtonSize(base_.getFontSize());

        auto header = surface.removeFromTop(b);
        title_bound_ = header.removeFromLeft(juce::jmax(b * 3, juce::roundToInt(base_.getFontSize() * 6.1f)));
        collapse_button_.setBounds(header.removeFromRight(b));
        header.removeFromRight(p / 3);
        const auto dynW = juce::jmax(b * 3, juce::roundToInt(base_.getFontSize() * 6.3f));
        dynamic_button_.setBounds(header.removeFromRight(dynW));
        header.removeFromRight(p / 3);
        const auto channelW = juce::jmax(b * 2, juce::roundToInt(base_.getFontSize() * 5.1f));
        channel_box_.setBounds(header.removeFromRight(channelW));

        surface.removeFromTop(juce::jmax(2, p / 3));
        auto nav = surface.removeFromTop(juce::jmax(b * 3 / 4, juce::roundToInt(base_.getFontSize() * 1.20f)));
        const auto gap = juce::jmax(3, p / 3);
        const auto tabW = (nav.getWidth() - 2 * gap) / 3;
        dynamics_page_button_.setBounds(nav.removeFromLeft(tabW)); nav.removeFromLeft(gap);
        detector_page_button_.setBounds(nav.removeFromLeft(tabW)); nav.removeFromLeft(gap);
        sidechain_page_button_.setBounds(nav);

        surface.removeFromTop(juce::jmax(3, p / 2));
        page_title_bound_ = surface.removeFromTop(juce::jmax(13, juce::roundToInt(base_.getFontSize() * 1.05f)));
        surface.removeFromTop(juce::jmax(2, p / 4));
        const auto labelH = juce::jmax(9, juce::roundToInt(base_.getFontSize() * .62f));
        auto labels = surface.removeFromTop(labelH);
        auto values = surface.removeFromTop(b);
        const auto cellGap = juce::jmax(4, p / 2);
        const auto cellW = (values.getWidth() - 3 * cellGap) / 4;
        for (size_t i = 0; i < 4; ++i) {
            primary_label_bounds_[i] = labels.removeFromLeft(cellW);
            detail_label_bounds_[i] = primary_label_bounds_[i];
            if (i < 3) labels.removeFromLeft(cellGap);
        }

        threshold_slider_.setBounds(values.removeFromLeft(cellW)); values.removeFromLeft(cellGap);
        range_slider_.setBounds(values.removeFromLeft(cellW)); values.removeFromLeft(cellGap);
        attack_slider_.setBounds(values.removeFromLeft(cellW)); values.removeFromLeft(cellGap);
        release_slider_.setBounds(values);

        auto detectorValues = threshold_slider_.getBounds().getUnion(range_slider_.getBounds())
            .getUnion(attack_slider_.getBounds()).getUnion(release_slider_.getBounds());
        auto dv = detectorValues;
        knee_slider_.setBounds(dv.removeFromLeft(cellW)); dv.removeFromLeft(cellGap);
        rms_length_slider_.setBounds(dv.removeFromLeft(cellW)); dv.removeFromLeft(cellGap);
        rms_mix_slider_.setBounds(dv.removeFromLeft(cellW)); dv.removeFromLeft(cellGap);
        smooth_slider_.setBounds(dv);

        auto sv = detectorValues;
        side_type_box_.setBounds(sv.removeFromLeft(cellW)); sv.removeFromLeft(cellGap);
        side_order_box_.setBounds(sv.removeFromLeft(cellW)); sv.removeFromLeft(cellGap);
        side_freq_slider_.setBounds(sv.removeFromLeft(cellW)); sv.removeFromLeft(cellGap);
        side_q_slider_.setBounds(sv);

        surface.removeFromTop(juce::jmax(4, p / 2));
        auto flags = surface.removeFromTop(juce::jmax(b * 3 / 4, juce::roundToInt(base_.getFontSize() * 1.18f)));
        const auto flagGap = juce::jmax(4, p / 2);
        const auto flagW3 = (flags.getWidth() - 2 * flagGap) / 3;
        learn_button_.setBounds(flags.removeFromLeft(flagW3)); flags.removeFromLeft(flagGap);
        relative_button_.setBounds(flags.removeFromLeft(flagW3)); flags.removeFromLeft(flagGap);
        dyn_bypass_button_.setBounds(flags);

        auto sideFlags = learn_button_.getBounds().getUnion(relative_button_.getBounds()).getUnion(dyn_bypass_button_.getBounds());
        const auto sideW = (sideFlags.getWidth() - flagGap) / 2;
        side_link_button_.setBounds(sideFlags.removeFromLeft(sideW)); sideFlags.removeFromLeft(flagGap);
        side_swap_button_.setBounds(sideFlags);

        updateVisibility();
    }

    bool BandHubPanel::hitTest(const int x, const int y) {
        if (suppressed_ || attached_band_ >= zlp::kBandNum) return false;
        return currentSurface().contains(static_cast<float>(x), static_cast<float>(y));
    }

    void BandHubPanel::mouseUp(const juce::MouseEvent& event) {
        if (!expanded_ && collapsedSurface().contains(event.getPosition().toFloat())) setExpanded(true);
    }

    int BandHubPanel::getIdealWidth() const {
        return juce::roundToInt(base_.getFontSize() * 42.f);
    }

    int BandHubPanel::getIdealHeight() const {
        return juce::roundToInt(base_.getFontSize() * 12.2f);
    }

    void BandHubPanel::setExpanded(const bool expanded) {
        expanded_ = expanded;
        reveal_target_ = expanded ? 1.f : 0.f;
        startTimerHz(60);
        updateVisibility();
        repaint();
    }

    void BandHubPanel::setPage(const Page page) {
        page_ = page;
        dynamics_page_button_.getButton().setToggleState(page == Page::dynamics, juce::dontSendNotification);
        detector_page_button_.getButton().setToggleState(page == Page::detector, juce::dontSendNotification);
        sidechain_page_button_.getButton().setToggleState(page == Page::sidechain, juce::dontSendNotification);
        updateVisibility();
        repaint();
    }
}
