// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#include "other_ui_setting_panel.hpp"
#include "../../gui/glass_tokens.hpp"

namespace zlpanel {
    OtherUISettingPanel::OtherUISettingPanel(PluginProcessor& p, zlgui::UIBase& base) :
        p_ref_(p),
        base_(base), name_laf_(base),
        refresh_rate_box_(zlstate::PTargetRefreshSpeed::kChoices, base),
        fft_tilt_slider_("Tilt", base),
        fft_speed_slider_("Speed", base),
        single_curve_slider_("Single", base),
        sum_curve_slider_("Sum", base),
        tooltip_box_(zlstate::PTooltipLang::kChoices, base),
        font_mode_box_(zlstate::PFontMode::kChoices, base),
        font_scale_slider_("Scale", base),
        static_font_size_slider_("Static", base),
        curve_db0_slider_("Min", base),
        curve_db1_slider_("Default", base),
        curve_db2_slider_("Max", base),
        window_size_fix_box_(zlstate::PWindowSizeFix::kChoices, base) {
        juce::ignoreUnused(p_ref_);
        setOpaque(false);
        name_laf_.setFontScale(.76f);

        const auto configure_label = [this](juce::Label& label, const juce::String& text) {
            label.setText(text, juce::dontSendNotification);
            label.setJustificationType(juce::Justification::centredLeft);
            label.setLookAndFeel(&name_laf_);
            label.setAlpha(.76f);
            addAndMakeVisible(label);
        };

        for (auto* slider : {&fft_tilt_slider_, &fft_speed_slider_, &single_curve_slider_, &sum_curve_slider_,
                             &font_scale_slider_, &static_font_size_slider_, &curve_db0_slider_,
                             &curve_db1_slider_, &curve_db2_slider_}) {
            slider->setFontScale(.72f);
            slider->getSlider().setSliderSnapsToMousePosition(false);
        }

        auto style_combo = [](zlgui::combobox::CompactCombobox& combo) {
            combo.getLAF().setFontScale(.72f);
            combo.getLAF().setBoxAlpha(.38f);
            combo.getLAF().setLabelJustification(juce::Justification::centred);
            combo.setBufferedToImage(true);
        };
        style_combo(refresh_rate_box_);
        style_combo(tooltip_box_);
        style_combo(font_mode_box_);
        style_combo(window_size_fix_box_);

        configure_label(refresh_rate_label_, "Refresh Rate");
        addAndMakeVisible(refresh_rate_box_);

        configure_label(fft_label_, "Analyzer Rendering");
        fft_tilt_slider_.getSlider().setNormalisableRange(juce::NormalisableRange<double>(-4.5, 4.5, .01));
        fft_tilt_slider_.getSlider().setDoubleClickReturnValue(true, 0.);
        addAndMakeVisible(fft_tilt_slider_);
        fft_speed_slider_.getSlider().setNormalisableRange(juce::NormalisableRange<double>(0., 2., .01));
        fft_speed_slider_.getSlider().setDoubleClickReturnValue(true, 1.0);
        addAndMakeVisible(fft_speed_slider_);

        configure_label(curve_thick_label_, "Curve Weight");
        single_curve_slider_.getSlider().setNormalisableRange(juce::NormalisableRange<double>(0., 2., .01));
        single_curve_slider_.getSlider().setDoubleClickReturnValue(true, 1.0);
        addAndMakeVisible(single_curve_slider_);
        sum_curve_slider_.getSlider().setNormalisableRange(juce::NormalisableRange<double>(0., 2., .01));
        sum_curve_slider_.getSlider().setDoubleClickReturnValue(true, 1.0);
        addAndMakeVisible(sum_curve_slider_);

        configure_label(tooltip_label_, "Tooltips");
        addAndMakeVisible(tooltip_box_);

        configure_label(font_label_, "Interface Scale");
        font_mode_box_.getBox().addListener(this);
        addAndMakeVisible(font_mode_box_);
        font_scale_slider_.getSlider().setNormalisableRange(juce::NormalisableRange<double>(0.5, 1.0, .01));
        font_scale_slider_.getSlider().setDoubleClickReturnValue(true, 0.9);
        addAndMakeVisible(font_scale_slider_);
        static_font_size_slider_.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(static_font_size_slider_);

        configure_label(curve_db_label_, "Graph dB Range");
        curve_db0_slider_.getSlider().setNormalisableRange(juce::NormalisableRange<double>(1., 30.0, 1.));
        curve_db0_slider_.getSlider().setDoubleClickReturnValue(true, 6.0);
        addAndMakeVisible(curve_db0_slider_);
        curve_db1_slider_.getSlider().setNormalisableRange(juce::NormalisableRange<double>(1., 30.0, 1.));
        curve_db1_slider_.getSlider().setDoubleClickReturnValue(true, 12.0);
        addAndMakeVisible(curve_db1_slider_);
        curve_db2_slider_.getSlider().setNormalisableRange(juce::NormalisableRange<double>(1., 30.0, 1.));
        curve_db2_slider_.getSlider().setDoubleClickReturnValue(true, 30.0);
        addAndMakeVisible(curve_db2_slider_);

        configure_label(window_size_fix_label_, "Window Size");
        addAndMakeVisible(window_size_fix_box_);
    }

