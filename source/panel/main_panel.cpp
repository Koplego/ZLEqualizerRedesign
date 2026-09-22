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
#include "../gui/ambient_light.hpp"
#include <cmath>
#include <vector>

namespace zlpanel {
    namespace {
        std::vector<zlgui::glass::AmbientLightSource> collectAmbientNodes(
            PluginProcessor& p,
            zlgui::UIBase& base,
            juce::Rectangle<float> graph,
            const double sample_rate) {
            std::vector<zlgui::glass::AmbientLightSource> result;
            result.reserve(zlp::kBandNum);
            if (graph.isEmpty() || sample_rate <= 1000.0) return result;

            graph.removeFromBottom(static_cast<float>(getBottomAreaHeight(base.getFontSize())));
            if (graph.getWidth() <= 1.f || graph.getHeight() <= 1.f) return result;

            const auto fft_max = static_cast<float>(freq_helper::getFFTMax(sample_rate));
            const auto log_max = std::log(fft_max * .1f);
            if (log_max <= 1.0e-5f) return result;

            const auto selected_band = base.getSelectedBand();
            const auto* scale_value = p.parameters_.getRawParameterValue(zlp::PGainScale::kID);
            const auto gain_scale = scale_value ? scale_value->load(std::memory_order_relaxed) * .01f : 1.f;
            const auto* max_value = p.parameters_NA_.getRawParameterValue(zlstate::PEQMaxDB::kID);
            const auto max_idx = max_value
                ? static_cast<size_t>(juce::jmax(0, static_cast<int>(std::round(max_value->load(std::memory_order_relaxed)))))
                : size_t{0};
            const auto max_db = juce::jmax(1.f, static_cast<float>(base.getCurveDBScale(max_idx)));

            for (size_t band = 0; band < zlp::kBandNum; ++band) {
                const auto suffix = std::to_string(band);
                const auto* status_value = p.parameters_.getRawParameterValue(zlp::PFilterStatus::kID + suffix);
                const auto* freq_value = p.parameters_.getRawParameterValue(zlp::PFreq::kID + suffix);
                const auto* gain_value = p.parameters_.getRawParameterValue(zlp::PGain::kID + suffix);
                if (status_value == nullptr || freq_value == nullptr || gain_value == nullptr) continue;

                const auto status = static_cast<zlp::FilterStatus>(
                    static_cast<int>(std::round(status_value->load(std::memory_order_relaxed))));
                if (status == zlp::FilterStatus::kOff) continue;

                const auto freq = juce::jmax(10.f, freq_value->load(std::memory_order_relaxed));
                const auto x_portion = juce::jlimit(0.f, 1.f,
                    static_cast<float>(kFFTSizeOverWidth) * std::log(freq * .1f) / log_max);
                const auto gain = juce::jlimit(-max_db, max_db,
                    gain_value->load(std::memory_order_relaxed) * gain_scale);
                const auto y_portion = juce::jlimit(0.f, 1.f, .5f - gain / (2.f * max_db));

                // Selection changes focus, not whether a band emits light. Every enabled node
                // remains a meaningful lamp in the ambient scene, matching the agreed reference.
                auto strength = band == selected_band ? 1.f : .58f;
                if (status == zlp::FilterStatus::kBypass) strength *= .38f;

                result.push_back({
                    {graph.getX() + graph.getWidth() * x_portion,
                     graph.getY() + graph.getHeight() * y_portion},
                    base.getColourMap1(band), strength, 0.f
                });
            }
            return result;
        }

        std::vector<zlgui::glass::AmbientLightSource> localiseAmbientSources(
            const std::vector<zlgui::glass::AmbientLightSource>& sources,
            const juce::Rectangle<int> receiver,
            const float radius) {
            std::vector<zlgui::glass::AmbientLightSource> local;
            local.reserve(sources.size());
            for (auto source : sources) {
                source.point.x -= static_cast<float>(receiver.getX());
                source.point.y -= static_cast<float>(receiver.getY());
                source.radius = radius;
                local.push_back(source);
            }
            return local;
        }
    }

