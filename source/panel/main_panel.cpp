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
        top_panel_(p, base, tooltip_helper_, [this]() { toggleSettingsSheet(); }),
        footer_panel_(p, base),
        preset_browser_(p, base),
        ui_setting_panel_(p, base_),
        tooltip_laf_(base_) {
        juce::ignoreUnused(base_);
        setOpaque(false);

        top_panel_.setPresetNameProvider([this]() {
            return preset_browser_.getDisplayPresetName();
        });

        if (base_.getTooltipLangID() != 0) {
            tooltip_window_ = std::make_unique<zlgui::tooltip::TooltipWindow>(&curve_panel_);
            tooltip_window_->setLookAndFeel(&tooltip_laf_);
            tooltip_window_->setOpaque(false);
            tooltip_window_->setBufferedToImage(true);
        }

        base_.getPanelValueTree().addListener(this);
        startTimerHz(1);

        addAndMakeVisible(curve_panel_);
        addChildComponent(overlay_scrim_);

        // Normal band editing lives in the contextual inspector. ControlPanel remains
        // available only because its Match view is the dedicated EQ Match workspace.
        addChildComponent(control_panel_);
        control_panel_.setVisible(false);

        extra_dynamic_panel_.setBufferedToImage(true);
        addChildComponent(extra_dynamic_panel_);
        addAndMakeVisible(top_panel_);
        addAndMakeVisible(footer_panel_);
        addChildComponent(ui_setting_panel_);
        preset_browser_.setBufferedToImage(true);
        addChildComponent(preset_browser_);

        updateOverlayState();
    }

    MainPanel::~MainPanel() {
        base_.getPanelValueTree().removeListener(this);
        stopTimer();
    }

    void MainPanel::paint(juce::Graphics& g) {
        const auto bounds = getLocalBounds().toFloat();
        const auto shell = bounds.reduced(3.0f);
        const auto radius = zlgui::glass::shellRadius(base_.getFontSize());

        juce::Path shell_clip;
        shell_clip.addRoundedRectangle(shell, radius);
        {
            juce::Graphics::ScopedSaveState clip(g);
            g.reduceClipRegion(shell_clip);

            // Exposure-calibrated against the supplied reference. The shell must remain
            // dark navy even where the warm/cool stained-glass field passes through it.
            juce::ColourGradient body(juce::Colour(29, 59, 85), shell.getCentreX(), shell.getY(),
                                      juce::Colour(5, 18, 32), shell.getCentreX(), shell.getBottom(), false);
            body.addColour(.36, juce::Colour(19, 45, 69));
            body.addColour(.72, juce::Colour(9, 29, 48));
            g.setGradientFill(body);
            g.fillRect(shell.expanded(2.f));

            // The reference does have a broad amber -> teal/blue -> violet transmission
            // through the glass. The important part is exposure: it changes hue without
            // lifting the shell into the pale grey/pastel range of the previous build.
            juce::ColourGradient transmitted(
                juce::Colour(235, 183, 105).withAlpha(.042f),
                shell.getX(), shell.getCentreY(),
                juce::Colour(157, 119, 235).withAlpha(.040f),
                shell.getRight(), shell.getCentreY(), false);
            transmitted.addColour(.20, juce::Colour(77, 182, 177).withAlpha(.045f));
            transmitted.addColour(.43, juce::Colour(71, 146, 220).withAlpha(.040f));
            transmitted.addColour(.68, juce::Colour(69, 119, 207).withAlpha(.034f));
            g.setGradientFill(transmitted);
            g.fillRect(shell);

            juce::ColourGradient cool_bloom(
                juce::Colour(88, 174, 226).withAlpha(.045f),
                shell.getX() + shell.getWidth() * .36f,
                shell.getY() + shell.getHeight() * .05f,
                juce::Colours::transparentBlack,
                shell.getX() + shell.getWidth() * .58f,
                shell.getY() + shell.getHeight() * .60f,
                true);
            cool_bloom.addColour(.40, juce::Colour(70, 145, 203).withAlpha(.014f));
            g.setGradientFill(cool_bloom);
            g.fillRect(shell);

            juce::ColourGradient warm_bloom(
                juce::Colour(242, 192, 112).withAlpha(.028f),
                shell.getX() + shell.getWidth() * .055f,
                shell.getY() + shell.getHeight() * .20f,
                juce::Colours::transparentBlack,
                shell.getX() + shell.getWidth() * .28f,
                shell.getY() + shell.getHeight() * .69f,
                true);
            g.setGradientFill(warm_bloom);
            g.fillRect(shell);

            juce::ColourGradient violet_bloom(
                juce::Colour(159, 120, 236).withAlpha(.027f),
                shell.getX() + shell.getWidth() * .94f,
                shell.getY() + shell.getHeight() * .29f,
                juce::Colours::transparentBlack,
                shell.getX() + shell.getWidth() * .75f,
                shell.getY() + shell.getHeight() * .72f,
                true);
            g.setGradientFill(violet_bloom);
            g.fillRect(shell);
        }

        g.setColour(zlgui::glass::rimStrong());
        g.drawRoundedRectangle(shell, radius, .95f);
        const auto inner = shell.reduced(2.f);
        g.setColour(zlgui::glass::rim().withMultipliedAlpha(.42f));
        g.drawRoundedRectangle(inner, juce::jmax(1.f, radius - 2.f), .65f);

        auto top_specular = shell.reduced(radius * .55f, 1.f);
        top_specular.setHeight(1.f);
        juce::ColourGradient top_line(juce::Colours::transparentWhite, top_specular.getX(), top_specular.getY(),
                                      juce::Colours::transparentWhite, top_specular.getRight(), top_specular.getY(), false);
        top_line.addColour(.20, juce::Colour(255, 255, 255).withAlpha(.14f));
        top_line.addColour(.64, juce::Colour(188, 225, 249).withAlpha(.055f));
        g.setGradientFill(top_line);
        g.fillRect(top_specular);
    }

    void MainPanel::paintOverChildren(juce::Graphics& g) {
        juce::ignoreUnused(g);
    }

    void MainPanel::resized() {
        auto full = getLocalBounds();
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

        extra_dynamic_panel_.setBounds({});

        top_panel_.setBounds(bound.removeFromTop(top_panel_.getIdealHeight()));
        bound.removeFromTop(juce::jmax(5, outer_padding / 2));

        footer_panel_.setBounds(bound.removeFromBottom(footer_panel_.getIdealHeight()));
        bound.removeFromBottom(juce::jmax(5, outer_padding / 2));

        curve_panel_.setBounds(bound);
        overlay_scrim_.setBounds(curve_panel_.getBounds());

        const auto padding = getPaddingSize(font_size);
        const auto match_open = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kMatchPanel)) > .5;
        if (match_open) {
            const auto max_w = juce::jmax(0, curve_panel_.getWidth() - 4 * padding);
            const auto max_h = juce::jmax(0, curve_panel_.getHeight() - 4 * padding);
            const auto sheet_w = juce::jmin(control_panel_.getActiveIdealWidth(), max_w);
            const auto sheet_h = juce::jmin(control_panel_.getActiveIdealHeight(), max_h);
            auto sheet = juce::Rectangle<int>(0, 0, sheet_w, sheet_h);
            sheet.setCentre(curve_panel_.getBounds().getCentreX(),
                            curve_panel_.getY() + padding + sheet_h / 2);
            control_panel_.setBounds(sheet);
        } else {
            control_panel_.setBounds({});
        }

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

        updateOverlayState();
    }

    void MainPanel::toggleSettingsSheet() {
        const auto open = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kUISettingPanel)) > .5;
        if (!open) closeGlobalOverlaysExcept(zlgui::PanelSettingIdx::kUISettingPanel);
        base_.setPanelProperty(zlgui::PanelSettingIdx::kUISettingPanel, open ? 0.0 : 1.0);
        resized();
    }

    void MainPanel::closeGlobalOverlaysExcept(const zlgui::PanelSettingIdx keep) {
        if (keep != zlgui::PanelSettingIdx::kPresetBrowser)
            base_.setPanelProperty(zlgui::PanelSettingIdx::kPresetBrowser, 0.0);
        if (keep != zlgui::PanelSettingIdx::kUISettingPanel)
            base_.setPanelProperty(zlgui::PanelSettingIdx::kUISettingPanel, 0.0);
        if (keep != zlgui::PanelSettingIdx::kMatchPanel)
            base_.setPanelProperty(zlgui::PanelSettingIdx::kMatchPanel, 0.0);
        base_.setPanelProperty(zlgui::PanelSettingIdx::kAnalyzerPanel, 0.0);
        base_.setPanelProperty(zlgui::PanelSettingIdx::kOutputPanel, 0.0);
    }

    void MainPanel::closeGlobalSheetsForUtility(const zlgui::PanelSettingIdx utility) {
        base_.setPanelProperty(zlgui::PanelSettingIdx::kPresetBrowser, 0.0);
        base_.setPanelProperty(zlgui::PanelSettingIdx::kUISettingPanel, 0.0);
        base_.setPanelProperty(zlgui::PanelSettingIdx::kMatchPanel, 0.0);
        if (utility != zlgui::PanelSettingIdx::kAnalyzerPanel)
            base_.setPanelProperty(zlgui::PanelSettingIdx::kAnalyzerPanel, 0.0);
        if (utility != zlgui::PanelSettingIdx::kOutputPanel)
            base_.setPanelProperty(zlgui::PanelSettingIdx::kOutputPanel, 0.0);
    }

    void MainPanel::updateOverlayState() {
        const auto match_open = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kMatchPanel)) > .5;
        const auto settings_open = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kUISettingPanel)) > .5;
        const auto preset_open = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kPresetBrowser)) > .5;

        const auto scrim_visible = settings_open || preset_open;
        overlay_scrim_.setVisible(scrim_visible);
        control_panel_.setVisible(match_open);
        ui_setting_panel_.setVisible(settings_open);
        preset_browser_.setVisible(preset_open);

        if (scrim_visible) overlay_scrim_.toFront(false);
        if (control_panel_.isVisible()) control_panel_.toFront(false);
        if (settings_open) ui_setting_panel_.toFront(false);
        if (preset_open) preset_browser_.toFront(false);

        top_panel_.toFront(false);
        footer_panel_.toFront(false);
    }

    void MainPanel::repaintCallBack(const double time_stamp) {
        if (refresh_handler_.tick(time_stamp)) {
            if (time_stamp - previous_time_stamp_ > 0.1) {
                previous_time_stamp_ = time_stamp;
                repaintCallBackSlow();
            }
            if (c_band_ != base_.getSelectedBand()) {
                c_band_ = base_.getSelectedBand();
                extra_dynamic_panel_.updateBand();
                control_panel_.updateBand();
                curve_panel_.updateBand();
            }
            if (ui_setting_panel_.isVisible()) ui_setting_panel_.flushPendingScroll();
            if (preset_browser_.isVisible()) preset_browser_.flushPendingScroll();
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
            const auto open = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kUISettingPanel)) > .5;
            if (open) {
                closeGlobalOverlaysExcept(zlgui::PanelSettingIdx::kUISettingPanel);
            }
            updateOverlayState();
            resized();
        } else if (base_.isPanelIdentifier(zlgui::PanelSettingIdx::kPresetBrowser, property)) {
            const auto open = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kPresetBrowser)) > .5;
            if (open) {
                closeGlobalOverlaysExcept(zlgui::PanelSettingIdx::kPresetBrowser);
            }
            updateOverlayState();
            resized();
        } else if (base_.isPanelIdentifier(zlgui::PanelSettingIdx::kMatchPanel, property)) {
            const auto open = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kMatchPanel)) > .5;
            if (open) {
                closeGlobalOverlaysExcept(zlgui::PanelSettingIdx::kMatchPanel);
            }
            updateOverlayState();
            resized();
        } else if (base_.isPanelIdentifier(zlgui::PanelSettingIdx::kAnalyzerPanel, property)) {
            const auto open = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kAnalyzerPanel)) > .5;
            if (open) closeGlobalSheetsForUtility(zlgui::PanelSettingIdx::kAnalyzerPanel);
            updateOverlayState();
            resized();
        } else if (base_.isPanelIdentifier(zlgui::PanelSettingIdx::kOutputPanel, property)) {
            const auto open = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kOutputPanel)) > .5;
            if (open) closeGlobalSheetsForUtility(zlgui::PanelSettingIdx::kOutputPanel);
            updateOverlayState();
            resized();
        }
    }

    void MainPanel::timerCallback() {
        if (juce::Process::isForegroundProcess()) {
            if (getCurrentlyFocusedComponent() != this) grabKeyboardFocus();
            stopTimer();
        }
    }

    void MainPanel::repaintCallBackSlow() {
        const auto sample_rate = p_ref_.getAtomicSampleRate();
        if (std::abs(sample_rate - c_sample_rate_) > 1.0) {
            c_sample_rate_ = sample_rate;
            curve_panel_.updateSampleRate(sample_rate);
            control_panel_.updateSampleRate(sample_rate);
        }
        extra_dynamic_panel_.repaintCallBackSlow();
        control_panel_.repaintCallBackSlow();
        curve_panel_.repaintCallBackSlow();
        top_panel_.repaintCallbackSlow();
        footer_panel_.repaintCallbackSlow();
    }

    void MainPanel::startThreads() { curve_panel_.startThreads(); }
    void MainPanel::stopThreads() { curve_panel_.stopThreads(); }
}
