// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer

#include "main_panel.hpp"
#include "../gui/glass_tokens.hpp"
#include "../zlp/sample_rate_helper.hpp"

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

            // The material itself is deliberately neutral. There is no baked-in amber,
            // cyan, blue or violet field here. Chroma enters the interface only through
            // active EQ nodes below, exactly as a coloured light would enter clear glass.
            juce::ColourGradient material(juce::Colour(39, 52, 63), shell.getX(), shell.getCentreY(),
                                          juce::Colour(25, 38, 53), shell.getRight(), shell.getCentreY(), false);
            material.addColour(.38, juce::Colour(34, 49, 62));
            material.addColour(.72, juce::Colour(29, 43, 58));
            g.setGradientFill(material);
            g.fillRect(shell.expanded(2.f));

            // Neutral liquid-glass depth: pale reflected sky at the top, absorption at the bottom.
            juce::ColourGradient vertical(juce::Colour(231, 242, 248).withAlpha(.085f),
                                          shell.getCentreX(), shell.getY(),
                                          juce::Colour(2, 10, 18).withAlpha(.34f),
                                          shell.getCentreX(), shell.getBottom(), false);
            vertical.addColour(.26, juce::Colours::transparentBlack);
            vertical.addColour(.78, juce::Colour(5, 14, 24).withAlpha(.12f));
            g.setGradientFill(vertical);
            g.fillRect(shell);

            // Reconstruct the visible EQ-node positions from the same live parameters used by
            // ResponsePanel. These are the ONLY coloured illumination sources for the shell.
            const auto font = base_.getFontSize();
            const auto meter_width = juce::jmax(48, juce::roundToInt(font * 4.2f));
            const auto meter_gap = juce::jmax(3, getPaddingSize(font) / 2);
            const auto graph_w = juce::jmax(1.f, static_cast<float>(curve_panel_.getWidth() - meter_width - meter_gap));
            const auto graph_h = juce::jmax(1.f, static_cast<float>(curve_panel_.getHeight()));
            const auto sample_rate = juce::jmax(1.0, p_ref_.getAtomicSampleRate());
            const auto fft_max = freq_helper::getFFTMax(sample_rate);
            const auto freq_to_x = graph_w * kFFTSizeOverWidth / static_cast<float>(std::log(fft_max * .1));

            float db_scale = 12.f;
            if (const auto* eq_max_ref = p_ref_.parameters_NA_.getRawParameterValue(zlstate::PEQMaxDB::kID)) {
                const auto idx = static_cast<size_t>(juce::jmax(0, static_cast<int>(std::round(
                    eq_max_ref->load(std::memory_order_relaxed)))));
                db_scale = base_.getCurveDBScale(idx);
            }
            const auto response_h = graph_h - static_cast<float>(getBottomAreaHeight(font));
            const auto dragger_padding = font * kDraggerScale;
            const auto zero_y = response_h * .5f;
            const auto bottom_y = response_h - dragger_padding;
            const auto db_to_y = (zero_y - bottom_y) / juce::jmax(1.f, db_scale);

            const auto graph_origin = curve_panel_.getBounds().getPosition().toFloat();
            const auto selected = base_.getSelectedBand();
            const auto local_radius = juce::jmax(82.f, font * 7.1f);
            const auto reflected_radius = juce::jmax(170.f, font * 13.2f);

            for (size_t band = 0; band < zlp::kBandNum; ++band) {
                const auto suffix = std::to_string(band);
                const auto* status_ref = p_ref_.parameters_.getRawParameterValue(zlp::PFilterStatus::kID + suffix);
                const auto* freq_ref = p_ref_.parameters_.getRawParameterValue(zlp::PFreq::kID + suffix);
                const auto* gain_ref = p_ref_.parameters_.getRawParameterValue(zlp::PGain::kID + suffix);
                const auto* type_ref = p_ref_.parameters_.getRawParameterValue(zlp::PFilterType::kID + suffix);
                if (status_ref == nullptr || freq_ref == nullptr || gain_ref == nullptr || type_ref == nullptr) continue;

                const auto status = static_cast<zlp::FilterStatus>(std::round(
                    status_ref->load(std::memory_order_relaxed)));
                if (status == zlp::FilterStatus::kOff) continue;

                const auto freq = juce::jlimit(10.f, static_cast<float>(fft_max),
                                               freq_ref->load(std::memory_order_relaxed));
                const auto x = static_cast<float>(std::log(freq * .1f)) * freq_to_x;
                if (!std::isfinite(x)) continue;

                const auto type = static_cast<zldsp::filter::FilterType>(std::round(
                    type_ref->load(std::memory_order_relaxed)));
                auto button_gain = gain_ref->load(std::memory_order_relaxed);
                if (type == zldsp::filter::kLowShelf || type == zldsp::filter::kHighShelf
                    || type == zldsp::filter::kTiltShelf || type == zldsp::filter::kFlatTilt) {
                    button_gain *= .5f;
                } else if (type != zldsp::filter::kPeak && type != zldsp::filter::kFlatGain) {
                    button_gain = 0.f;
                }
                const auto y = db_to_y * button_gain + zero_y;
                const auto centre = graph_origin + juce::Point<float>(x, y);

                const auto intensity = band == selected ? 1.f : .66f;
                const auto colour = base_.getColourMap1(band);

                // Wide, weak internal reflection: this is what reaches header/footer/control glass.
                juce::ColourGradient reflected(colour.withAlpha(.060f * intensity), centre.x, centre.y,
                                               colour.withAlpha(0.f), centre.x + reflected_radius,
                                               centre.y, true);
                reflected.addColour(.26, colour.withAlpha(.042f * intensity));
                reflected.addColour(.58, colour.withAlpha(.016f * intensity));
                reflected.addColour(.84, colour.withAlpha(.0035f * intensity));
                g.setGradientFill(reflected);
                g.fillEllipse(centre.x - reflected_radius, centre.y - reflected_radius,
                              reflected_radius * 2.f, reflected_radius * 2.f);

                // Tighter transmitted/reflected pool immediately around the actual source.
                juce::ColourGradient local(
                    colour.interpolatedWith(juce::Colours::white, .12f).withAlpha(.17f * intensity),
                    centre.x, centre.y, colour.withAlpha(0.f), centre.x + local_radius, centre.y, true);
                local.addColour(.22, colour.withAlpha(.12f * intensity));
                local.addColour(.52, colour.withAlpha(.044f * intensity));
                local.addColour(.80, colour.withAlpha(.006f * intensity));
                g.setGradientFill(local);
                g.fillEllipse(centre.x - local_radius, centre.y - local_radius,
                              local_radius * 2.f, local_radius * 2.f);
            }
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

        // Node positions are light-source positions now, so the parent glass must refresh
        // whenever the response refreshes instead of keeping a stale coloured reflection.
        repaint();
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
