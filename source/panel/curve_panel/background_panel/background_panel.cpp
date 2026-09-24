// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.

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

            // Neutral transparent liquid glass. The graph no longer owns any amber/teal/
            // blue/violet colour field. Colour underneath this pane comes from the live EQ
            // nodes painted by MainPanel, and colour above it comes from SinglePanel's local
            // node transmission. This panel only provides density, depth and reflection.
            g.setColour(juce::Colour(12, 15, 18).withAlpha(.10f));
            g.fillRect(panel.expanded(1.f));

            juce::ColourGradient body(juce::Colour(210, 214, 218).withAlpha(.052f),
                                      panel.getCentreX(), panel.getY(),
                                      juce::Colour(10, 13, 16).withAlpha(.12f),
                                      panel.getCentreX(), panel.getBottom(), false);
            body.addColour(.34, juce::Colour(120, 124, 128).withAlpha(.022f));
            body.addColour(.72, juce::Colour(22, 25, 28).withAlpha(.075f));
            g.setGradientFill(body);
            g.fillRect(panel.expanded(1.f));

            // Soft neutral reflection across the top edge gives the pane its liquid-glass
            // identity without inventing a coloured light source.
            juce::ColourGradient sheen(juce::Colour(239, 248, 252).withAlpha(.075f),
                                       panel.getCentreX(), panel.getY(),
                                       juce::Colours::transparentWhite,
                                       panel.getCentreX(), panel.getY() + panel.getHeight() * .42f, false);
            sheen.addColour(.22, juce::Colour(211, 231, 240).withAlpha(.032f));
            g.setGradientFill(sheen);
            g.fillRect(panel);
        }

        g.setColour(juce::Colour(225, 241, 250).withAlpha(.13f));
        g.drawRoundedRectangle(panel, radius, .72f);
        g.setColour(juce::Colour(255, 255, 255).withAlpha(.055f));
        g.drawLine(panel.getX() + radius * .72f, panel.getY() + .6f,
                   panel.getRight() - radius * .72f, panel.getY() + .6f, .65f);

        if (freq_max_ <= 10.0) return;
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
        g.setColour(juce::Colour(210, 232, 247).withAlpha(.030f));
        g.fillRectList(minor_rects);

        juce::RectangleList<float> rect_list;
        for (const auto& freq : kFreqValues) {
            const auto p = std::log(static_cast<double>(freq) * .1) / std::log(freq_max_ * .1);
            const auto rect = juce::Rectangle(static_cast<float>(p) * bound.getWidth() - thickness * .5f, 0.f,
                                              thickness, bound.getHeight());
            if (rect.getRight() > full_width) break;
            rect_list.add(rect);
        }
        g.setColour(grid_colour_);
        g.fillRectList(rect_list);

        g.setColour(juce::Colour(232, 241, 247).withAlpha(.39f));
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
            if (rect.getRight() > full_width) break;
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
        grid_colour_ = juce::Colour(210, 232, 247).withAlpha(.052f);
    }
}