    void OtherUISettingPanel::loadSetting() {
        refresh_rate_box_.getBox().setSelectedItemIndex(static_cast<int>(base_.getRefreshRateID()));
        fft_tilt_slider_.getSlider().setValue(static_cast<double>(base_.getFFTExtraTilt()));
        fft_speed_slider_.getSlider().setValue(static_cast<double>(base_.getFFTExtraSpeed()));
        single_curve_slider_.getSlider().setValue(base_.getSingleEQCurveThickness());
        sum_curve_slider_.getSlider().setValue(base_.getSumEQCurveThickness());
        tooltip_box_.getBox().setSelectedItemIndex(static_cast<int>(base_.getTooltipLangID()));
        font_mode_box_.getBox().setSelectedItemIndex(static_cast<int>(base_.getFontMode()), juce::sendNotificationSync);
        font_scale_slider_.getSlider().setValue(static_cast<double>(base_.getFontScale()));
        static_font_size_slider_.getSlider().setValue(static_cast<double>(base_.getFontSize()));
        curve_db0_slider_.getSlider().setValue(base_.getCurveDBScale(0));
        curve_db1_slider_.getSlider().setValue(base_.getCurveDBScale(1));
        curve_db2_slider_.getSlider().setValue(base_.getCurveDBScale(2));
        window_size_fix_box_.getBox().setSelectedItemIndex(static_cast<int>(base_.getWindowSizeFix()));
        comboBoxChanged(&font_mode_box_.getBox());
    }

    void OtherUISettingPanel::saveSetting() {
        base_.setRefreshRateID(static_cast<size_t>(refresh_rate_box_.getBox().getSelectedItemIndex()));
        base_.setFFTExtraTilt(static_cast<float>(fft_tilt_slider_.getSlider().getValue()));
        base_.setFFTExtraSpeed(static_cast<float>(fft_speed_slider_.getSlider().getValue()));
        base_.setSingleEQCurveThickness(static_cast<float>(single_curve_slider_.getSlider().getValue()));
        base_.setSumEQCurveThickness(static_cast<float>(sum_curve_slider_.getSlider().getValue()));
        base_.setTooltipLandID(static_cast<size_t>(tooltip_box_.getBox().getSelectedItemIndex()));
        base_.setFontMode(static_cast<size_t>(font_mode_box_.getBox().getSelectedItemIndex()));
        base_.setFontScale(static_cast<float>(font_scale_slider_.getSlider().getValue()));
        base_.setStaticFontSize(static_cast<float>(static_font_size_slider_.getSlider().getValue()));
        base_.setCurveDBScales({static_cast<float>(curve_db0_slider_.getSlider().getValue()),
                                static_cast<float>(curve_db1_slider_.getSlider().getValue()),
                                static_cast<float>(curve_db2_slider_.getSlider().getValue())});
        base_.setWindowSizeFix(window_size_fix_box_.getBox().getSelectedItemIndex() > 0);
        base_.saveToAPVTS();
    }

