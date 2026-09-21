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

            // A darker viewport lets the colored bands and analyzer provide the light,
            // matching the contrast hierarchy of the reference.
            juce::ColourGradient glass(juce::Colour(39, 65, 84), panel.getCentreX(), panel.getY(),
                                       juce::Colour(8, 25, 39), panel.getCentreX(), panel.getBottom(), false);
            glass.addColour(.43, juce::Colour(23, 49, 67));
            g.setGradientFill(glass);
            g.fillRect(panel.expanded(2.f));

            juce::ColourGradient center_glow(juce::Colour(187, 221, 241).withAlpha(.10f),
                                              panel.getCentreX(), panel.getY() + panel.getHeight() * .30f,
                                              juce::Colours::transparentBlack,
                                              panel.getCentreX(), panel.getBottom(), true);
            g.setGradientFill(center_glow);
            g.fillRect(panel);

            juce::ColourGradient left_mist(juce::Colour(174, 217, 245).withAlpha(.105f),
                                            panel.getX() + panel.getWidth() * .13f,
                                            panel.getY() + panel.getHeight() * .06f,
                                            juce::Colours::transparentBlack,
                                            panel.getX() + panel.getWidth() * .43f,
                                            panel.getY() + panel.getHeight() * .62f, true);
            g.setGradientFill(left_mist);
            g.fillRect(panel);

            juce::ColourGradient warm_mist(juce::Colour(233, 207, 174).withAlpha(.070f),
                                            panel.getX() + panel.getWidth() * .76f,
                                            panel.getY() + panel.getHeight() * .22f,
                                            juce::Colours::transparentBlack,
                                            panel.getX() + panel.getWidth() * .50f,
                                            panel.getBottom(), true);
            g.setGradientFill(warm_mist);
            g.fillRect(panel);

            // The faint displaced ribbon is the graph's refractive cue. Its broad soft
            // edge remains behind the grid and response so it never competes with data.
            juce::Path ribbon;
            ribbon.startNewSubPath(panel.getX() - panel.getWidth() * .04f,
                                   panel.getY() + panel.getHeight() * .23f);
            ribbon.cubicTo(panel.getX() + panel.getWidth() * .18f,
                           panel.getY() + panel.getHeight() * .06f,
                           panel.getX() + panel.getWidth() * .29f,
                           panel.getY() + panel.getHeight() * .37f,
                           panel.getX() + panel.getWidth() * .48f,
                           panel.getY() + panel.getHeight() * .22f);
            ribbon.cubicTo(panel.getX() + panel.getWidth() * .66f,
                           panel.getY() + panel.getHeight() * .09f,
                           panel.getX() + panel.getWidth() * .78f,
                           panel.getY() + panel.getHeight() * .34f,
                           panel.getRight() + panel.getWidth() * .04f,
                           panel.getY() + panel.getHeight() * .14f);
            g.setColour(juce::Colour(1, 12, 23).withAlpha(.16f));
            g.strokePath(ribbon, juce::PathStrokeType(panel.getHeight() * .13f,
                                                      juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded),
                         juce::AffineTransform::translation(0.f, panel.getHeight() * .022f));
            g.setColour(juce::Colour(207, 234, 250).withAlpha(.068f));
            g.strokePath(ribbon, juce::PathStrokeType(panel.getHeight() * .105f,
                                                      juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
            g.setColour(juce::Colour(250, 253, 255).withAlpha(.075f));
            g.strokePath(ribbon, juce::PathStrokeType(1.f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded),
                         juce::AffineTransform::translation(0.f, -panel.getHeight() * .045f));
        }

        g.setColour(zlgui::glass::rim().withMultipliedAlpha(.65f));
        g.drawRoundedRectangle(panel, radius, .75f);

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
        const auto thickness = juce::jmax(0.45f, base_.getFontSize() * 0.045f);
        juce::RectangleList<float> minor_rects;
        for (double decade = 10.0; decade <= freq_max_; decade *= 10.0) {
            for (int multiple = 2; multiple < 10; ++multiple) {
                if (multiple == 2 || multiple == 5) continue;
                const auto freq = decade * static_cast<double>(multiple);
                if (freq >= freq_max_) break;
                const auto p = std::log(freq * .1) / std::log(freq_max_ * .1);
                minor_rects.add(static_cast<float>(p) * bound.getWidth() - thickness * .30f,
                                0.f, thickness * .60f, bound.getHeight());
            }
        }
        g.setColour(zlgui::glass::gridMinor().withMultipliedAlpha(1.45f));
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

        // Very subtle top/bottom falloff, replacing the old dark strips.
        juce::ColourGradient edge_fade;
        edge_fade.point1 = juce::Point<float>(bound.getX(), bound.getY());
        edge_fade.point2 = juce::Point<float>(bound.getX(), bound.getBottom());
        edge_fade.isRadial = false;
        edge_fade.clearColours();
        edge_fade.addColour(0.0, juce::Colour(4, 16, 28).withAlpha(.13f));
        edge_fade.addColour(.12, juce::Colours::transparentBlack);
        edge_fade.addColour(.86, juce::Colours::transparentBlack);
        edge_fade.addColour(1.0, juce::Colour(4, 16, 28).withAlpha(.16f));
        g.setGradientFill(edge_fade);
        g.fillRect(getLocalBounds());

        g.setColour(zlgui::glass::textTertiary().withMultipliedAlpha(.92f));
        g.setFont(base_.getFontSize() * 1.03f);
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
        const auto thickness = juce::jmax(0.45f, base_.getFontSize() * 0.045f);
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
        grid_colour_ = zlgui::glass::gridMajor().withMultipliedAlpha(1.35f);
    }
}
