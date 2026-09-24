// Copyright (C) 2026 - zsliu98
// Glass EQ contextual band workspace

#include "band_hub_panel.hpp"

namespace zlpanel {
    void BandHubPanel::resized() {
        const auto font = base_.getFontSize();
        const auto pad = juce::jmax(4, juce::roundToInt(font * .34f));
        const auto gap = juce::jmax(3, juce::roundToInt(font * .24f));
        const auto rowH = juce::jmax(20, juce::roundToInt(font * 1.42f));

        auto surface = expandedSurface().toNearestInt().reduced(pad);
        auto header = surface.removeFromTop(rowH);

        title_bound_ = header.removeFromLeft(juce::roundToInt(font * 5.35f));
        header.removeFromLeft(gap);

        collapse_button_.setBounds(header.removeFromRight(rowH));
        header.removeFromRight(gap);

        const auto dynW = juce::roundToInt(font * 4.7f);
        dynamic_button_.setBounds(header.removeFromRight(dynW));
        header.removeFromRight(gap);

        const auto channelW = juce::roundToInt(font * 4.05f);
        channel_box_.setBounds(header.removeFromRight(channelW));
        header.removeFromRight(gap);

        const auto tabGap = juce::jmax(2, gap - 1);
        const auto tabW = juce::jmax(1, (header.getWidth() - 2 * tabGap) / 3);
        dynamics_page_button_.setBounds(header.removeFromLeft(tabW));
        header.removeFromLeft(tabGap);
        detector_page_button_.setBounds(header.removeFromLeft(tabW));
        header.removeFromLeft(tabGap);
        sidechain_page_button_.setBounds(header);

        surface.removeFromTop(gap);
        page_title_bound_ = {};

        const auto labelH = juce::jmax(8, juce::roundToInt(font * .54f));
        const auto controlH = juce::jmax(18, juce::roundToInt(font * 1.28f));
        const auto contentH = labelH + controlH;
        auto content = surface.withHeight(juce::jmin(surface.getHeight(), contentH));
        if (surface.getHeight() > contentH)
            content.setY(surface.getY() + (surface.getHeight() - contentH) / 2);

        auto dynRegion = content;
        const auto dynCellW = juce::jmax(1, (dynRegion.getWidth() - 3 * gap) / 4);
        auto dynLabels = dynRegion.removeFromTop(labelH);
        auto dynValues = dynRegion.removeFromTop(controlH);
        for (size_t i = 0; i < 4; ++i) {
            primary_label_bounds_[i] = dynLabels.removeFromLeft(dynCellW);
            if (i < 3) dynLabels.removeFromLeft(gap);
        }
        threshold_slider_.setBounds(dynValues.removeFromLeft(dynCellW)); dynValues.removeFromLeft(gap);
        range_slider_.setBounds(dynValues.removeFromLeft(dynCellW)); dynValues.removeFromLeft(gap);
        attack_slider_.setBounds(dynValues.removeFromLeft(dynCellW)); dynValues.removeFromLeft(gap);
        release_slider_.setBounds(dynValues);

        const auto flagW = juce::jmax(42, juce::roundToInt(font * 3.05f));
        const auto actionW = 3 * flagW + 2 * gap;
        auto detailRegion = content;
        auto actions = detailRegion.removeFromRight(juce::jmin(actionW, detailRegion.getWidth() / 2));
        detailRegion.removeFromRight(gap);

        const auto detailCellW = juce::jmax(1, (detailRegion.getWidth() - 3 * gap) / 4);
        auto detailLabels = detailRegion.removeFromTop(labelH);
        auto detailValues = detailRegion.removeFromTop(controlH);
        for (size_t i = 0; i < 4; ++i) {
            detail_label_bounds_[i] = detailLabels.removeFromLeft(detailCellW);
            if (i < 3) detailLabels.removeFromLeft(gap);
        }

        auto detectorValues = detailValues;
        knee_slider_.setBounds(detectorValues.removeFromLeft(detailCellW)); detectorValues.removeFromLeft(gap);
        rms_length_slider_.setBounds(detectorValues.removeFromLeft(detailCellW)); detectorValues.removeFromLeft(gap);
        rms_mix_slider_.setBounds(detectorValues.removeFromLeft(detailCellW)); detectorValues.removeFromLeft(gap);
        smooth_slider_.setBounds(detectorValues);

        auto sideValues = detailValues;
        side_type_box_.setBounds(sideValues.removeFromLeft(detailCellW)); sideValues.removeFromLeft(gap);
        side_order_box_.setBounds(sideValues.removeFromLeft(detailCellW)); sideValues.removeFromLeft(gap);
        side_freq_slider_.setBounds(sideValues.removeFromLeft(detailCellW)); sideValues.removeFromLeft(gap);
        side_q_slider_.setBounds(sideValues);

        actions.removeFromTop(labelH);
        actions.setHeight(controlH);
        learn_button_.setBounds(actions.removeFromLeft(flagW)); actions.removeFromLeft(gap);
        relative_button_.setBounds(actions.removeFromLeft(flagW)); actions.removeFromLeft(gap);
        dyn_bypass_button_.setBounds(actions);

        auto sideActions = learn_button_.getBounds().getUnion(relative_button_.getBounds()).getUnion(dyn_bypass_button_.getBounds());
        const auto sideGap = gap;
        const auto sideW = juce::jmax(1, (sideActions.getWidth() - sideGap) / 2);
        side_link_button_.setBounds(sideActions.removeFromLeft(sideW));
        sideActions.removeFromLeft(sideGap);
        side_swap_button_.setBounds(sideActions);

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
        return juce::roundToInt(base_.getFontSize() * 31.5f);
    }

    int BandHubPanel::getIdealHeight() const {
        return juce::jmax(72, juce::roundToInt(base_.getFontSize() * 4.85f));
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
