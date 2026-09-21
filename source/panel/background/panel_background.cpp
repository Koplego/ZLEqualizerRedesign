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
        const juce::DropShadow shadow{juce::Colours::black.withAlpha(0.22f * shadow_alpha_),
                                      juce::jmax(padding + 2, 3), {0, juce::jmax(1, padding / 3)}};
        shadow.drawForPath(g, path);

        zlgui::glass::fillGlassSurface(g, bound, corner, .15f, .25f, .18f);

        if (paints_surfaces_) {
            g.setColour(juce::Colour(7, 22, 35).withAlpha(.16f));
            for (const auto& surface_bound : surface_bounds_) {
                g.fillRoundedRectangle(surface_bound.toFloat(), static_cast<float>(padding) * .72f);
            }
        }
    }

    void PanelBackground::setSurfaceBounds(std::vector<juce::Rectangle<int>> bounds) {
        surface_bounds_ = std::move(bounds);
        paints_surfaces_ = true;
        repaint();
    }
}
