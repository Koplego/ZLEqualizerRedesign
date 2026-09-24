// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer

#include "main_panel.hpp"
#include "../gui/glass_tokens.hpp"
#include "../zlp/sample_rate_helper.hpp"

namespace {
    // A lens bends the scene underneath it. Only the thin perimeter is displaced;
    // the centre remains clear so text, curves, and controls stay readable.
    void drawRefractedRim(juce::Graphics& g, const juce::Image& scene,
                          juce::Rectangle<int> pane, float radius, float font,
                          float strength) {
        pane = pane.getIntersection(scene.getBounds());
        if (pane.getWidth() < 12 || pane.getHeight() < 12) return;
        const auto depth = juce::jlimit(4, 28, juce::roundToInt(
            juce::jmin(font * 1.65f, pane.getHeight() * .24f)));
        juce::Image::BitmapData source(scene, juce::Image::BitmapData::readOnly);
        juce::Path glass;
        glass.addRoundedRectangle(pane.toFloat(), radius);
        juce::Graphics::ScopedSaveState clip(g);
        g.reduceClipRegion(glass);

        const auto renderStrip = [&](bool horizontal, bool beginning) {
            const auto width = horizontal ? pane.getWidth() : depth;
            const auto height = horizontal ? depth : pane.getHeight();
            const auto x0 = horizontal ? pane.getX() :
                (beginning ? pane.getX() : pane.getRight() - depth);
            const auto y0 = horizontal ?
                (beginning ? pane.getY() : pane.getBottom() - depth) : pane.getY();
            juce::Image strip(juce::Image::ARGB, width, height, true);
            juce::Image::BitmapData output(strip, juce::Image::BitmapData::writeOnly);

            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    const auto fromEdge = horizontal ?
                        (beginning ? y : height - 1 - y) :
                        (beginning ? x : width - 1 - x);
                    const auto fresnel = 1.f - static_cast<float>(fromEdge) /
                                               static_cast<float>(depth);
                    const auto bend = fresnel * fresnel;
                    const auto px = x0 + x;
                    const auto py = y0 + y;
                    const auto normalShift = static_cast<int>(std::round(
                        (2.f + static_cast<float>(depth) * 1.4f) * bend));
                    const auto tangent = horizontal ?
                        (static_cast<float>(px - pane.getCentreX()) / pane.getWidth()) :
                        (static_cast<float>(py - pane.getCentreY()) / pane.getHeight());
                    const auto lateralShift = static_cast<int>(std::round(
                        tangent * static_cast<float>(depth) * .85f * bend));
                    const auto sx = juce::jlimit(0, source.width - 1,
                        px + (horizontal ? lateralShift : (beginning ? normalShift : -normalShift)));
                    const auto sy = juce::jlimit(0, source.height - 1,
                        py + (horizontal ? (beginning ? normalShift : -normalShift) : lateralShift));
                    auto transmitted = source.getPixelColour(sx, sy);
                    const auto shine = (beginning ? .22f : .10f) * bend;
                    transmitted = transmitted.interpolatedWith(juce::Colours::white, shine);
                    if (!beginning)
                        transmitted = transmitted.interpolatedWith(juce::Colours::black, .11f * bend);
                    output.setPixelColour(x, y, transmitted.withAlpha(
                        juce::jlimit(0.f, .86f, strength * (.12f * fresnel + .74f * bend))));
                }
            }
            g.drawImageAt(strip, x0, y0);
        };
        renderStrip(true, true);
        renderStrip(true, false);
        renderStrip(false, true);
        renderStrip(false, false);
    }
}

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
        const auto bounds = getLocalBounds();
        if (bounds.isEmpty()) return;
        if (!backdrop_image_.isValid() || backdrop_image_.getBounds() != bounds)
            backdrop_image_ = juce::Image(juce::Image::ARGB, bounds.getWidth(), bounds.getHeight(), true);
        backdrop_image_.clear(bounds);
        juce::Graphics underlay(backdrop_image_);
        paintGlassBackdrop(underlay);
        g.drawImageAt(backdrop_image_, 0, 0);
    }

    void MainPanel::paintGlassBackdrop(juce::Graphics& g) {
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

            // All chromatic transmission originates at visible lenses.
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
            }

            // Real depth is most legible as occlusion *behind* the raised pane.
            // These shadows are part of the neutral material and never add hue.
            const auto castShadow = [&](juce::Rectangle<int> bounds, float radius, float alpha) {
                if (bounds.isEmpty()) return;
                juce::Path silhouette;
                silhouette.addRoundedRectangle(bounds.toFloat(), radius);
                const juce::DropShadow shadow(juce::Colours::black.withAlpha(alpha),
                    juce::jmax(5, juce::roundToInt(font * .85f)),
                    {0, juce::jmax(1, juce::roundToInt(font * .16f))});
                shadow.drawForPath(g, silhouette);
            };
            castShadow(getLocalArea(&curve_panel_, curve_panel_.getGraphGlassBounds()),
                zlgui::glass::surfaceRadius(font) * 1.25f, .18f);
            castShadow(footer_panel_.getBounds(), footer_panel_.getHeight() * .43f, .17f);
            castShadow(getLocalArea(&top_panel_, top_panel_.getPresetGlassBounds()),
                top_panel_.getPresetGlassBounds().getHeight() * .5f, .14f);
        }

        juce::ColourGradient shellEdge(juce::Colour(255, 255, 255).withAlpha(.34f),
            shell.getX(), shell.getY(),
            juce::Colour(255, 255, 255).withAlpha(.055f),
            shell.getRight(), shell.getBottom(), false);
        shellEdge.addColour(.55, juce::Colour(245, 250, 253).withAlpha(.13f));
        g.setGradientFill(shellEdge);
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
        if (!backdrop_image_.isValid() || overlay_scrim_.isVisible()
            || control_panel_.isVisible()) return;

        const auto font = base_.getFontSize();
        const auto graph = getLocalArea(&curve_panel_, curve_panel_.getGraphGlassBounds());
        if (!graph_material_image_.isValid()
            || graph_material_image_.getWidth() != graph.getWidth()
            || graph_material_image_.getHeight() != graph.getHeight())
            graph_material_image_ = curve_panel_.getGraphMaterialImage();

        // A copy of the actual node-lit backdrop plus the graph's real grid is the
        // optical input. Rim pixels are sampled from displaced positions in it.
        auto scene = backdrop_image_.createCopy();
        if (graph_material_image_.isValid()) {
            juce::Graphics composite(scene);
            composite.drawImageAt(graph_material_image_, graph.getX(), graph.getY());
        }

        const auto shell = getLocalBounds().reduced(4);
        const auto footer = footer_panel_.getBounds().reduced(2);
        const auto meter = getLocalArea(&curve_panel_, curve_panel_.getMeterGlassBounds())
            .reduced(1);
        const auto preset = getLocalArea(&top_panel_, top_panel_.getPresetGlassBounds())
            .reduced(1);
        const auto speed = getLocalArea(&footer_panel_, footer_panel_.getSpeedGlassBounds())
            .reduced(1);
        const auto phase = getLocalArea(&footer_panel_, footer_panel_.getPhaseGlassBounds())
            .reduced(1);

        drawRefractedRim(g, scene, shell, zlgui::glass::shellRadius(font), font, .60f);
        drawRefractedRim(g, scene, graph.reduced(1),
            zlgui::glass::surfaceRadius(font) * 1.25f, font, .82f);
        drawRefractedRim(g, scene, footer, footer.getHeight() * .43f, font, .76f);
        drawRefractedRim(g, scene, meter, juce::jmax(7.f, font * .56f), font, .66f);
        drawRefractedRim(g, scene, preset, preset.getHeight() * .5f, font, .76f);
        drawRefractedRim(g, scene, speed, speed.getHeight() * .5f, font, .66f);
        drawRefractedRim(g, scene, phase, phase.getHeight() * .5f, font, .66f);
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
        graph_material_image_ = {};

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
