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

        // Keep the source itself restrained and let the colour survive far into the glass.
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

    inline void paintAmbientStripField(juce::Graphics& g,
                                       const AmbientLightSource& source,
                                       const juce::Rectangle<float> clip_bounds,
                                       const float alpha) {
        if (alpha <= .0001f || source.radius <= 1.f || clip_bounds.isEmpty()) return;

        // The runtime reference showed the right amount of light in the graph but too little
        // colour reaching the header/footer. Keep the peak energy essentially unchanged and
        // split it into a broad ambient tail plus a softer local field. This gives the same
        // YouTube-style sense of room illumination without turning the bars into neon.
        const auto vertical_distance = std::abs(source.point.y - clip_bounds.getCentreY());
        const auto vertical_reach = source.radius * 2.15f;
        const auto t = juce::jlimit(0.f, 1.f, vertical_distance / vertical_reach);
        const auto vertical_gain = std::pow(juce::jmax(0.f, 1.f - t), .34f);
        if (vertical_gain <= .001f) return;

        const auto core_half_width = juce::jmax(clip_bounds.getHeight() * 5.0f,
                                               source.radius * .72f);
        const auto tail_half_width = juce::jmax(clip_bounds.getHeight() * 8.0f,
                                               source.radius * 1.12f);
        const auto effective_alpha = alpha * vertical_gain;

        juce::Graphics::ScopedSaveState state(g);
        g.reduceClipRegion(clip_bounds.toNearestInt());

        // Broad saturated tail: low energy, very long reach. This is what lets the coloured
        // nodes tint distant glass without increasing the apparent brightness of the source.
        juce::ColourGradient tail(
            source.colour.withAlpha(0.f), source.point.x - tail_half_width, 0.f,
            source.colour.withAlpha(0.f), source.point.x + tail_half_width, 0.f, false);
        tail.addColour(.04, source.colour.withAlpha(effective_alpha * .010f));
        tail.addColour(.16, source.colour.withAlpha(effective_alpha * .055f));
        tail.addColour(.30, source.colour.withAlpha(effective_alpha * .16f));
        tail.addColour(.50, source.colour.withAlpha(effective_alpha * .30f));
        tail.addColour(.70, source.colour.withAlpha(effective_alpha * .16f));
        tail.addColour(.84, source.colour.withAlpha(effective_alpha * .055f));
        tail.addColour(.96, source.colour.withAlpha(effective_alpha * .010f));
        g.setGradientFill(tail);
        g.fillRect(clip_bounds);

        // Local receiver field: narrower than the tail, but deliberately softer than before.
        // Tail + core still sum to roughly the old peak level while distributing much more
        // colour across the entire glass surface.
        juce::ColourGradient core(
            source.colour.withAlpha(0.f), source.point.x - core_half_width, 0.f,
            source.colour.withAlpha(0.f), source.point.x + core_half_width, 0.f, false);
        core.addColour(.05, source.colour.withAlpha(effective_alpha * .012f));
        core.addColour(.18, source.colour.withAlpha(effective_alpha * .080f));
        core.addColour(.32, source.colour.withAlpha(effective_alpha * .30f));
        core.addColour(.43, source.colour.withAlpha(effective_alpha * .57f));
        core.addColour(.50, source.colour.interpolatedWith(juce::Colours::white, .008f)
                                    .withAlpha(effective_alpha * .70f));
        core.addColour(.57, source.colour.withAlpha(effective_alpha * .57f));
        core.addColour(.68, source.colour.withAlpha(effective_alpha * .30f));
        core.addColour(.82, source.colour.withAlpha(effective_alpha * .080f));
        core.addColour(.95, source.colour.withAlpha(effective_alpha * .012f));
        g.setGradientFill(core);
        g.fillRect(clip_bounds);
    }

    inline void paintAmbientSources(juce::Graphics& g,
                                    const std::vector<AmbientLightSource>& sources,
                                    const juce::Rectangle<float> clip_bounds,
                                    const float alpha_scale) {
        constexpr float kReceiverTransmission = 2.60f;
        const auto wide_receiver = clip_bounds.getWidth() > clip_bounds.getHeight() * 4.f;

        for (const auto& source : sources) {
            if (source.strength <= .0001f) continue;

            // Preserve selected-band dominance, but allow ordinary active bands to remain
            // visible contributors. This is especially important when no band is selected.
            const auto perceptual_strength = std::pow(juce::jmax(0.f, source.strength), .36f);
            const auto alpha = alpha_scale * kReceiverTransmission * perceptual_strength;

            if (wide_receiver)
                paintAmbientStripField(g, source, clip_bounds, alpha);
            else
                paintAmbientField(g, source.point, source.colour, source.radius, alpha, clip_bounds);
        }
    }
}
