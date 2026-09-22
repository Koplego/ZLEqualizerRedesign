#pragma once

#include <juce_graphics/juce_graphics.h>

namespace zlgui::glass {
    // The reference lives much deeper in the blue channel than the previous pass.  Keep
    // these base colours deliberately saturated so translucent surfaces do not turn the
    // editor into a grey/milky sheet once they are composited on top of the shell.
    inline juce::Colour canvasTop() { return juce::Colour(21, 52, 78); }
    inline juce::Colour canvasMid() { return juce::Colour(13, 38, 60); }
    inline juce::Colour canvasBottom() { return juce::Colour(6, 20, 34); }
    inline juce::Colour shellTop() { return juce::Colour(37, 69, 94); }
    inline juce::Colour shellBottom() { return juce::Colour(9, 27, 44); }
    inline juce::Colour textPrimary() { return juce::Colour(244, 249, 253); }
    inline juce::Colour textSecondary() { return textPrimary().withAlpha(.60f); }
    inline juce::Colour textTertiary() { return textPrimary().withAlpha(.34f); }
    inline juce::Colour rim() { return juce::Colour(246, 251, 255).withAlpha(.16f); }
    inline juce::Colour rimStrong() { return juce::Colour(252, 254, 255).withAlpha(.26f); }
    inline juce::Colour gridMajor() { return juce::Colour(210, 232, 247).withAlpha(.047f); }
    inline juce::Colour gridMinor() { return juce::Colour(210, 232, 247).withAlpha(.020f); }
    inline juce::Colour neutralResponse() { return juce::Colour(218, 239, 253); }

    inline juce::Colour surfaceTop(const float alpha = .16f) {
        return juce::Colour(173, 214, 241).withAlpha(alpha);
    }
    inline juce::Colour surfaceBottom(const float alpha = .26f) {
        return juce::Colour(31, 64, 89).withAlpha(alpha);
    }

    inline float shellRadius(const float font) { return juce::jmax(18.f, font * 1.45f); }
    inline float surfaceRadius(const float font) { return juce::jmax(11.f, font * .95f); }

    // Reference-match glass: dark blue material first, optical highlight second.  The old
    // version leaned too far toward white which made the complete interface look fogged.
    // Keep the transmitted highlight narrow and cool so the shell underneath remains visible.
    inline void fillGlassSurface(juce::Graphics& g, juce::Rectangle<float> bounds,
                                 const float radius, const float topAlpha = .13f,
                                 const float bottomAlpha = .22f, const float rimAlpha = .18f) {
        juce::Path clip;
        clip.addRoundedRectangle(bounds, radius);
        {
            juce::Graphics::ScopedSaveState state(g);
            g.reduceClipRegion(clip);

            g.setColour(juce::Colour(5, 20, 34).withAlpha(bottomAlpha * .52f));
            g.fillRect(bounds.expanded(1.f));

            juce::ColourGradient fill(surfaceTop(topAlpha), bounds.getCentreX(), bounds.getY(),
                                      surfaceBottom(bottomAlpha), bounds.getCentreX(), bounds.getBottom(), false);
            fill.addColour(.48, juce::Colour(65, 111, 146)
                                      .withAlpha((topAlpha + bottomAlpha) * .18f));
            g.setGradientFill(fill);
            g.fillRect(bounds.expanded(1.f));

            juce::ColourGradient transmitted(
                juce::Colour(205, 232, 249).withAlpha(topAlpha * .32f),
                bounds.getX() + bounds.getWidth() * .13f,
                bounds.getY() + bounds.getHeight() * .07f,
                juce::Colours::transparentBlack,
                bounds.getX() + bounds.getWidth() * .69f,
                bounds.getY() + bounds.getHeight() * .82f,
                true);
            transmitted.addColour(.40, juce::Colour(96, 158, 199).withAlpha(topAlpha * .075f));
            g.setGradientFill(transmitted);
            g.fillRect(bounds.expanded(radius * .16f));
        }

        g.setColour(juce::Colour(248, 252, 255).withAlpha(rimAlpha));
        g.drawRoundedRectangle(bounds, radius, .8f);

        auto highlight = bounds.reduced(1.1f);
        g.setColour(juce::Colour(255, 255, 255).withAlpha(rimAlpha * .46f));
        g.drawLine(highlight.getX() + radius * .66f, highlight.getY() + .5f,
                   highlight.getRight() - radius * .66f, highlight.getY() + .5f, .75f);
    }
}
