#pragma once

#include <juce_graphics/juce_graphics.h>

namespace zlgui::glass {
    inline juce::Colour canvasTop() { return juce::Colour(27, 51, 72); }
    inline juce::Colour canvasMid() { return juce::Colour(19, 40, 59); }
    inline juce::Colour canvasBottom() { return juce::Colour(11, 27, 42); }
    inline juce::Colour shellTop() { return juce::Colour(44, 62, 76); }
    inline juce::Colour shellBottom() { return juce::Colour(18, 31, 43); }
    inline juce::Colour textPrimary() { return juce::Colour(244, 249, 253); }
    inline juce::Colour textSecondary() { return textPrimary().withAlpha(.60f); }
    inline juce::Colour textTertiary() { return textPrimary().withAlpha(.34f); }
    inline juce::Colour rim() { return juce::Colour(246, 251, 255).withAlpha(.16f); }
    inline juce::Colour rimStrong() { return juce::Colour(252, 254, 255).withAlpha(.26f); }
    inline juce::Colour gridMajor() { return juce::Colour(218, 236, 248).withAlpha(.050f); }
    inline juce::Colour gridMinor() { return juce::Colour(218, 236, 248).withAlpha(.022f); }
    inline juce::Colour neutralResponse() { return juce::Colour(221, 240, 253); }

    inline juce::Colour surfaceTop(const float alpha = .16f) {
        return juce::Colour(240, 247, 251).withAlpha(alpha);
    }
    inline juce::Colour surfaceBottom(const float alpha = .26f) {
        return juce::Colour(49, 62, 73).withAlpha(alpha);
    }

    inline float shellRadius(const float font) { return juce::jmax(18.f, font * 1.45f); }
    inline float surfaceRadius(const float font) { return juce::jmax(11.f, font * .95f); }

    // Glass EQ uses a deliberately neutral-cool substrate. The band nodes provide the hue;
    // the material itself should not be so blue that amber, mint and violet reflections are
    // swallowed before they become visible. Depth still comes from translucency and rims.
    inline void fillGlassSurface(juce::Graphics& g, juce::Rectangle<float> bounds,
                                 const float radius, const float topAlpha = .13f,
                                 const float bottomAlpha = .22f, const float rimAlpha = .18f) {
        juce::Path clip;
        clip.addRoundedRectangle(bounds, radius);
        {
            juce::Graphics::ScopedSaveState state(g);
            g.reduceClipRegion(clip);

            g.setColour(juce::Colour(10, 20, 29).withAlpha(bottomAlpha * .46f));
            g.fillRect(bounds.expanded(1.f));

            juce::ColourGradient fill(surfaceTop(topAlpha), bounds.getCentreX(), bounds.getY(),
                                      surfaceBottom(bottomAlpha), bounds.getCentreX(), bounds.getBottom(), false);
            fill.addColour(.48, juce::Colour(126, 145, 160).withAlpha((topAlpha + bottomAlpha) * .20f));
            g.setGradientFill(fill);
            g.fillRect(bounds.expanded(1.f));

            juce::ColourGradient transmitted(
                juce::Colour(222, 235, 243).withAlpha(topAlpha * .42f),
                bounds.getX() + bounds.getWidth() * .13f,
                bounds.getY() + bounds.getHeight() * .08f,
                juce::Colours::transparentBlack,
                bounds.getX() + bounds.getWidth() * .74f,
                bounds.getY() + bounds.getHeight() * .88f,
                true);
            transmitted.addColour(.43, juce::Colour(139, 159, 173).withAlpha(topAlpha * .09f));
            g.setGradientFill(transmitted);
            g.fillRect(bounds.expanded(radius * .18f));
        }

        g.setColour(juce::Colour(248, 252, 255).withAlpha(rimAlpha));
        g.drawRoundedRectangle(bounds, radius, .8f);

        auto highlight = bounds.reduced(1.1f);
        g.setColour(juce::Colour(255, 255, 255).withAlpha(rimAlpha * .52f));
        g.drawLine(highlight.getX() + radius * .66f, highlight.getY() + .5f,
                   highlight.getRight() - radius * .66f, highlight.getY() + .5f, .75f);
    }
}
