#pragma once

#include <juce_graphics/juce_graphics.h>

namespace zlgui::glass {
    // Shared material values for the reference rebuild. These are intentionally dark and
    // blue-biased: glass surfaces should pick up colour from what sits behind/around them,
    // not turn into pale grey cards of their own.
    inline juce::Colour canvasTop() { return juce::Colour(16, 44, 72); }
    inline juce::Colour canvasMid() { return juce::Colour(10, 32, 54); }
    inline juce::Colour canvasBottom() { return juce::Colour(4, 17, 30); }
    inline juce::Colour shellTop() { return juce::Colour(29, 59, 85); }
    inline juce::Colour shellBottom() { return juce::Colour(5, 18, 32); }
    inline juce::Colour textPrimary() { return juce::Colour(244, 249, 253); }
    inline juce::Colour textSecondary() { return textPrimary().withAlpha(.60f); }
    inline juce::Colour textTertiary() { return textPrimary().withAlpha(.34f); }
    inline juce::Colour rim() { return juce::Colour(246, 251, 255).withAlpha(.14f); }
    inline juce::Colour rimStrong() { return juce::Colour(252, 254, 255).withAlpha(.23f); }
    inline juce::Colour gridMajor() { return juce::Colour(210, 232, 247).withAlpha(.043f); }
    inline juce::Colour gridMinor() { return juce::Colour(210, 232, 247).withAlpha(.018f); }
    inline juce::Colour neutralResponse() { return juce::Colour(218, 239, 253); }

    inline juce::Colour surfaceTop(const float alpha = .11f) {
        return juce::Colour(126, 186, 224).withAlpha(alpha);
    }
    inline juce::Colour surfaceBottom(const float alpha = .19f) {
        return juce::Colour(19, 48, 73).withAlpha(alpha);
    }

    inline float shellRadius(const float font) { return juce::jmax(18.f, font * 1.45f); }
    inline float surfaceRadius(const float font) { return juce::jmax(11.f, font * .95f); }

    // Dark liquid glass. The body supplies depth, then a very narrow cool specular layer is
    // added on top. This avoids the milky/frosted look that was pushing the implementation
    // far away from the supplied mockup.
    inline void fillGlassSurface(juce::Graphics& g, juce::Rectangle<float> bounds,
                                 const float radius, const float topAlpha = .10f,
                                 const float bottomAlpha = .18f, const float rimAlpha = .15f) {
        juce::Path clip;
        clip.addRoundedRectangle(bounds, radius);
        {
            juce::Graphics::ScopedSaveState state(g);
            g.reduceClipRegion(clip);

            g.setColour(juce::Colour(3, 15, 27).withAlpha(bottomAlpha * .66f));
            g.fillRect(bounds.expanded(1.f));

            juce::ColourGradient fill(surfaceTop(topAlpha), bounds.getCentreX(), bounds.getY(),
                                      surfaceBottom(bottomAlpha), bounds.getCentreX(), bounds.getBottom(), false);
            fill.addColour(.50, juce::Colour(47, 92, 126)
                                      .withAlpha((topAlpha + bottomAlpha) * .12f));
            g.setGradientFill(fill);
            g.fillRect(bounds.expanded(1.f));

            juce::ColourGradient transmitted(
                juce::Colour(201, 229, 247).withAlpha(topAlpha * .20f),
                bounds.getX() + bounds.getWidth() * .14f,
                bounds.getY() + bounds.getHeight() * .06f,
                juce::Colours::transparentBlack,
                bounds.getX() + bounds.getWidth() * .62f,
                bounds.getY() + bounds.getHeight() * .76f,
                true);
            transmitted.addColour(.36, juce::Colour(82, 145, 188).withAlpha(topAlpha * .045f));
            g.setGradientFill(transmitted);
            g.fillRect(bounds.expanded(radius * .12f));
        }

        g.setColour(juce::Colour(248, 252, 255).withAlpha(rimAlpha));
        g.drawRoundedRectangle(bounds, radius, .78f);

        auto highlight = bounds.reduced(1.1f);
        g.setColour(juce::Colour(255, 255, 255).withAlpha(rimAlpha * .36f));
        g.drawLine(highlight.getX() + radius * .66f, highlight.getY() + .5f,
                   highlight.getRight() - radius * .66f, highlight.getY() + .5f, .70f);
    }
}
