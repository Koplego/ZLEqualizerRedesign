#pragma once

#include <juce_graphics/juce_graphics.h>

namespace zlgui::glass {
    // The glass itself is neutral. Colour belongs to light sources (the EQ nodes), not to
    // the material. A very slight cool bias keeps the transparent surfaces readable without
    // turning the interface into an inherently blue or rainbow object.
    inline juce::Colour canvasTop() { return juce::Colour(57, 59, 62); }
    inline juce::Colour canvasMid() { return juce::Colour(38, 40, 43); }
    inline juce::Colour canvasBottom() { return juce::Colour(24, 26, 29); }
    inline juce::Colour shellTop() { return juce::Colour(58, 60, 63); }
    inline juce::Colour shellBottom() { return juce::Colour(25, 27, 30); }
    inline juce::Colour textPrimary() { return juce::Colour(246, 250, 254); }
    inline juce::Colour textSecondary() { return textPrimary().withAlpha(.66f); }
    inline juce::Colour textTertiary() { return textPrimary().withAlpha(.42f); }
    inline juce::Colour rim() { return juce::Colour(246, 251, 255).withAlpha(.19f); }
    inline juce::Colour rimStrong() { return juce::Colour(252, 254, 255).withAlpha(.31f); }
    inline juce::Colour gridMajor() { return juce::Colour(226, 237, 244).withAlpha(.058f); }
    inline juce::Colour gridMinor() { return juce::Colour(226, 237, 244).withAlpha(.026f); }
    inline juce::Colour neutralResponse() { return juce::Colour(232, 241, 247); }

    inline juce::Colour surfaceTop(const float alpha = .11f) {
        return juce::Colour(235, 236, 238).withAlpha(alpha);
    }
    inline juce::Colour surfaceBottom(const float alpha = .19f) {
        return juce::Colour(32, 34, 37).withAlpha(alpha);
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

            // Smoked neutral body. It adds density but does not invent hue.
            g.setColour(juce::Colour(15, 17, 20).withAlpha(bottomAlpha * .58f));
            g.fillRect(bounds.expanded(1.f));

            juce::ColourGradient fill(surfaceTop(topAlpha), bounds.getCentreX(), bounds.getY(),
                                      surfaceBottom(bottomAlpha), bounds.getCentreX(), bounds.getBottom(), false);
            fill.addColour(.46, juce::Colour(124, 126, 129)
                                      .withAlpha((topAlpha + bottomAlpha) * .16f));
            g.setGradientFill(fill);
            g.fillRect(bounds.expanded(1.f));

            // Colourless specular/transmitted reflection. Any actual colour visible through
            // this surface must come from whatever is behind it — notably the live EQ nodes.
            juce::ColourGradient transmitted(
                juce::Colour(240, 248, 252).withAlpha(topAlpha * .28f),
                bounds.getX() + bounds.getWidth() * .12f,
                bounds.getY() + bounds.getHeight() * .04f,
                juce::Colours::transparentBlack,
                bounds.getX() + bounds.getWidth() * .68f,
                bounds.getY() + bounds.getHeight() * .82f,
                true);
            transmitted.addColour(.34, juce::Colour(220, 222, 225).withAlpha(topAlpha * .060f));
            g.setGradientFill(transmitted);
            g.fillRect(bounds.expanded(radius * .12f));
        }

        g.setColour(juce::Colour(248, 252, 255).withAlpha(rimAlpha));
        g.drawRoundedRectangle(bounds, radius, .82f);

        // Two displaced contours make the thickness of the glass visible even when
        // every EQ light is off. They are neutral specular reflections, not hue.
        auto inner = bounds.reduced(1.8f);
        juce::ColourGradient bevel(juce::Colour(255, 255, 255).withAlpha(rimAlpha * .55f),
                                   inner.getCentreX(), inner.getY(),
                                   juce::Colour(255, 255, 255).withAlpha(rimAlpha * .16f),
                                   inner.getCentreX(), inner.getBottom(), false);
        bevel.addColour(.40, juce::Colours::transparentWhite);
        g.setGradientFill(bevel);
        g.drawRoundedRectangle(inner, juce::jmax(1.f, radius - 1.8f), .82f);
        g.setColour(juce::Colour(4, 6, 8).withAlpha(rimAlpha * .42f));
        g.drawRoundedRectangle(bounds.reduced(3.1f), juce::jmax(1.f, radius - 3.1f), .64f);

        auto highlight = bounds.reduced(1.1f);
        g.setColour(juce::Colour(255, 255, 255).withAlpha(rimAlpha * .48f));
        g.drawLine(highlight.getX() + radius * .66f, highlight.getY() + .5f,
                   highlight.getRight() - radius * .66f, highlight.getY() + .5f, .74f);
    }
}
