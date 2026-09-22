// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#include "background_panel.hpp"
#include "../../../gui/glass_tokens.hpp"
#include "../../../gui/ambient_light.hpp"
#include <cmath>
#include <vector>

namespace zlpanel {
    BackgroundPanel::BackgroundPanel(PluginProcessor& p,
                                     zlgui::UIBase& base,
                                     const multilingual::TooltipHelper& tooltip_helper) :
        p_ref_(p), base_(base) {
        juce::ignoreUnused(tooltip_helper);
        setInterceptsMouseClicks(false, false);
        setOpaque(false);
        lookAndFeelChanged();
    }

    void BackgroundPanel::paint(juce::Graphics& g) {
        const auto bounds = getLocalBounds().toFloat();
        auto panel = bounds.reduced(.5f);
        const auto radius = zlgui::glass::surfaceRadius(base_.getFontSize()) * 1.25f;

        juce::Path panel_clip;
        panel_clip.addRoundedRectangle(panel, radius);
        {
            juce::Graphics::ScopedSaveState clip(g);
            g.reduceClipRegion(panel_clip);

            // The graph is a dark pane, but it should visibly transmit the colour of the
            // bands that live inside it. The previous fixed blue wash was flattening every
            // state into the same colour and hiding the new spatial-light system.
            juce::ColourGradient glass(juce::Colour(31, 55, 75), panel.getCentreX(), panel.getY(),
                                       juce::Colour(6, 21, 34), panel.getCentreX(), panel.getBottom(), false);
            glass.addColour(.44, juce::Colour(17, 40, 58));
            g.setGradientFill(glass);
            g.fillRect(panel.expanded(2.f));

            // Keep only a very quiet neutral/cool transmission so an empty graph still
            // has depth. Live band colour below is now the dominant optical information.
            juce::ColourGradient cool_light(
                juce::Colour(187, 221, 241).withAlpha(.026f),
                panel.getX() + panel.getWidth() * .18f,
                panel.getY() + panel.getHeight() * .10f,
                juce::Colours::transparentBlack,
                panel.getX() + panel.getWidth() * .61f,
                panel.getY() + panel.getHeight() * .77f,
                true);
            g.setGradientFill(cool_light);
            g.fillRect(panel);

            juce::ColourGradient warm_light(
                juce::Colour(233, 207, 174).withAlpha(.010f),
                panel.getX() + panel.getWidth() * .82f,
                panel.getY() + panel.getHeight() * .24f,
                juce::Colours::transparentBlack,
                panel.getX() + panel.getWidth() * .55f,
                panel.getBottom(),
                true);
            g.setGradientFill(warm_light);
            g.fillRect(panel);

            if (freq_max_ > 10.0 && panel.getWidth() > 1.f && panel.getHeight() > 1.f) {
                std::vector<zlgui::glass::AmbientLightSource> sources;
                sources.reserve(zlp::kBandNum);

                auto graph = panel;
                graph.removeFromBottom(static_cast<float>(getBottomAreaHeight(base_.getFontSize())));

                const auto log_max = std::log(freq_max_ * .1);
                const auto* scale_value = p_ref_.parameters_.getRawParameterValue(zlp::PGainScale::kID);
                const auto gain_scale = scale_value
                    ? scale_value->load(std::memory_order_relaxed) * .01f
                    : 1.f;
                const auto* max_value = p_ref_.parameters_NA_.getRawParameterValue(zlstate::PEQMaxDB::kID);
                const auto max_idx = max_value
                    ? static_cast<size_t>(juce::jmax(0, static_cast<int>(std::round(
                        max_value->load(std::memory_order_relaxed)))))
                    : size_t{0};
                const auto max_db = juce::jmax(1.f, static_cast<float>(base_.getCurveDBScale(max_idx)));
                const auto selected_band = base_.getSelectedBand();
                const auto light_radius = juce::jmax(base_.getFontSize() * 26.f, panel.getWidth() * .40f);

                if (log_max > 1.0e-5) {
                    for (size_t band = 0; band < zlp::kBandNum; ++band) {
                        const auto suffix = std::to_string(band);
                        const auto* status_value = p_ref_.parameters_.getRawParameterValue(
                            zlp::PFilterStatus::kID + suffix);
                        const auto* freq_value = p_ref_.parameters_.getRawParameterValue(
                            zlp::PFreq::kID + suffix);
                        const auto* gain_value = p_ref_.parameters_.getRawParameterValue(
                            zlp::PGain::kID + suffix);
                        if (status_value == nullptr || freq_value == nullptr || gain_value == nullptr)
                            continue;

                        const auto status = static_cast<zlp::FilterStatus>(
                            static_cast<int>(std::round(status_value->load(std::memory_order_relaxed))));
                        if (status == zlp::FilterStatus::kOff) continue;

                        const auto freq = juce::jmax(10.f, freq_value->load(std::memory_order_relaxed));
                        const auto x_portion = juce::jlimit(0.f, 1.f,
                            static_cast<float>(kFFTSizeOverWidth) *
                            static_cast<float>(std::log(static_cast<double>(freq) * .1) / log_max));
                        const auto gain = juce::jlimit(-max_db, max_db,
                            gain_value->load(std::memory_order_relaxed) * gain_scale);
                        const auto y_portion = juce::jlimit(0.f, 1.f,
                            .5f - gain / (2.f * max_db));

                        auto strength = band == selected_band ? 1.f : .42f;
                        if (status == zlp::FilterStatus::kBypass) strength *= .40f;

                        sources.push_back({
                            {graph.getX() + graph.getWidth() * x_portion,
                             graph.getY() + graph.getHeight() * y_portion},
                            base_.getColourMap1(band), strength, light_radius
                        });
                    }
                }

                // This is intentionally painted behind grid/data. The colour reads as light
                // inside the pane rather than as a tint over labels and response curves.
                zlgui::glass::paintAmbientSources(g, sources, panel, .035f);
            }
        }

        g.setColour(zlgui::glass::rim().withMultipliedAlpha(.62f));
        g.drawRoundedRectangle(panel, radius, .72f);

        if (freq_max_ <= 10.0) {
            return;
        }
        drawFreqs(g);
        drawDBs(g);
    }

