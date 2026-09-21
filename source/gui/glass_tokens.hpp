#pragma once

#include <juce_graphics/juce_graphics.h>

namespace zlgui::glass {
    inline juce::Colour canvasTop() { return juce::Colour(27, 51, 72); }
    inline juce::Colour canvasMid() { return juce::Colour(19, 40, 59); }
    inline juce::Colour canvasBottom() { return juce::Colour(11, 27, 42); }
    inline juce::Colour shellTop() { return juce::Colour(48, 70, 89); }
    inline juce::Colour shellBottom() { return juce::Colour(18, 35, 51); }
    inline juce::Colour textPrimary() { return juce::Colour(244, 249, 253); }
    inline juce::Colour textSecondary() { return textPrimary().withAlpha(.60f); }
    inline juce::Colour textTertiary() { return textPrimary().withAlpha(.34f); }
    inline juce::Colour rim() { return juce::Colour(246, 251, 255).withAlpha(.18f); }
    inline juce::Colour rimStrong() { return juce::Colour(252, 254, 255).withAlpha(.28f); }
    inline juce::Colour gridMajor() { return juce::Colour(218, 236, 248).withAlpha(.050f); }
    inline juce::Colour gridMinor() { return juce::Colour(218, 236, 248).withAlpha(.022f); }
    inline juce::Colour neutralResponse() { return juce::Colour(221, 240, 253); }

    inline juce::Colour surfaceTop(const float alpha = .16f) {
        return juce::Colour(238, 248, 255).withAlpha(alpha);
    }
    inline juce::Colour surfaceBottom(const float alpha = .26f) {
        return juce::Colour(43, 68, 88).withAlpha(alpha);
    }

    inline float shellRadius(const float font) { return juce::jmax(18.f, font * 1.45f); }
    inline float surfaceRadius(const float font) { return juce::jmax(11.f, font * .95f); }

    inline void fillGlassSurface(juce::Graphics& g, juce::Rectangle<float> bounds,
                                 const float radius, const float topAlpha = .13f,
                                 const float bottomAlpha = .22f, const float rimAlpha = .18f) {
        juce::ColourGradient fill(surfaceTop(topAlpha), bounds.getCentreX(), bounds.getY(),
                                  surfaceBottom(bottomAlpha), bounds.getCentreX(), bounds.getBottom(), false);
        fill.addColour(.48, juce::Colour(127, 158, 181).withAlpha((topAlpha + bottomAlpha) * .36f));
        g.setGradientFill(fill);
        g.fillRoundedRectangle(bounds, radius);
        g.setColour(juce::Colour(248, 252, 255).withAlpha(rimAlpha));
        g.drawRoundedRectangle(bounds, radius, .85f);
        auto highlight = bounds.reduced(1.1f);
        g.setColour(juce::Colour(255, 255, 255).withAlpha(rimAlpha * .58f));
        g.drawLine(highlight.getX() + radius * .62f, highlight.getY() + .5f,
                   highlight.getRight() - radius * .62f, highlight.getY() + .5f, .8f);
    }
}
