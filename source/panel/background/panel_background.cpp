// Copyright (C) 2026 - zsliu98
// This file is part of ZLSpectrumEqualizer
//
// ZLSpectrumEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.

#include "panel_background.hpp"
#include "../../gui/glass_tokens.hpp"

#include <utility>

#include "../helper/panel_constants.hpp"

namespace zlpanel {
    PanelBackground::PanelBackground(zlgui::UIBase& base, const float shadow_alpha) :
        base_(base), shadow_alpha_(shadow_alpha) {
        setInterceptsMouseClicks(false, false);
        setOpaque(false);
    }

    void PanelBackground::paint(juce::Graphics& g) {
        const auto padding = getPaddingSize(base_.getFontSize());
        const auto bound = getLocalBounds().reduced(juce::jmax(2, padding / 2)).toFloat();
        const auto corner = zlgui::glass::surfaceRadius(base_.getFontSize());

        juce::Path path;
        path.addRoundedRectangle(bound, corner);
        const juce::DropShadow shadow{juce::Colours::black.withAlpha(0.16f * shadow_alpha_),
                                      juce::jmax(padding + 1, 3), {0, juce::jmax(1, padding / 4)}};
        shadow.drawForPath(g, path);

        // Secondary workflows should read as one quiet floating sheet, not as the old ZL
        // panels with a glass coat. Keep the body translucent and let spacing do the work.
        g.setColour(juce::Colour(8, 23, 37).withAlpha(.34f));
        g.fillRoundedRectangle(bound, corner);
        zlgui::glass::fillGlassSurface(g, bound, corner, .10f, .16f, .22f);

        if (paints_surfaces_) {
            for (const auto& surface_bound : surface_bounds_) {
                auto surface = surface_bound.toFloat();
                g.setColour(juce::Colour(4, 18, 31).withAlpha(.095f));
                g.fillRoundedRectangle(surface, static_cast<float>(padding) * .66f);
                g.setColour(zlgui::glass::rim().withMultipliedAlpha(.34f));
                g.drawRoundedRectangle(surface, static_cast<float>(padding) * .66f, .6f);
            }
        }
    }

    void PanelBackground::setSurfaceBounds(std::vector<juce::Rectangle<int>> bounds) {
        surface_bounds_ = std::move(bounds);
        paints_surfaces_ = true;
        repaint();
    }
}
