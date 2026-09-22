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

            // Locked to the approved screenshot: the graph is a medium-dark steel/blue
            // pane, not near-black. It stays dark enough for the curves to glow, while
            // retaining the clearly visible blue body seen in the reference.
            juce::ColourGradient glass(juce::Colour(35, 73, 108), panel.getCentreX(), panel.getY(),
                                       juce::Colour(18, 43, 69), panel.getCentreX(), panel.getBottom(), false);
            glass.addColour(.42, juce::Colour(29, 66, 101));
            glass.addColour(.78, juce::Colour(22, 50, 78));
            g.setGradientFill(glass);
            g.fillRect(panel.expanded(2.f));

            // Broad colour transmission is deliberately obvious in the target. The left
            // third is amber/olive, the middle crosses teal into blue and the right side
            // carries a violet wash. These are background light fields, not band fills.
            juce::ColourGradient transmitted(
                juce::Colour(236, 186, 108).withAlpha(.115f), panel.getX(), panel.getCentreY(),
                juce::Colour(160, 120, 239).withAlpha(.100f), panel.getRight(), panel.getCentreY(), false);
            transmitted.addColour(.20, juce::Colour(80, 186, 177).withAlpha(.100f));
            transmitted.addColour(.46, juce::Colour(69, 145, 219).withAlpha(.090f));
            transmitted.addColour(.73, juce::Colour(91, 124, 221).withAlpha(.085f));
            g.setGradientFill(transmitted);
            g.fillRect(panel);

            juce::ColourGradient cool_light(
                juce::Colour(91, 174, 231).withAlpha(.105f),
                panel.getX() + panel.getWidth() * .47f,
                panel.getY() + panel.getHeight() * .17f,
                juce::Colours::transparentBlack,
                panel.getX() + panel.getWidth() * .67f,
                panel.getY() + panel.getHeight() * .78f,
                true);
            cool_light.addColour(.40, juce::Colour(69, 144, 207).withAlpha(.040f));
            g.setGradientFill(cool_light);
            g.fillRect(panel);

            juce::ColourGradient warm_light(
                juce::Colour(246, 196, 116).withAlpha(.105f),
                panel.getX() + panel.getWidth() * .075f,
                panel.getY() + panel.getHeight() * .30f,
                juce::Colours::transparentBlack,
                panel.getX() + panel.getWidth() * .31f,
                panel.getY() + panel.getHeight() * .78f,
                true);
            warm_light.addColour(.45, juce::Colour(211, 174, 100).withAlpha(.034f));
            g.setGradientFill(warm_light);
            g.fillRect(panel);

            juce::ColourGradient teal_light(
                juce::Colour(68, 203, 180).withAlpha(.060f),
                panel.getX() + panel.getWidth() * .28f,
                panel.getY() + panel.getHeight() * .48f,
                juce::Colours::transparentBlack,
                panel.getX() + panel.getWidth() * .45f,
                panel.getY() + panel.getHeight() * .88f,
                true);
            g.setGradientFill(teal_light);
            g.fillRect(panel);

            juce::ColourGradient violet_light(
                juce::Colour(164, 121, 240).withAlpha(.085f),
                panel.getX() + panel.getWidth() * .88f,
                panel.getY() + panel.getHeight() * .43f,
                juce::Colours::transparentBlack,
                panel.getX() + panel.getWidth() * .70f,
                panel.getY() + panel.getHeight() * .84f,
                true);
            g.setGradientFill(violet_light);
            g.fillRect(panel);

            // Only a very slight edge falloff is present in the screenshot.
            juce::ColourGradient edge_absorption(
                juce::Colour(3, 14, 25).withAlpha(.035f), panel.getX(), panel.getCentreY(),
                juce::Colours::transparentBlack, panel.getX() + panel.getWidth() * .15f,
                panel.getCentreY(), false);
            g.setGradientFill(edge_absorption);
            g.fillRect(panel);
        }

        g.setColour(zlgui::glass::rim().withMultipliedAlpha(.66f));
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
        edge_fade.addColour(0.0, juce::Colour(10, 28, 45).withAlpha(.060f));
        edge_fade.addColour(.12, juce::Colours::transparentBlack);
        edge_fade.addColour(.86, juce::Colours::transparentBlack);
        edge_fade.addColour(1.0, juce::Colour(7, 22, 38).withAlpha(.080f));
        g.setGradientFill(edge_fade);
        g.fillRect(getLocalBounds());

        g.setColour(zlgui::glass::textTertiary().withMultipliedAlpha(.96f));
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
