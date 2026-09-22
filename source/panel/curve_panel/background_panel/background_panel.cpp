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

            // The graph itself is neutral dark-blue glass. All strong colour must come from
            // the actual EQ bands, never from a left-to-right rainbow painted behind them.
            juce::ColourGradient glass(juce::Colour(17, 43, 66), panel.getCentreX(), panel.getY(),
                                       juce::Colour(3, 15, 27), panel.getCentreX(), panel.getBottom(), false);
            glass.addColour(.42, juce::Colour(9, 30, 49));
            glass.addColour(.78, juce::Colour(5, 21, 36));
            g.setGradientFill(glass);
            g.fillRect(panel.expanded(2.f));

            // One cool optical highlight gives the pane depth without tinting it by band.
            juce::ColourGradient top_light(
                juce::Colour(116, 188, 232).withAlpha(.040f),
                panel.getX() + panel.getWidth() * .50f,
                panel.getY() + panel.getHeight() * .05f,
                juce::Colours::transparentBlack,
                panel.getX() + panel.getWidth() * .50f,
                panel.getY() + panel.getHeight() * .58f,
                true);
            top_light.addColour(.34, juce::Colour(87, 156, 203).withAlpha(.019f));
            g.setGradientFill(top_light);
            g.fillRect(panel);

            // Slight edge absorption makes the middle feel illuminated through glass while
            // keeping the corners inky like the reference.
            juce::ColourGradient left_vignette(
                juce::Colour(1, 9, 17).withAlpha(.13f), panel.getX(), panel.getCentreY(),
                juce::Colours::transparentBlack, panel.getX() + panel.getWidth() * .20f,
                panel.getCentreY(), false);
            g.setGradientFill(left_vignette);
            g.fillRect(panel);

            juce::ColourGradient right_vignette(
                juce::Colour(1, 9, 17).withAlpha(.12f), panel.getRight(), panel.getCentreY(),
                juce::Colours::transparentBlack, panel.getRight() - panel.getWidth() * .18f,
                panel.getCentreY(), false);
            g.setGradientFill(right_vignette);
            g.fillRect(panel);
        }

        g.setColour(zlgui::glass::rim().withMultipliedAlpha(.58f));
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
