#pragma once

#include <juce_graphics/juce_graphics.h>

namespace zlgui::glass {
    inline juce::Colour canvasTop() { return juce::Colour(18, 45, 70); }
    inline juce::Colour canvasMid() { return juce::Colour(10, 31, 51); }
    inline juce::Colour canvasBottom() { return juce::Colour(5, 18, 31); }
    inline juce::Colour shellTop() { return juce::Colour(31, 63, 89); }
    inline juce::Colour shellBottom() { return juce::Colour(8, 25, 41); }
    inline juce::Colour textPrimary() { return juce::Colour(244, 249, 253); }
    inline juce::Colour textSecondary() { return textPrimary().withAlpha(.60f); }
    inline juce::Colour textTertiary() { return textPrimary().withAlpha(.34f); }
    inline juce::Colour rim() { return juce::Colour(246, 251, 255).withAlpha(.14f); }
    inline juce::Colour rimStrong() { return juce::Colour(252, 254, 255).withAlpha(.22f); }
    inline juce::Colour gridMajor() { return juce::Colour(210, 232, 247).withAlpha(.043f); }
    inline juce::Colour gridMinor() { return juce::Colour(210, 232, 247).withAlpha(.018f); }
    inline juce::Colour neutralResponse() { return juce::Colour(218, 239, 253); }

    inline juce::Colour surfaceTop(const float alpha = .13f) {
        return juce::Colour(151, 197, 229).withAlpha(alpha);
    }
    inline juce::Colour surfaceBottom(const float alpha = .22f) {
        return juce::Colour(23, 52, 76).withAlpha(alpha);
    }

    inline float shellRadius(const float font) { return juce::jmax(18.f, font * 1.45f); }
    inline float surfaceRadius(const float font) { return juce::jmax(11.f, font * .95f); }

    inline void fillGlassSurface(juce::Graphics& g, juce::Rectangle<float> bounds,
                                 const float radius, const float topAlpha = .11f,
                                 const float bottomAlpha = .19f, const float rimAlpha = .16f) {
        juce::Path clip;
        clip.addRoundedRectangle(bounds, radius);
        {
            juce::Graphics::ScopedSaveState state(g);
            g.reduceClipRegion(clip);

            g.setColour(juce::Colour(4, 17, 29).withAlpha(bottomAlpha * .58f));
            g.fillRect(bounds.expanded(1.f));

            juce::ColourGradient fill(surfaceTop(topAlpha), bounds.getCentreX(), bounds.getY(),
                                      surfaceBottom(bottomAlpha), bounds.getCentreX(), bounds.getBottom(), false);
            fill.addColour(.48, juce::Colour(53, 98, 132)
                                      .withAlpha((topAlpha + bottomAlpha) * .15f));
            g.setGradientFill(fill);
            g.fillRect(bounds.expanded(1.f));

            juce::ColourGradient transmitted(
                juce::Colour(207, 231, 247).withAlpha(topAlpha * .22f),
                bounds.getX() + bounds.getWidth() * .13f,
                bounds.getY() + bounds.getHeight() * .07f,
                juce::Colours::transparentBlack,
                bounds.getX() + bounds.getWidth() * .64f,
                bounds.getY() + bounds.getHeight() * .78f,
                true);
            transmitted.addColour(.38, juce::Colour(85, 146, 187).withAlpha(topAlpha * .050f));
            g.setGradientFill(transmitted);
            g.fillRect(bounds.expanded(radius * .12f));
        }

        g.setColour(juce::Colour(248, 252, 255).withAlpha(rimAlpha));
        g.drawRoundedRectangle(bounds, radius, .78f);

        auto highlight = bounds.reduced(1.1f);
        g.setColour(juce::Colour(255, 255, 255).withAlpha(rimAlpha * .38f));
        g.drawLine(highlight.getX() + radius * .66f, highlight.getY() + .5f,
                   highlight.getRight() - radius * .66f, highlight.getY() + .5f, .70f);
    }
}