    void BackgroundPanel::updateSampleRate(const double sample_rate) {
        freq_max_ = freq_helper::getFFTMax(sample_rate);
        repaint();
    }

    void BackgroundPanel::drawFreqs(juce::Graphics& g) const {
        auto bound = getLocalBounds().toFloat();
        const auto full_width = bound.getWidth();
        bound.setWidth(bound.getWidth() * kFFTSizeOverWidth);
        const auto thickness = juce::jmax(0.40f, base_.getFontSize() * 0.040f);
        juce::RectangleList<float> minor_rects;
        for (double decade = 10.0; decade <= freq_max_; decade *= 10.0) {
            for (int multiple = 2; multiple < 10; ++multiple) {
                if (multiple == 2 || multiple == 5) continue;
                const auto freq = decade * static_cast<double>(multiple);
                if (freq >= freq_max_) break;
                const auto p = std::log(freq * .1) / std::log(freq_max_ * .1);
                minor_rects.add(static_cast<float>(p) * bound.getWidth() - thickness * .25f,
                                0.f, thickness * .50f, bound.getHeight());
            }
        }
        g.setColour(zlgui::glass::gridMinor().withMultipliedAlpha(1.22f));
        g.fillRectList(minor_rects);

        juce::RectangleList<float> rect_list;
        for (const auto& freq : kFreqValues) {
            const auto p = std::log(static_cast<double>(freq) * .1) / std::log(freq_max_ * .1);
            const auto rect = juce::Rectangle(static_cast<float>(p) * bound.getWidth() - thickness * .5f, 0.f,
                                              thickness, bound.getHeight());
            if (rect.getRight() > full_width) {
                break;
            }
            rect_list.add(rect);
        }
        g.setColour(grid_colour_);
        g.fillRectList(rect_list);

        juce::ColourGradient edge_fade;
        edge_fade.point1 = juce::Point<float>(bound.getX(), bound.getY());
        edge_fade.point2 = juce::Point<float>(bound.getX(), bound.getBottom());
        edge_fade.isRadial = false;
        edge_fade.clearColours();
        edge_fade.addColour(0.0, juce::Colour(4, 16, 28).withAlpha(.10f));
        edge_fade.addColour(.12, juce::Colours::transparentBlack);
        edge_fade.addColour(.86, juce::Colours::transparentBlack);
        edge_fade.addColour(1.0, juce::Colour(4, 16, 28).withAlpha(.13f));
        g.setGradientFill(edge_fade);
        g.fillRect(getLocalBounds());

        g.setColour(zlgui::glass::textTertiary().withMultipliedAlpha(.88f));
        g.setFont(base_.getFontSize() * .98f);
        const auto label_y0 = bound.getBottom() - base_.getFontSize() * 1.15f;
        const auto label_height = base_.getFontSize() * 1.1f;
        for (const auto& freq : kFreqValues) {
            const auto label = freq < 1000.f ? juce::String(freq) : juce::String(std::round(freq * 0.001f)) + "K";
            const auto label_width = freq < 20000.f
                ? base_.getFontSize() * 3.f
                : juce::GlyphArrangement::getStringWidth(g.getCurrentFont(), label) * 1.1f;
            const auto p = std::log(static_cast<double>(freq) * .1) / std::log(freq_max_ * .1);
            const auto rect = juce::Rectangle(static_cast<float>(p) * bound.getWidth() - label_width * .5f, label_y0,
                                              label_width, label_height);
            if (rect.getRight() > full_width) {
                break;
            }
            g.drawText(label, rect, juce::Justification::centredBottom, false);
        }
    }

    void BackgroundPanel::drawDBs(juce::Graphics& g) const {
        const auto bound = getLocalBounds().toFloat();
        const auto thickness = juce::jmax(0.40f, base_.getFontSize() * 0.040f);
        auto y0 = base_.getFontSize() - thickness * .5f;
        const auto unit_height = (bound.getHeight() - 2.f * base_.getFontSize() * kDraggerScale
            - static_cast<float>(getBottomAreaHeight(base_.getFontSize()))) / 6.f;

        juce::RectangleList<float> rect_list;
        while (y0 + thickness < bound.getHeight() - base_.getFontSize() * 3.f) {
            rect_list.add(0.f, y0, bound.getWidth(), thickness);
            y0 += unit_height;
        }
        g.setColour(grid_colour_);
        g.fillRectList(rect_list);
    }

    void BackgroundPanel::lookAndFeelChanged() {
        grid_colour_ = zlgui::glass::gridMajor().withMultipliedAlpha(1.15f);
    }
}