    MainPanel::MainPanel(PluginProcessor& p, zlgui::UIBase& base, const multilingual::TooltipLanguage language) :
        p_ref_(p), base_(base),
        tooltip_helper_(language),
        refresh_handler_(zlstate::PTargetRefreshSpeed::kRates[base_.getRefreshRateID()]),
        curve_panel_(p, base, tooltip_helper_),
        band_hub_panel_(p, base, tooltip_helper_),
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
        addAndMakeVisible(band_hub_panel_);
        band_hub_panel_.updateBand();
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
        const auto bounds = getLocalBounds().toFloat();
        const auto shell = bounds.reduced(3.0f);
        const auto radius = zlgui::glass::shellRadius(base_.getFontSize());

        juce::Path shell_clip;
        shell_clip.addRoundedRectangle(shell, radius);
        {
            juce::Graphics::ScopedSaveState clip(g);
            g.reduceClipRegion(shell_clip);

            // The old hard-coded blue shell was masking the ambient system. Use a much more
            // neutral smoked-glass substrate and let the nodes provide the actual colour.
            juce::ColourGradient body(juce::Colour(39, 51, 60), shell.getCentreX(), shell.getY(),
                                      juce::Colour(10, 20, 28), shell.getCentreX(), shell.getBottom(), false);
            body.addColour(.38, juce::Colour(31, 43, 53));
            body.addColour(.74, juce::Colour(18, 30, 39));
            g.setGradientFill(body);
            g.fillRect(shell.expanded(2.f));

            const auto sample_rate = c_sample_rate_ > 1000.0 ? c_sample_rate_ : p_ref_.getSampleRate();
            const auto nodes = collectAmbientNodes(p_ref_, base_, curve_panel_.getBounds().toFloat(), sample_rate);
            for (const auto& node : nodes) {
                // Wider rather than harsher: one room-scale field plus a softer local pool.
                const auto broad_radius = juce::jmax(base_.getFontSize() * 38.f, shell.getWidth() * .46f);
                const auto near_radius = juce::jmax(base_.getFontSize() * 18.f, shell.getWidth() * .24f);
                zlgui::glass::paintAmbientField(g, node.point, node.colour, broad_radius,
                                                .026f * node.strength, shell);
                zlgui::glass::paintAmbientField(g, node.point, node.colour, near_radius,
                                                .023f * node.strength, shell);
            }
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
        top_line.addColour(.20, juce::Colour(255, 255, 255).withAlpha(.19f));
        top_line.addColour(.64, juce::Colour(232, 241, 247).withAlpha(.070f));
        g.setGradientFill(top_line);
        g.fillRect(top_specular);
    }

    void MainPanel::paintOverChildren(juce::Graphics& g) {
        // Keep the graph's final ambience restrained. The stronger room response belongs to
        // the surrounding glass, while the graph already gets colour from its fills/curves.
        const auto sample_rate = c_sample_rate_ > 1000.0 ? c_sample_rate_ : p_ref_.getSampleRate();
        const auto nodes = collectAmbientNodes(p_ref_, base_, curve_panel_.getBounds().toFloat(), sample_rate);
        const auto graph_radius = juce::jmax(base_.getFontSize() * 32.f, getWidth() * .36f);
        for (const auto& node : nodes) {
            zlgui::glass::paintAmbientField(g, node.point, node.colour, graph_radius,
                                            .0055f * node.strength,
                                            curve_panel_.getBounds().toFloat());
        }
    }

    void MainPanel::updateAmbientReceivers() {
        if (curve_panel_.getBounds().isEmpty()) return;

        const auto sample_rate = c_sample_rate_ > 1000.0 ? c_sample_rate_ : p_ref_.getSampleRate();
        const auto nodes = collectAmbientNodes(p_ref_, base_, curve_panel_.getBounds().toFloat(), sample_rate);
        const auto surface_radius = juce::jmax(base_.getFontSize() * 44.f, getWidth() * .50f);

        top_panel_.setAmbientSources(localiseAmbientSources(nodes, top_panel_.getBounds(), surface_radius));
        footer_panel_.setAmbientSources(localiseAmbientSources(nodes, footer_panel_.getBounds(), surface_radius));
        band_hub_panel_.setAmbientSources(localiseAmbientSources(nodes, band_hub_panel_.getBounds(), surface_radius * .90f));
    }