    void OtherUISettingPanel::resetSetting() {
    }

    int OtherUISettingPanel::getIdealHeight() const {
        const auto font = base_.getFontSize();
        const auto padding = juce::roundToInt(font * .62f);
        const auto row = juce::roundToInt(font * 2.18f);
        const auto section = juce::roundToInt(font * 1.42f);
        return 3 * section + 7 * row + 9 * padding;
    }

    void OtherUISettingPanel::resized() {
        const auto font = base_.getFontSize();
        const auto padding = juce::jmax(5, juce::roundToInt(font * .62f));
        const auto row = juce::jmax(28, juce::roundToInt(font * 2.18f));
        const auto section = juce::jmax(18, juce::roundToInt(font * 1.42f));
        const auto label_w = juce::jmax(126, juce::roundToInt(font * 9.8f));
        const auto gap = juce::jmax(4, padding / 2);

        row_bounds_.clear();
        auto bound = getLocalBounds().reduced(padding, 0);
        const auto add_row = [&](juce::Rectangle<int> r) { row_bounds_.push_back(r); };
        const auto split_label = [&](juce::Rectangle<int>& inner, juce::Label& label) {
            label.setBounds(inner.removeFromLeft(juce::jmin(label_w, inner.getWidth() / 3)));
            inner.removeFromLeft(gap);
        };

        performance_title_bound_ = bound.removeFromTop(section);
        bound.removeFromTop(padding / 3);
        {
            auto r = bound.removeFromTop(row); add_row(r);
            auto inner = r.reduced(padding / 2, 1); split_label(inner, refresh_rate_label_);
            refresh_rate_box_.setBounds(inner.reduced(0, padding / 5));
            bound.removeFromTop(padding / 3);
        }
        {
            auto r = bound.removeFromTop(row); add_row(r);
            auto inner = r.reduced(padding / 2, 1); split_label(inner, fft_label_);
            const auto w = (inner.getWidth() - gap) / 2;
            fft_tilt_slider_.setBounds(inner.removeFromLeft(w)); inner.removeFromLeft(gap);
            fft_speed_slider_.setBounds(inner);
            bound.removeFromTop(padding / 3);
        }
        {
            auto r = bound.removeFromTop(row); add_row(r);
            auto inner = r.reduced(padding / 2, 1); split_label(inner, curve_thick_label_);
            const auto w = (inner.getWidth() - gap) / 2;
            single_curve_slider_.setBounds(inner.removeFromLeft(w)); inner.removeFromLeft(gap);
            sum_curve_slider_.setBounds(inner);
            bound.removeFromTop(padding / 2);
        }

        appearance_title_bound_ = bound.removeFromTop(section);
        bound.removeFromTop(padding / 3);
        {
            auto r = bound.removeFromTop(row); add_row(r);
            auto inner = r.reduced(padding / 2, 1); split_label(inner, tooltip_label_);
            tooltip_box_.setBounds(inner.reduced(0, padding / 5));
            bound.removeFromTop(padding / 3);
        }
        {
            auto r = bound.removeFromTop(row); add_row(r);
            auto inner = r.reduced(padding / 2, 1); split_label(inner, font_label_);
            const auto mode_w = juce::jmax(86, inner.getWidth() * 34 / 100);
            font_mode_box_.setBounds(inner.removeFromLeft(mode_w).reduced(0, padding / 5));
            inner.removeFromLeft(gap);
            const auto remaining_w = inner.getWidth();
            font_scale_slider_.setBounds(inner.removeFromLeft(remaining_w / 2));
            if (inner.getWidth() > gap) inner.removeFromLeft(gap);
            static_font_size_slider_.setBounds(inner);

            if (parent_width_ < 2) {
                static_font_size_slider_.setVisible(false);
            } else {
                static_font_size_slider_.setVisible(true);
                const auto max_font_size = std::floor(static_cast<float>(parent_width_) * kFontSizeOverWidth);
                const auto min_font_size = std::ceil(static_cast<float>(parent_width_) * kFontSizeOverWidth * .25f);
                static_font_size_slider_.getSlider().setNormalisableRange(juce::NormalisableRange<double>(
                    min_font_size, max_font_size, 0.01));
                static_font_size_slider_.getSlider().setDoubleClickReturnValue(
                    true, .5f * (min_font_size + max_font_size));
            }
            bound.removeFromTop(padding / 3);
        }
        {
            auto r = bound.removeFromTop(row); add_row(r);
            auto inner = r.reduced(padding / 2, 1); split_label(inner, window_size_fix_label_);
            window_size_fix_box_.setBounds(inner.reduced(0, padding / 5));
            bound.removeFromTop(padding / 2);
        }

        scale_title_bound_ = bound.removeFromTop(section);
        bound.removeFromTop(padding / 3);
        {
            auto r = bound.removeFromTop(row); add_row(r);
            auto inner = r.reduced(padding / 2, 1); split_label(inner, curve_db_label_);
            const auto w = juce::jmax(62, (inner.getWidth() - 2 * gap) / 3);
            curve_db0_slider_.setBounds(inner.removeFromLeft(w)); inner.removeFromLeft(gap);
            curve_db1_slider_.setBounds(inner.removeFromLeft(juce::jmin(w, inner.getWidth())));
            if (inner.getWidth() > gap) inner.removeFromLeft(gap);
            curve_db2_slider_.setBounds(inner);
        }
    }

