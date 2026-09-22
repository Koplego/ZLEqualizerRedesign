// Glass EQ shared ambient-light helpers
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
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

        // Keep the core restrained but let saturated colour travel a long way through the
        // material. The screenshot pass showed that the previous tail disappeared before
        // it reached the header/footer, leaving the UI blue-grey instead of optically linked.
        juce::ColourGradient ambient(
            colour.interpolatedWith(juce::Colours::white, .018f).withAlpha(alpha),
            source.x, source.y,
            colour.withAlpha(0.f),
            source.x + radius, source.y, true);
        ambient.addColour(.18, colour.withAlpha(alpha * .97f));
        ambient.addColour(.40, colour.withAlpha(alpha * .78f));
        ambient.addColour(.64, colour.withAlpha(alpha * .51f));
        ambient.addColour(.82, colour.withAlpha(alpha * .27f));
        ambient.addColour(.94, colour.withAlpha(alpha * .105f));
        ambient.addColour(.988, colour.withAlpha(alpha * .018f));
        g.setGradientFill(ambient);
        g.fillEllipse(source.x - radius, source.y - radius, radius * 2.f, radius * 2.f);
    }

    inline void paintAmbientSources(juce::Graphics& g,
                                    const std::vector<AmbientLightSource>& sources,
                                    const juce::Rectangle<float> clip_bounds,
                                    const float alpha_scale) {
        // Receiver panels are painted after the parent shell, so their material needs a
        // little more transmission than the shell itself. sqrt() also lets unselected bands
        // remain visible as colour contributors without making the selected band brighter.
        constexpr float kReceiverTransmission = 2.60f;
        for (const auto& source : sources) {
            if (source.strength <= .0001f) continue;
            const auto perceptual_strength = std::sqrt(juce::jmax(0.f, source.strength));
            paintAmbientField(g, source.point, source.colour, source.radius,
                              alpha_scale * kReceiverTransmission * perceptual_strength,
                              clip_bounds);
        }
    }
}
