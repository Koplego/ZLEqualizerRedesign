// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#include "main_panel.hpp"
#include "../gui/glass_tokens.hpp"

namespace zlpanel {
    MainPanel::MainPanel(PluginProcessor& p, zlgui::UIBase& base, const multilingual::TooltipLanguage language) :
        p_ref_(p), base_(base),
        tooltip_helper_(language),
        refresh_handler_(zlstate::PTargetRefreshSpeed::kRates[base_.getRefreshRateID()]),
        curve_panel_(p, base, tooltip_helper_),
        control_panel_(p, base, curve_panel_.getMatchFFTPanel(), tooltip_helper_),
        extra_dynamic_panel_(p, base, tooltip_helper_),
        top_panel_(p, base, tooltip_helper_),
        footer_panel_(p, base),
        preset_browser_(p, base),
        ui_setting_panel_(p, base_),
        tooltip_laf_(base_) {
        juce::ignoreUnused(base_);
        setOpaque(false);

        if (base_.getTooltipLangID() != 0) {
            tooltip_window_ = std::make_unique<zlgui::tooltip::TooltipWindow>(&curve_panel_);
            tooltip_window_->setLookAndFeel(&tooltip_laf_);
            tooltip_window_->setOpaque(false);
            tooltip_window_->setBufferedToImage(true);
        }

        base_.getPanelValueTree().addListener(this);

        startTimerHz(1);

        addAndMakeVisible(curve_panel_);
        // Liquid Glass UI: keep the legacy ControlPanel alive for parameter/update plumbing,
        // but do not present its permanent bottom strip. Band controls now live in the
        // contextual FloatPopPanel attached to the selected EQ node.
        addChildComponent(control_panel_);
        control_panel_.setVisible(false);
        extra_dynamic_panel_.setBufferedToImage(true);
        addChildComponent(extra_dynamic_panel_);
        addAndMakeVisible(top_panel_);
        addAndMakeVisible(footer_panel_);
        addChildComponent(ui_setting_panel_);
        preset_browser_.setBufferedToImage(true);
        addChildComponent(preset_browser_);
        preset_browser_.toFront(false);
    }

    MainPanel::~MainPanel() {
        base_.getPanelValueTree().removeListener(this);
        stopTimer();
    }

    void MainPanel::paint(juce::Graphics& g) {
        const auto bounds = getLocalBounds().toFloat();
        const auto shell = bounds.reduced(3.0f);
        const auto radius = zlgui::glass::shellRadius(base_.getFontSize());

        // One coherent material shell. The graph and floating controls provide the hierarchy;
        // the editor itself should not advertise decorative glass effects.
        juce::ColourGradient body(zlgui::glass::shellTop(), shell.getCentreX(), shell.getY(),
                                  zlgui::glass::shellBottom(), shell.getCentreX(), shell.getBottom(), false);
        body.addColour(.42, juce::Colour(34, 57, 76));
        body.addColour(.76, juce::Colour(20, 40, 58));
        g.setGradientFill(body);
        g.fillRoundedRectangle(shell, radius);

        // A restrained central bloom gives depth without the artificial streaks used in v0.9.
        juce::ColourGradient bloom(juce::Colour(159, 199, 226).withAlpha(.075f),
                                   shell.getCentreX(), shell.getY() + shell.getHeight() * .20f,
                                   juce::Colours::transparentBlack,
                                   shell.getCentreX(), shell.getBottom(), true);
        g.setGradientFill(bloom);
        g.fillRoundedRectangle(shell, radius);

        g.setColour(zlgui::glass::rimStrong());
        g.drawRoundedRectangle(shell, radius, 1.f);
        const auto inner = shell.reduced(2.f);
        g.setColour(zlgui::glass::rim().withMultipliedAlpha(.62f));
        g.drawRoundedRectangle(inner, juce::jmax(1.f, radius - 2.f), .8f);
    }

