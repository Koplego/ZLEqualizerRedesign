#pragma once

#include <juce_graphics/juce_graphics.h>

namespace zlgui::glass {
    // Reference-locked palette. The target is not near-black glass: it is a readable,
    // medium-dark blue pane with warm/teal/violet light visibly transmitted through it.
    inline juce::Colour canvasTop() { return juce::Colour(43, 78, 116); }
    inline juce::Colour canvasMid() { return juce::Colour(27, 61, 94); }
    inline juce::Colour canvasBottom() { return juce::Colour(17, 39, 63); }
    inline juce::Colour shellTop() { return juce::Colour(47, 82, 119); }
    inline juce::Colour shellBottom() { return juce::Colour(18, 42, 68); }
    inline juce::Colour textPrimary() { return juce::Colour(246, 250, 254); }
    inline juce::Colour textSecondary() { return textPrimary().withAlpha(.66f); }
    inline juce::Colour textTertiary() { return textPrimary().withAlpha(.42f); }
    inline juce::Colour rim() { return juce::Colour(246, 251, 255).withAlpha(.19f); }
    inline juce::Colour rimStrong() { return juce::Colour(252, 254, 255).withAlpha(.31f); }
    inline juce::Colour gridMajor() { return juce::Colour(210, 232, 247).withAlpha(.058f); }
    inline juce::Colour gridMinor() { return juce::Colour(210, 232, 247).withAlpha(.026f); }
    inline juce::Colour neutralResponse() { return juce::Colour(225, 241, 252); }

    inline juce::Colour surfaceTop(const float alpha = .11f) {
        return juce::Colour(150, 199, 231).withAlpha(alpha);
    }
    inline juce::Colour surfaceBottom(const float alpha = .19f) {
        return juce::Colour(28, 64, 96).withAlpha(alpha);
    }

    inline float shellRadius(const float font) { return juce::jmax(18.f, font * 1.45f); }
    inline float surfaceRadius(const float font) { return juce::jmax(11.f, font * .95f); }

    inline void fillGlassSurface(juce::Graphics& g, juce::Rectangle<float> bounds,
                                 const float radius, const float topAlpha = .10f,
                                 const float bottomAlpha = .18f, const float rimAlpha = .15f) {
        juce::Path clip;
        clip.addRoundedRectangle(bounds, radius);
        {
            juce::Graphics::ScopedSaveState state(g);
            g.reduceClipRegion(clip);

            // A visible blue body is part of the approved reference. Keep it translucent,
            // but never let utility cards collapse into near-black rectangles.
            g.setColour(juce::Colour(14, 32, 50).withAlpha(bottomAlpha * .58f));
            g.fillRect(bounds.expanded(1.f));

            juce::ColourGradient fill(surfaceTop(topAlpha), bounds.getCentreX(), bounds.getY(),
                                      surfaceBottom(bottomAlpha), bounds.getCentreX(), bounds.getBottom(), false);
            fill.addColour(.46, juce::Colour(57, 105, 143)
                                      .withAlpha((topAlpha + bottomAlpha) * .18f));
            g.setGradientFill(fill);
            g.fillRect(bounds.expanded(1.f));

            juce::ColourGradient transmitted(
                juce::Colour(210, 235, 249).withAlpha(topAlpha * .28f),
                bounds.getX() + bounds.getWidth() * .12f,
                bounds.getY() + bounds.getHeight() * .04f,
                juce::Colours::transparentBlack,
                bounds.getX() + bounds.getWidth() * .68f,
                bounds.getY() + bounds.getHeight() * .82f,
                true);
            transmitted.addColour(.34, juce::Colour(91, 157, 199).withAlpha(topAlpha * .075f));
            g.setGradientFill(transmitted);
            g.fillRect(bounds.expanded(radius * .12f));
        }

        g.setColour(juce::Colour(248, 252, 255).withAlpha(rimAlpha));
        g.drawRoundedRectangle(bounds, radius, .82f);

        auto highlight = bounds.reduced(1.1f);
        g.setColour(juce::Colour(255, 255, 255).withAlpha(rimAlpha * .48f));
        g.drawLine(highlight.getX() + radius * .66f, highlight.getY() + .5f,
                   highlight.getRight() - radius * .66f, highlight.getY() + .5f, .74f);
    }
}
