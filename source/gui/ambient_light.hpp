// Glass EQ shared ambient-light helpers
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace zlgui::glass {
    struct AmbientLightSource {
        juce::Point<float> point{};
        juce::Colour colour{juce::Colours::transparentBlack};
        float strength{0.f};
        float radius{0.f};
    };

    inline void paintAmbientField(juce::Graphics& g,
                                  const juce::Point<float> source,
                                  const juce::Colour colour,
                                  const float radius,
                                  const float alpha,
                                  const juce::Rectangle<float> clip_bounds) {
        if (alpha <= .0001f || radius <= 1.f || clip_bounds.isEmpty()) return;

        juce::Graphics::ScopedSaveState state(g);
        g.reduceClipRegion(clip_bounds.toNearestInt());

        // The reference is driven by reach rather than peak brightness. Keep the source
        // restrained and let saturated colour linger far into the surrounding glass.
        juce::ColourGradient ambient(
            colour.interpolatedWith(juce::Colours::white, .025f).withAlpha(alpha),
            source.x, source.y,
            colour.withAlpha(0.f),
            source.x + radius, source.y, true);
        ambient.addColour(.22, colour.withAlpha(alpha * .92f));
        ambient.addColour(.48, colour.withAlpha(alpha * .60f));
        ambient.addColour(.72, colour.withAlpha(alpha * .30f));
        ambient.addColour(.90, colour.withAlpha(alpha * .09f));
        ambient.addColour(.985, colour.withAlpha(alpha * .012f));
        g.setGradientFill(ambient);
        g.fillEllipse(source.x - radius, source.y - radius, radius * 2.f, radius * 2.f);
    }

    inline void paintAmbientSources(juce::Graphics& g,
                                    const std::vector<AmbientLightSource>& sources,
                                    const juce::Rectangle<float> clip_bounds,
                                    const float alpha_scale) {
        for (const auto& source : sources) {
            if (source.strength <= .0001f) continue;
            paintAmbientField(g, source.point, source.colour, source.radius,
                              alpha_scale * source.strength, clip_bounds);
        }
    }
}