    void MainPanel::resized() {
        auto full = getLocalBounds();

        // set actual width/height
        {
            const auto height = static_cast<float>(full.getHeight());
            const auto width = static_cast<float>(full.getWidth());
            if (height < width * kHoWMin) {
                full.setHeight(static_cast<int>(std::ceil(width * kHoWMin)));
            }
        }
        const auto max_font_size = static_cast<float>(full.getWidth()) * kFontSizeOverWidth;
        const auto min_font_size = max_font_size * .25f;
        const auto font_size = base_.getFontMode() == 0
            ? max_font_size * base_.getFontScale()
            : std::clamp(base_.getStaticFontSize(), min_font_size, max_font_size);
        base_.setFontSize(font_size);

        const auto outer_padding = juce::jmax(8, juce::roundToInt(font_size * .58f));
        auto bound = full.reduced(outer_padding);
        const auto main_bound = bound;

        control_panel_.setBounds({});
        extra_dynamic_panel_.setBounds({});

        top_panel_.setBounds(bound.removeFromTop(top_panel_.getIdealHeight()));
        bound.removeFromTop(juce::jmax(5, outer_padding / 2));

        footer_panel_.setBounds(bound.removeFromBottom(footer_panel_.getIdealHeight()));
        bound.removeFromBottom(juce::jmax(5, outer_padding / 2));

        curve_panel_.setBounds(bound);

        const auto padding = getPaddingSize(font_size);
        const auto setting_width = juce::jmax(0, juce::jmin(ui_setting_panel_.getIdealWidth(),
                                                            main_bound.getWidth() - 4 * padding));
        const auto setting_height = juce::jmax(0, juce::jmin(ui_setting_panel_.getIdealHeight(),
                                                             main_bound.getHeight() - 4 * padding));
        ui_setting_panel_.setBounds(main_bound.withSizeKeepingCentre(setting_width, setting_height));

        const auto preset_width = juce::jmax(0, juce::jmin(preset_browser_.getIdealWidth(),
                                                           main_bound.getWidth() - 4 * padding));
        const auto preset_height = juce::jmax(0, juce::jmin(preset_browser_.getIdealHeight(),
                                                            main_bound.getHeight() - 4 * padding));
        preset_browser_.setBounds(main_bound.withSizeKeepingCentre(preset_width, preset_height));
    }

    void MainPanel::repaintCallBack(const double time_stamp) {
        if (refresh_handler_.tick(time_stamp)) {
            if (time_stamp - previous_time_stamp_ > 0.1) {
                previous_time_stamp_ = time_stamp;
                repaintCallBackSlow();
            }
            // update selected band
            if (c_band_ != base_.getSelectedBand()) {
                c_band_ = base_.getSelectedBand();
                extra_dynamic_panel_.updateBand();
                control_panel_.updateBand();
                curve_panel_.updateBand();
            }
            if (ui_setting_panel_.isVisible()) {
                ui_setting_panel_.flushPendingScroll();
            }
            if (preset_browser_.isVisible()) {
                preset_browser_.flushPendingScroll();
            }
            curve_panel_.repaintCallBack();
            control_panel_.repaintCallBack();
            const auto c_refresh_rate = refresh_handler_.getActualRefreshRate();
            if (std::abs(c_refresh_rate - refresh_rate_) > 0.1) {
                refresh_rate_ = c_refresh_rate;
                curve_panel_.getFFTPanel().setRefreshRate(static_cast<float>(refresh_rate_));
            }
        }
    }

    void MainPanel::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property) {
        if (base_.isPanelIdentifier(zlgui::PanelSettingIdx::kUISettingPanel, property)) {
            const auto ui_setting_visibility = static_cast<bool>(
                base_.getPanelProperty(zlgui::PanelSettingIdx::kUISettingPanel));
            ui_setting_panel_.setVisible(ui_setting_visibility);
            if (ui_setting_visibility) {
                ui_setting_panel_.toFront(false);
            }
        }
    }

    void MainPanel::timerCallback() {
        if (juce::Process::isForegroundProcess()) {
            if (getCurrentlyFocusedComponent() != this) {
                grabKeyboardFocus();
            }
            stopTimer();
        }
    }

    void MainPanel::repaintCallBackSlow() {
        // update sample rate
        const auto sample_rate = p_ref_.getAtomicSampleRate();
        if (std::abs(sample_rate - c_sample_rate_) > 1.0) {
            c_sample_rate_ = sample_rate;
            curve_panel_.updateSampleRate(sample_rate);
            control_panel_.updateSampleRate(sample_rate);
        }
        // sub slow callbacks
        extra_dynamic_panel_.repaintCallBackSlow();
        control_panel_.repaintCallBackSlow();
        curve_panel_.repaintCallBackSlow();
        top_panel_.repaintCallbackSlow();
        footer_panel_.repaintCallbackSlow();
    }

    void MainPanel::startThreads() {
        curve_panel_.startThreads();
    }

    void MainPanel::stopThreads() {
        curve_panel_.stopThreads();
    }
}
