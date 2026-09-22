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

namespace zlpanel {
    BackgroundPanel::BackgroundPanel(PluginProcessor& p,
                                     zlgui::UIBase& base,
                                     const multilingual::TooltipHelper& tooltip_helper) :
        base_(base) {
        juce::ignoreUnused(p, tooltip_helper);
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

            // The supplied reference sits much closer to navy/ink than the previous build.
            // Keep the pane dark enough that coloured curves can look self-luminous instead
            // of simply being lighter paint on a grey background.
            juce::ColourGradient glass(juce::Colour(15, 43, 70), panel.getCentreX(), panel.getY(),
                                       juce::Colour(3, 15, 28), panel.getCentreX(), panel.getBottom(), false);
            glass.addColour(.43, juce::Colour(9, 31, 53));
            glass.addColour(.78, juce::Colour(5, 22, 39));
            g.setGradientFill(glass);
            g.fillRect(panel.expanded(2.f));

            // The reference does contain a broad stained-glass colour field, but it is very
            // low exposure. It should alter hue, not lift the whole graph toward pastel grey.
            juce::ColourGradient transmitted(
                juce::Colour(236, 183, 104).withAlpha(.020f), panel.getX(), panel.getCentreY(),
                juce::Colour(156, 118, 236).withAlpha(.022f), panel.getRight(), panel.getCentreY(), false);
            transmitted.addColour(.20, juce::Colour(82, 185, 178).withAlpha(.018f));
            transmitted.addColour(.46, juce::Colour(69, 142, 215).withAlpha(.020f));
            transmitted.addColour(.73, juce::Colour(91, 123, 219).withAlpha(.019f));
            g.setGradientFill(transmitted);
            g.fillRect(panel);

            // Subtle optical pools keep the glass from feeling flat, while remaining much
            // dimmer than the actual band illumination drawn above this layer.
            juce::ColourGradient cool_light(
                juce::Colour(93, 172, 226).withAlpha(.034f),
                panel.getX() + panel.getWidth() * .40f,
                panel.getY() + panel.getHeight() * .10f,
                juce::Colours::transparentBlack,
                panel.getX() + panel.getWidth() * .60f,
                panel.getY() + panel.getHeight() * .72f,
                true);
            cool_light.addColour(.38, juce::Colour(68, 142, 204).withAlpha(.012f));
            g.setGradientFill(cool_light);
            g.fillRect(panel);

            juce::ColourGradient warm_light(
                juce::Colour(245, 194, 112).withAlpha(.016f),
                panel.getX() + panel.getWidth() * .06f,
                panel.getY() + panel.getHeight() * .30f,
                juce::Colours::transparentBlack,
                panel.getX() + panel.getWidth() * .29f,
                panel.getY() + panel.getHeight() * .72f,
                true);
            g.setGradientFill(warm_light);
            g.fillRect(panel);

            juce::ColourGradient violet_light(
                juce::Colour(161, 121, 238).withAlpha(.016f),
                panel.getX() + panel.getWidth() * .92f,
                panel.getY() + panel.getHeight() * .32f,
                juce::Colours::transparentBlack,
                panel.getX() + panel.getWidth() * .74f,
                panel.getY() + panel.getHeight() * .76f,
                true);
            g.setGradientFill(violet_light);
            g.fillRect(panel);

            // Gentle dark edge absorption is visible in the reference and gives the pane
            // more depth without introducing another coloured overlay.
            juce::ColourGradient edge_absorption(
                juce::Colour(1, 8, 16).withAlpha(.085f), panel.getX(), panel.getCentreY(),
                juce::Colours::transparentBlack, panel.getX() + panel.getWidth() * .16f,
                panel.getCentreY(), false);
            g.setGradientFill(edge_absorption);
            g.fillRect(panel);
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
        g.setColour(zlgui::glass::gridMinor().withMultipliedAlpha(1.10f));
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
        edge_fade.addColour(0.0, juce::Colour(3, 15, 27).withAlpha(.095f));
        edge_fade.addColour(.12, juce::Colours::transparentBlack);
        edge_fade.addColour(.86, juce::Colours::transparentBlack);
        edge_fade.addColour(1.0, juce::Colour(3, 15, 27).withAlpha(.125f));
        g.setGradientFill(edge_fade);
        g.fillRect(getLocalBounds());

        g.setColour(zlgui::glass::textTertiary().withMultipliedAlpha(.92f));
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
        grid_colour_ = zlgui::glass::gridMajor().withMultipliedAlpha(1.04f);
    }
}
