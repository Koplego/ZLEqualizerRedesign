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

            // Reference lock: use the actual broad colour field visible in the approved
            // image as the graph material instead of a generic navy pane. This is opaque on
            // purpose; later light pools and band illumination are layered on top of it.
            juce::ColourGradient horizontal(juce::Colour(86, 99, 94), panel.getX(), panel.getCentreY(),
                                            juce::Colour(46, 48, 91), panel.getRight(), panel.getCentreY(), false);
            horizontal.addColour(.14, juce::Colour(78, 92, 91));
            horizontal.addColour(.25, juce::Colour(42, 83, 105));
            horizontal.addColour(.36, juce::Colour(38, 88, 109));
            horizontal.addColour(.50, juce::Colour(31, 69, 107));
            horizontal.addColour(.62, juce::Colour(35, 69, 111));
            horizontal.addColour(.76, juce::Colour(35, 61, 96));
            horizontal.addColour(.88, juce::Colour(48, 61, 107));
            g.setGradientFill(horizontal);
            g.fillRect(panel.expanded(2.f));

            // Match the reference's stronger vertical falloff: brighter / hazier at the top,
            // considerably darker toward the frequency labels at the bottom.
            juce::ColourGradient vertical(juce::Colour(126, 156, 179).withAlpha(.105f),
                                          panel.getCentreX(), panel.getY(),
                                          juce::Colour(3, 14, 28).withAlpha(.43f),
                                          panel.getCentreX(), panel.getBottom(), false);
            vertical.addColour(.40, juce::Colours::transparentBlack);
            vertical.addColour(.74, juce::Colour(4, 17, 31).withAlpha(.16f));
            g.setGradientFill(vertical);
            g.fillRect(panel);

            // Large optical pools measured from the target image. They are deliberately
            // much stronger than v1.8 because the reference visibly changes the graph's
            // colour hundreds of pixels away from the control points.
            juce::ColourGradient warm(juce::Colour(244, 193, 107).withAlpha(.34f),
                                      panel.getX() + panel.getWidth() * .075f,
                                      panel.getY() + panel.getHeight() * .36f,
                                      juce::Colours::transparentBlack,
                                      panel.getX() + panel.getWidth() * .27f,
                                      panel.getY() + panel.getHeight() * .67f, true);
            warm.addColour(.42, juce::Colour(218, 174, 92).withAlpha(.13f));
            warm.addColour(.72, juce::Colour(188, 150, 79).withAlpha(.035f));
            g.setGradientFill(warm);
            g.fillRect(panel);

            juce::ColourGradient teal(juce::Colour(70, 205, 180).withAlpha(.20f),
                                      panel.getX() + panel.getWidth() * .27f,
                                      panel.getY() + panel.getHeight() * .53f,
                                      juce::Colours::transparentBlack,
                                      panel.getX() + panel.getWidth() * .48f,
                                      panel.getY() + panel.getHeight() * .88f, true);
            teal.addColour(.46, juce::Colour(62, 176, 163).withAlpha(.075f));
            g.setGradientFill(teal);
            g.fillRect(panel);

            juce::ColourGradient blue(juce::Colour(69, 145, 231).withAlpha(.22f),
                                      panel.getX() + panel.getWidth() * .54f,
                                      panel.getY() + panel.getHeight() * .29f,
                                      juce::Colours::transparentBlack,
                                      panel.getX() + panel.getWidth() * .72f,
                                      panel.getY() + panel.getHeight() * .73f, true);
            blue.addColour(.45, juce::Colour(58, 122, 204).withAlpha(.080f));
            g.setGradientFill(blue);
            g.fillRect(panel);

            juce::ColourGradient violet(juce::Colour(161, 118, 238).withAlpha(.22f),
                                        panel.getX() + panel.getWidth() * .88f,
                                        panel.getY() + panel.getHeight() * .43f,
                                        juce::Colours::transparentBlack,
                                        panel.getX() + panel.getWidth() * .69f,
                                        panel.getY() + panel.getHeight() * .82f, true);
            violet.addColour(.46, juce::Colour(132, 103, 214).withAlpha(.085f));
            g.setGradientFill(violet);
            g.fillRect(panel);
        }

        g.setColour(juce::Colour(220, 238, 248).withAlpha(.12f));
        g.drawRoundedRectangle(panel, radius, .72f);

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