    void OtherUISettingPanel::paint(juce::Graphics& g) {
        const auto font = base_.getFontSize();
        g.setColour(zlgui::glass::textPrimary().withAlpha(.84f));
        g.setFont(juce::FontOptions(font * .72f));
        g.drawText("PERFORMANCE", performance_title_bound_, juce::Justification::centredLeft, false);
        g.drawText("APPEARANCE", appearance_title_bound_, juce::Justification::centredLeft, false);
        g.drawText("GRAPH SCALE", scale_title_bound_, juce::Justification::centredLeft, false);

        for (const auto& row : row_bounds_) {
            auto r = row.toFloat().reduced(.5f);
            g.setColour(juce::Colour(238, 248, 255).withAlpha(.022f));
            g.fillRoundedRectangle(r, juce::jmax(6.f, font * .55f));
            g.setColour(zlgui::glass::rim().withMultipliedAlpha(.22f));
            g.drawRoundedRectangle(r, juce::jmax(6.f, font * .55f), .55f);
        }
    }

    void OtherUISettingPanel::setParentWidth(const int width) {
        parent_width_ = width;
    }

    void OtherUISettingPanel::comboBoxChanged(juce::ComboBox*) {
        if (font_mode_box_.getBox().getSelectedItemIndex() == 0) {
            font_scale_slider_.setInterceptsMouseClicks(true, true);
            font_scale_slider_.setAlpha(1.f);
            static_font_size_slider_.setInterceptsMouseClicks(false, false);
            static_font_size_slider_.setAlpha(.38f);
        } else {
            font_scale_slider_.setInterceptsMouseClicks(false, false);
            font_scale_slider_.setAlpha(.38f);
            static_font_size_slider_.setInterceptsMouseClicks(true, true);
            static_font_size_slider_.setAlpha(1.f);
        }
    }
}