    void MainPanel::resized() {
        auto full = getLocalBounds();
        {
            const auto height = static_cast<float>(full.getHeight());
            const auto width = static_cast<float>(full.getWidth());
            if (height < width * kHoWMin) full.setHeight(static_cast<int>(std::ceil(width * kHoWMin)));
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
        const auto graph = curve_panel_.getBounds();
        const auto hub_w = juce::jmax(0, juce::jmin(band_hub_panel_.getIdealWidth(), graph.getWidth() - 4 * padding));
        const auto hub_h = juce::jmax(0, juce::jmin(band_hub_panel_.getIdealHeight(), graph.getHeight() - 3 * padding));
        auto hub = juce::Rectangle<int>(0, 0, hub_w, hub_h);
        hub.setCentre(graph.getCentreX(), graph.getBottom() - padding - hub_h / 2);
        band_hub_panel_.setBounds(hub);

        const auto match_open = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kMatchPanel)) > .5;
        if (match_open) {
            const auto max_w = juce::jmax(0, curve_panel_.getWidth() - 4 * padding);
            const auto max_h = juce::jmax(0, curve_panel_.getHeight() - 4 * padding);
            const auto sheet_w = juce::jmin(control_panel_.getActiveIdealWidth(), max_w);
            const auto sheet_h = juce::jmin(control_panel_.getActiveIdealHeight(), max_h);
            auto sheet = juce::Rectangle<int>(0, 0, sheet_w, sheet_h);
            sheet.setCentre(curve_panel_.getBounds().getCentreX(), curve_panel_.getY() + padding + sheet_h / 2);
            control_panel_.setBounds(sheet);
        } else {
            control_panel_.setBounds({});
        }

        const auto setting_width = juce::jmax(0, juce::jmin(ui_setting_panel_.getIdealWidth(), main_bound.getWidth() - 4 * padding));
        const auto setting_height = juce::jmax(0, juce::jmin(ui_setting_panel_.getIdealHeight(), main_bound.getHeight() - 4 * padding));
        ui_setting_panel_.setBounds(main_bound.withSizeKeepingCentre(setting_width, setting_height));

        const auto preset_width = juce::jmax(0, juce::jmin(preset_browser_.getIdealWidth(), main_bound.getWidth() - 4 * padding));
        const auto preset_height = juce::jmax(0, juce::jmin(preset_browser_.getIdealHeight(), main_bound.getHeight() - 4 * padding));
        preset_browser_.setBounds(main_bound.withSizeKeepingCentre(preset_width, preset_height));

        updateAmbientReceivers();
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
        const auto analyzer_open = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kAnalyzerPanel)) > .5;
        const auto output_open = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kOutputPanel)) > .5;

        const auto scrim_visible = settings_open || preset_open;
        const auto suppress_hub = match_open || settings_open || preset_open || analyzer_open || output_open;

        overlay_scrim_.setVisible(scrim_visible);
        band_hub_panel_.setSuppressed(suppress_hub);
        control_panel_.setVisible(match_open);
        ui_setting_panel_.setVisible(settings_open);
        preset_browser_.setVisible(preset_open);

        if (!suppress_hub && band_hub_panel_.isVisible()) band_hub_panel_.toFront(false);
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
                band_hub_panel_.updateBand();
            }
            if (ui_setting_panel_.isVisible()) ui_setting_panel_.flushPendingScroll();
            if (preset_browser_.isVisible()) preset_browser_.flushPendingScroll();
            curve_panel_.repaintCallBack();
            control_panel_.repaintCallBack();

            updateAmbientReceivers();

            // The whole shell is now part of the lighting scene. Repaint it when bands move,
            // not only the graph, so the header/footer ambience tracks nodes continuously.
            repaint();

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
        band_hub_panel_.repaintCallbackSlow();
        top_panel_.repaintCallbackSlow();
        footer_panel_.repaintCallbackSlow();
    }

    void MainPanel::startThreads() { curve_panel_.startThreads(); }
    void MainPanel::stopThreads() { curve_panel_.stopThreads(); }
}
