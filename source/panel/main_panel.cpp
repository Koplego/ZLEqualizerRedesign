// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer

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

        top_panel_.setPresetNameProvider([this]() { return preset_browser_.getDisplayPresetName(); });

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
        const auto shell = getLocalBounds().toFloat().reduced(3.f);
        const auto radius = zlgui::glass::shellRadius(base_.getFontSize());
        juce::Path clip_path;
        clip_path.addRoundedRectangle(shell, radius);

        {
            juce::Graphics::ScopedSaveState state(g);
            g.reduceClipRegion(clip_path);

            // Direct reference-derived shell colour field. v1.8 was still treating the
            // screenshot as a dark theme plus tiny tint overlays; the target is instead a
            // visibly coloured piece of glass across the whole instrument.
            juce::ColourGradient horizontal(juce::Colour(91, 98, 94), shell.getX(), shell.getCentreY(),
                                            juce::Colour(61, 73, 122), shell.getRight(), shell.getCentreY(), false);
            horizontal.addColour(.12, juce::Colour(74, 89, 96));
            horizontal.addColour(.25, juce::Colour(48, 101, 137));
            horizontal.addColour(.36, juce::Colour(57, 113, 139));
            horizontal.addColour(.48, juce::Colour(48, 92, 136));
            horizontal.addColour(.62, juce::Colour(44, 82, 131));
            horizontal.addColour(.76, juce::Colour(40, 72, 113));
            horizontal.addColour(.88, juce::Colour(61, 70, 124));
            g.setGradientFill(horizontal);
            g.fillRect(shell.expanded(2.f));

            // The top of the reference is hazy and reflective; the lower body is deeper.
            juce::ColourGradient vertical(juce::Colour(229, 241, 248).withAlpha(.085f),
                                          shell.getCentreX(), shell.getY(),
                                          juce::Colour(2, 13, 26).withAlpha(.34f),
                                          shell.getCentreX(), shell.getBottom(), false);
            vertical.addColour(.28, juce::Colours::transparentBlack);
            vertical.addColour(.77, juce::Colour(5, 20, 37).withAlpha(.11f));
            g.setGradientFill(vertical);
            g.fillRect(shell);

            // Large colour pools corresponding to the generated reference, not generic
            // decorative glows. These are intentionally visible in the header/footer.
            juce::ColourGradient warm(juce::Colour(246, 194, 111).withAlpha(.30f),
                                      shell.getX() + shell.getWidth() * .055f,
                                      shell.getY() + shell.getHeight() * .23f,
                                      juce::Colours::transparentBlack,
                                      shell.getX() + shell.getWidth() * .28f,
                                      shell.getY() + shell.getHeight() * .71f, true);
            warm.addColour(.42, juce::Colour(224, 181, 102).withAlpha(.12f));
            g.setGradientFill(warm);
            g.fillRect(shell);

            juce::ColourGradient cyan(juce::Colour(72, 192, 187).withAlpha(.22f),
                                      shell.getX() + shell.getWidth() * .31f,
                                      shell.getY() + shell.getHeight() * .42f,
                                      juce::Colours::transparentBlack,
                                      shell.getX() + shell.getWidth() * .50f,
                                      shell.getY() + shell.getHeight() * .86f, true);
            cyan.addColour(.48, juce::Colour(62, 166, 177).withAlpha(.075f));
            g.setGradientFill(cyan);
            g.fillRect(shell);

            juce::ColourGradient blue(juce::Colour(75, 151, 226).withAlpha(.20f),
                                      shell.getX() + shell.getWidth() * .51f,
                                      shell.getY() + shell.getHeight() * .20f,
                                      juce::Colours::transparentBlack,
                                      shell.getX() + shell.getWidth() * .70f,
                                      shell.getY() + shell.getHeight() * .72f, true);
            g.setGradientFill(blue);
            g.fillRect(shell);

            juce::ColourGradient violet(juce::Colour(166, 124, 238).withAlpha(.23f),
                                        shell.getX() + shell.getWidth() * .91f,
                                        shell.getY() + shell.getHeight() * .43f,
                                        juce::Colours::transparentBlack,
                                        shell.getX() + shell.getWidth() * .70f,
                                        shell.getY() + shell.getHeight() * .84f, true);
            violet.addColour(.47, juce::Colour(132, 105, 213).withAlpha(.080f));
            g.setGradientFill(violet);
            g.fillRect(shell);
        }

        g.setColour(juce::Colour(245, 251, 255).withAlpha(.26f));
        g.drawRoundedRectangle(shell, radius, .95f);
        g.setColour(juce::Colour(222, 241, 252).withAlpha(.085f));
        g.drawRoundedRectangle(shell.reduced(2.f), juce::jmax(1.f, radius - 2.f), .68f);

        auto top_line = shell.reduced(radius * .55f, 1.f);
        top_line.setHeight(1.f);
        juce::ColourGradient spec(juce::Colours::transparentWhite, top_line.getX(), top_line.getY(),
                                  juce::Colours::transparentWhite, top_line.getRight(), top_line.getY(), false);
        spec.addColour(.17, juce::Colour(255, 255, 255).withAlpha(.22f));
        spec.addColour(.64, juce::Colour(197, 230, 250).withAlpha(.10f));
        g.setGradientFill(spec);
        g.fillRect(top_line);
    }

    void MainPanel::paintOverChildren(juce::Graphics& g) { juce::ignoreUnused(g); }

    void MainPanel::resized() {
        auto full = getLocalBounds();
        const auto height = static_cast<float>(full.getHeight());
        const auto width = static_cast<float>(full.getWidth());
        if (height < width * kHoWMin) full.setHeight(static_cast<int>(std::ceil(width * kHoWMin)));

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
            sheet.setCentre(curve_panel_.getBounds().getCentreX(), curve_panel_.getY() + padding + sheet_h / 2);
            control_panel_.setBounds(sheet);
        } else control_panel_.setBounds({});

        const auto setting_width = juce::jmax(0, juce::jmin(ui_setting_panel_.getIdealWidth(), main_bound.getWidth() - 4 * padding));
        const auto setting_height = juce::jmax(0, juce::jmin(ui_setting_panel_.getIdealHeight(), main_bound.getHeight() - 4 * padding));
        ui_setting_panel_.setBounds(main_bound.withSizeKeepingCentre(setting_width, setting_height));

        const auto preset_width = juce::jmax(0, juce::jmin(preset_browser_.getIdealWidth(), main_bound.getWidth() - 4 * padding));
        const auto preset_height = juce::jmax(0, juce::jmin(preset_browser_.getIdealHeight(), main_bound.getHeight() - 4 * padding));
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
        if (keep != zlgui::PanelSettingIdx::kPresetBrowser) base_.setPanelProperty(zlgui::PanelSettingIdx::kPresetBrowser, 0.0);
        if (keep != zlgui::PanelSettingIdx::kUISettingPanel) base_.setPanelProperty(zlgui::PanelSettingIdx::kUISettingPanel, 0.0);
        if (keep != zlgui::PanelSettingIdx::kMatchPanel) base_.setPanelProperty(zlgui::PanelSettingIdx::kMatchPanel, 0.0);
        base_.setPanelProperty(zlgui::PanelSettingIdx::kAnalyzerPanel, 0.0);
        base_.setPanelProperty(zlgui::PanelSettingIdx::kOutputPanel, 0.0);
    }

    void MainPanel::closeGlobalSheetsForUtility(const zlgui::PanelSettingIdx utility) {
        base_.setPanelProperty(zlgui::PanelSettingIdx::kPresetBrowser, 0.0);
        base_.setPanelProperty(zlgui::PanelSettingIdx::kUISettingPanel, 0.0);
        base_.setPanelProperty(zlgui::PanelSettingIdx::kMatchPanel, 0.0);
        if (utility != zlgui::PanelSettingIdx::kAnalyzerPanel) base_.setPanelProperty(zlgui::PanelSettingIdx::kAnalyzerPanel, 0.0);
        if (utility != zlgui::PanelSettingIdx::kOutputPanel) base_.setPanelProperty(zlgui::PanelSettingIdx::kOutputPanel, 0.0);
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
        if (!refresh_handler_.tick(time_stamp)) return;
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

    void MainPanel::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property) {
        if (base_.isPanelIdentifier(zlgui::PanelSettingIdx::kUISettingPanel, property)) {
            const auto open = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kUISettingPanel)) > .5;
            if (open) closeGlobalOverlaysExcept(zlgui::PanelSettingIdx::kUISettingPanel);
            updateOverlayState(); resized();
        } else if (base_.isPanelIdentifier(zlgui::PanelSettingIdx::kPresetBrowser, property)) {
            const auto open = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kPresetBrowser)) > .5;
            if (open) closeGlobalOverlaysExcept(zlgui::PanelSettingIdx::kPresetBrowser);
            updateOverlayState(); resized();
        } else if (base_.isPanelIdentifier(zlgui::PanelSettingIdx::kMatchPanel, property)) {
            const auto open = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kMatchPanel)) > .5;
            if (open) closeGlobalOverlaysExcept(zlgui::PanelSettingIdx::kMatchPanel);
            updateOverlayState(); resized();
        } else if (base_.isPanelIdentifier(zlgui::PanelSettingIdx::kAnalyzerPanel, property)) {
            const auto open = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kAnalyzerPanel)) > .5;
            if (open) closeGlobalSheetsForUtility(zlgui::PanelSettingIdx::kAnalyzerPanel);
            updateOverlayState(); resized();
        } else if (base_.isPanelIdentifier(zlgui::PanelSettingIdx::kOutputPanel, property)) {
            const auto open = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kOutputPanel)) > .5;
            if (open) closeGlobalSheetsForUtility(zlgui::PanelSettingIdx::kOutputPanel);
            updateOverlayState(); resized();
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
