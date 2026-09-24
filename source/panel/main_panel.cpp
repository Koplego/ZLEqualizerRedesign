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
            g.setColour(juce::Colour(40, 43, 47));
            g.fillRect(shell.expanded(2.f));

            // Neutral liquid-glass depth: pale reflected sky at the top, absorption at the bottom.
            juce::ColourGradient vertical(juce::Colour(245, 245, 245).withAlpha(.085f),
                                          shell.getCentreX(), shell.getY(),
                                          juce::Colour(8, 9, 11).withAlpha(.34f),
                                          shell.getCentreX(), shell.getBottom(), false);
            vertical.addColour(.26, juce::Colours::transparentBlack);
            vertical.addColour(.78, juce::Colour(10, 11, 13).withAlpha(.12f));
            g.setGradientFill(vertical);
            g.fillRect(shell);

            // All chromatic transmission and edge reflections originate at visible lenses.
            // Reading the component geometry avoids a second, subtly different EQ mapping.
            const auto font = base_.getFontSize();
            for (size_t band = 0; band < zlp::kBandNum; ++band) {
                const auto* status = p_ref_.parameters_.getRawParameterValue(
                    zlp::PFilterStatus::kID + std::to_string(band));
                if (status == nullptr || static_cast<zlp::FilterStatus>(std::lround(status->load()))
                    != zlp::FilterStatus::kOn) continue;
                auto& lens = curve_panel_.getNodeLens(band);
                if (!lens.isShowing()) continue;
                const auto centre = getLocalPoint(&lens, lens.getLocalBounds().toFloat().getCentre());
                const auto colour = base_.getColourMap1(band).withMultipliedSaturation(1.15f);
                // Short-wavelength light spreads farther through this glass. This depends
                // on the source colour, never its band index or a fixed spectrum position.
                const auto spread = font * 24.f * (.8f + .6f * colour.getFloatBlue());

                // Broad transmission decays continuously in both axes. Selection does not
                // change the energy of a source; it only changes its interaction affordance.
                juce::ColourGradient transmitted(colour.withAlpha(.36f), centre.x, centre.y,
                    colour.withAlpha(0.f), centre.x + spread, centre.y, true);
                transmitted.addColour(.18, colour.withAlpha(.27f));
                transmitted.addColour(.40, colour.withAlpha(.13f));
                transmitted.addColour(.65, colour.withAlpha(.055f));
                transmitted.addColour(.84, colour.withAlpha(.018f));
                g.setGradientFill(transmitted);
                g.fillRect(shell);

                // A grazing reflection is elongated along the edge, with irradiance
                // controlled by the source-to-edge distance. Moving a node vertically
                // transfers energy between the upper and lower glass boundaries.
                const auto edgePool = [&](float y, float strength) {
                    const auto distance = std::abs(centre.y - y) / (font * 15.f);
                    const auto energy = strength / (1.f + distance * distance);
                    juce::Graphics::ScopedSaveState poolState(g);
                    g.addTransform(juce::AffineTransform::scale(font * 9.f, font * 3.0f)
                        .translated(centre.x, y));
                    juce::ColourGradient pool(colour.withAlpha(energy), 0.f, 0.f,
                        colour.withAlpha(0.f), 1.f, 0.f, true);
                    pool.addColour(.28, colour.withAlpha(energy * .64f));
                    pool.addColour(.60, colour.withAlpha(energy * .20f));
                    pool.addColour(.85, colour.withAlpha(energy * .025f));
                    g.setGradientFill(pool);
                    g.fillEllipse(-1.f, -1.f, 2.f, 2.f);
                };
                edgePool(shell.getY(), .24f);
                edgePool(shell.getBottom(), .34f);

                // Light is concentrated where it meets a glass boundary. The projected
                // reflection moves with the lens and loses energy with source distance.
                const auto reflectEdge = [&](juce::Rectangle<float> pane, float edgeRadius) {
                    juce::Path edge;
                    edge.addRoundedRectangle(pane, edgeRadius);
                    juce::Graphics::ScopedSaveState edgeState(g);
                    juce::Path edgeBand;
                    edgeBand.setUsingNonZeroWinding(false);
                    edgeBand.addRoundedRectangle(pane, edgeRadius);
                    edgeBand.addRoundedRectangle(pane.reduced(font * .55f),
                        juce::jmax(1.f, edgeRadius - font * .55f));
                    g.reduceClipRegion(edgeBand);
                    const auto strokeReflection = [&](float width, float energy) {
                        juce::ColourGradient caustic(colour.withAlpha(.46f * energy), centre.x, centre.y,
                            colour.withAlpha(0.f), centre.x + spread * 1.12f, centre.y, true);
                        caustic.addColour(.35, colour.withAlpha(.22f * energy));
                        caustic.addColour(.70, colour.withAlpha(.055f * energy));
                        g.setGradientFill(caustic);
                        g.strokePath(edge, juce::PathStrokeType(width));
                    };
                    strokeReflection(font * .80f, .07f);
                    strokeReflection(font * .42f, .13f);
                    strokeReflection(font * .18f, .24f);
                    strokeReflection(1.25f, .85f);
                };
                reflectEdge(shell.reduced(1.4f), radius);
                reflectEdge(footer_panel_.getBounds().toFloat().reduced(1.f),
                    footer_panel_.getHeight() * .43f);
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

    void MainPanel::paintOverChildren(juce::Graphics& g) {
        const auto font = base_.getFontSize();
        const auto shell = getLocalBounds().toFloat().reduced(4.f);
        const auto graph = getLocalArea(&curve_panel_, curve_panel_.getGraphGlassBounds())
            .toFloat().reduced(1.f);
        const auto footer = footer_panel_.getBounds().toFloat().reduced(1.5f);
        const auto meter = getLocalArea(&curve_panel_, curve_panel_.getMeterGlassBounds())
            .toFloat().reduced(1.f);
        const auto preset = getLocalArea(&top_panel_, top_panel_.getPresetGlassBounds())
            .toFloat().reduced(1.f);
        const auto speed = getLocalArea(&footer_panel_, footer_panel_.getSpeedGlassBounds())
            .toFloat().reduced(1.f);
        const auto phase = getLocalArea(&footer_panel_, footer_panel_.getPhaseGlassBounds())
            .toFloat().reduced(1.f);

        // These highlights sit above the child panels, where a glass edge actually
        // appears. Each is a projection of a live node; the panes own no fixed hue.
        const auto refract = [&](juce::Rectangle<float> pane, float radius,
                                 juce::Point<float> source, juce::Colour colour,
                                 float strength) {
            if (pane.isEmpty()) return;
            juce::Path outline;
            outline.addRoundedRectangle(pane, radius);
            juce::Path band;
            band.setUsingNonZeroWinding(false);
            band.addRoundedRectangle(pane, radius);
            const auto edgeWidth = juce::jmin(font * .48f, pane.getHeight() * .09f);
            band.addRoundedRectangle(pane.reduced(edgeWidth),
                                     juce::jmax(1.f, radius - edgeWidth));

            const auto drawHorizontal = [&](float y, bool top) {
                const auto x = juce::jlimit(pane.getX() + radius + font,
                                            pane.getRight() - radius - font,
                                            source.x + (y - source.y) * .11f);
                const auto distance = std::hypot(source.x - x, source.y - y) / (font * 17.f);
                const auto energy = strength / std::pow(1.f + distance * distance, 1.4f);
                const auto reach = font * 8.4f;
                {
                    juce::Graphics::ScopedSaveState edgeState(g);
                    g.reduceClipRegion(band);
                    juce::ColourGradient glow(colour.withAlpha(.32f * energy), x, y,
                                               colour.withAlpha(0.f), x + reach, y, true);
                    glow.addColour(.32, colour.withAlpha(.16f * energy));
                    glow.addColour(.68, colour.withAlpha(.035f * energy));
                    g.setGradientFill(glow);
                    g.fillRect(pane);
                }

                // The narrow, bent caustic is the high-frequency part of the
                // refraction; its soft outer edge and hard inner filament separate
                // the glass boundary from the diffuse light behind the pane.
                const auto sign = top ? 1.f : -1.f;
                juce::Path caustic;
                caustic.startNewSubPath(x - font * 2.9f, y + sign * 1.2f);
                caustic.quadraticTo(x - font * .85f, y + sign * font * .43f,
                                    x + font * .22f, y + sign * font * .13f);
                caustic.quadraticTo(x + font * 1.35f, y - sign * font * .12f,
                                    x + font * 3.4f, y + sign * 1.0f);
                juce::Graphics::ScopedSaveState causticState(g);
                g.reduceClipRegion(outline);
                juce::ColourGradient filament(colour.withAlpha(0.f), x - font * 3.f, y,
                                               colour.withAlpha(0.f), x + font * 3.5f, y, false);
                filament.addColour(.34, colour.withAlpha(.25f * energy));
                filament.addColour(.55, colour.brighter(.35f).withAlpha(.62f * energy));
                filament.addColour(.72, colour.withAlpha(.24f * energy));
                g.setGradientFill(filament);
                g.strokePath(caustic, juce::PathStrokeType(juce::jmax(.8f, font * .09f),
                    juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            };

            drawHorizontal(pane.getY() + 1.3f, true);
            drawHorizontal(pane.getBottom() - 1.3f, false);

            const auto drawVertical = [&](float x) {
                const auto y = juce::jlimit(pane.getY() + radius + font,
                                            pane.getBottom() - radius - font,
                                            source.y + (x - source.x) * .11f);
                const auto distance = std::hypot(source.x - x, source.y - y) / (font * 17.f);
                const auto energy = strength / std::pow(1.f + distance * distance, 1.4f);
                juce::Graphics::ScopedSaveState edgeState(g);
                g.reduceClipRegion(band);
                juce::ColourGradient glow(colour.withAlpha(.27f * energy), x, y,
                                           colour.withAlpha(0.f), x, y + font * 8.f, true);
                glow.addColour(.42, colour.withAlpha(.10f * energy));
                g.setGradientFill(glow);
                g.fillRect(pane);
            };
            drawVertical(pane.getX() + 1.3f);
            drawVertical(pane.getRight() - 1.3f);
        };

        for (size_t band = 0; band < zlp::kBandNum; ++band) {
            const auto* status = p_ref_.parameters_.getRawParameterValue(
                zlp::PFilterStatus::kID + std::to_string(band));
            if (status == nullptr || static_cast<zlp::FilterStatus>(std::lround(status->load()))
                != zlp::FilterStatus::kOn) continue;
            auto& lens = curve_panel_.getNodeLens(band);
            if (!lens.isShowing()) continue;
            const auto source = getLocalPoint(&lens, lens.getLocalBounds().toFloat().getCentre());
            const auto colour = base_.getColourMap1(band).withMultipliedSaturation(1.15f);
            refract(shell, zlgui::glass::shellRadius(font), source, colour, .55f);
            refract(graph, zlgui::glass::surfaceRadius(font) * 1.25f, source, colour, .78f);
            refract(footer, footer.getHeight() * .43f, source, colour, .95f);
            refract(meter, juce::jmax(7.f, font * .56f), source, colour, .60f);
            refract(preset, preset.getHeight() * .5f, source, colour, .43f);
            refract(speed, speed.getHeight() * .5f, source, colour, .36f);
            refract(phase, phase.getHeight() * .5f, source, colour, .36f);
        }
    }

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
