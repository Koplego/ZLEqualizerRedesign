#pragma once

#include <juce_graphics/juce_graphics.h>

namespace zlgui::glass {
    inline juce::Colour canvasTop() { return juce::Colour(27, 58, 84); }
    inline juce::Colour canvasMid() { return juce::Colour(17, 43, 65); }
    inline juce::Colour canvasBottom() { return juce::Colour(10, 28, 44); }
    inline juce::Colour shellTop() { return juce::Colour(47, 78, 103); }
    inline juce::Colour shellBottom() { return juce::Colour(15, 35, 51); }
    inline juce::Colour textPrimary() { return juce::Colour(244, 249, 253); }
    inline juce::Colour textSecondary() { return textPrimary().withAlpha(.60f); }
    inline juce::Colour textTertiary() { return textPrimary().withAlpha(.34f); }
    inline juce::Colour rim() { return juce::Colour(246, 251, 255).withAlpha(.16f); }
    inline juce::Colour rimStrong() { return juce::Colour(252, 254, 255).withAlpha(.26f); }
    inline juce::Colour gridMajor() { return juce::Colour(218, 236, 248).withAlpha(.050f); }
    inline juce::Colour gridMinor() { return juce::Colour(218, 236, 248).withAlpha(.022f); }
    inline juce::Colour neutralResponse() { return juce::Colour(221, 240, 253); }

    inline juce::Colour surfaceTop(const float alpha = .16f) {
        return juce::Colour(239, 248, 253).withAlpha(alpha);
    }
    inline juce::Colour surfaceBottom(const float alpha = .26f) {
        return juce::Colour(43, 72, 94).withAlpha(alpha);
    }

    inline float shellRadius(const float font) { return juce::jmax(18.f, font * 1.45f); }
    inline float surfaceRadius(const float font) { return juce::jmax(11.f, font * .95f); }

    // The agreed reference is unmistakably blue glass before any band light reaches it. Keep
    // that cool depth, but leave enough neutrality that amber, mint and violet can still tint
    // the material strongly. The nodes provide the spatial hue; this substrate provides depth.
    inline void fillGlassSurface(juce::Graphics& g, juce::Rectangle<float> bounds,
                                 const float radius, const float topAlpha = .13f,
                                 const float bottomAlpha = .22f, const float rimAlpha = .18f) {
        juce::Path clip;
        clip.addRoundedRectangle(bounds, radius);
        {
            juce::Graphics::ScopedSaveState state(g);
            g.reduceClipRegion(clip);

            g.setColour(juce::Colour(8, 23, 37).withAlpha(bottomAlpha * .42f));
            g.fillRect(bounds.expanded(1.f));

            juce::ColourGradient fill(surfaceTop(topAlpha), bounds.getCentreX(), bounds.getY(),
                                      surfaceBottom(bottomAlpha), bounds.getCentreX(), bounds.getBottom(), false);
            fill.addColour(.48, juce::Colour(108, 151, 181).withAlpha((topAlpha + bottomAlpha) * .21f));
            g.setGradientFill(fill);
            g.fillRect(bounds.expanded(1.f));

            juce::ColourGradient transmitted(
                juce::Colour(214, 238, 253).withAlpha(topAlpha * .45f),
                bounds.getX() + bounds.getWidth() * .13f,
                bounds.getY() + bounds.getHeight() * .08f,
                juce::Colours::transparentBlack,
                bounds.getX() + bounds.getWidth() * .74f,
                bounds.getY() + bounds.getHeight() * .88f,
                true);
            transmitted.addColour(.43, juce::Colour(119, 171, 205).withAlpha(topAlpha * .10f));
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
