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
        // Build the material from several low-opacity optical layers. JUCE plugin
        // windows cannot sample the host window for a real backdrop blur, but these
        // offset highlights and colour bends reproduce the transmitted-light read of
        // the reference instead of looking like a flat translucent rectangle.
        {
            juce::Path clip;
            clip.addRoundedRectangle(bounds, radius);
            juce::Graphics::ScopedSaveState state(g);
            g.reduceClipRegion(clip);

        g.setColour(juce::Colour(8, 24, 38).withAlpha(bottomAlpha * .72f));
        g.fillRoundedRectangle(bounds, radius);

        juce::ColourGradient fill(surfaceTop(topAlpha), bounds.getCentreX(), bounds.getY(),
                                  surfaceBottom(bottomAlpha), bounds.getCentreX(), bounds.getBottom(), false);
        fill.addColour(.48, juce::Colour(127, 158, 181).withAlpha((topAlpha + bottomAlpha) * .36f));
        g.setGradientFill(fill);
        g.fillRoundedRectangle(bounds, radius);

        juce::ColourGradient transmitted(
            juce::Colour(188, 225, 248).withAlpha(topAlpha * .72f),
            bounds.getX() + bounds.getWidth() * .08f, bounds.getY() + bounds.getHeight() * .12f,
            juce::Colours::transparentBlack,
            bounds.getX() + bounds.getWidth() * .70f, bounds.getY() + bounds.getHeight() * .82f, true);
        transmitted.addColour(.46, juce::Colour(112, 170, 210).withAlpha(topAlpha * .24f));
        g.setGradientFill(transmitted);
        g.fillRect(bounds.expanded(radius * .35f));

        auto caustic = bounds.withSizeKeepingCentre(bounds.getWidth() * .70f,
                                                     juce::jmax(2.f, bounds.getHeight() * .16f));
        caustic.translate(-bounds.getWidth() * .12f, -bounds.getHeight() * .25f);
        juce::ColourGradient causticGradient(
            juce::Colour(255, 255, 255).withAlpha(topAlpha * .52f), caustic.getCentreX(), caustic.getCentreY(),
            juce::Colours::transparentBlack, caustic.getRight(), caustic.getCentreY(), true);
            g.setGradientFill(causticGradient);
            g.fillEllipse(caustic);
        }
        g.setColour(juce::Colour(248, 252, 255).withAlpha(rimAlpha));
        g.drawRoundedRectangle(bounds, radius, .85f);
        g.setColour(juce::Colour(8, 23, 38).withAlpha(rimAlpha * .78f));
        g.drawRoundedRectangle(bounds.reduced(1.35f), juce::jmax(1.f, radius - 1.35f), .65f);
        auto highlight = bounds.reduced(1.1f);
        g.setColour(juce::Colour(255, 255, 255).withAlpha(rimAlpha * .58f));
        g.drawLine(highlight.getX() + radius * .62f, highlight.getY() + .5f,
                   highlight.getRight() - radius * .62f, highlight.getY() + .5f, .8f);
    }
}
