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

        // Wide/shallow glass should behave more like YouTube ambient lighting than a row of
        // spotlights. The source intensity is unchanged; only the reach is extended so the
        // colour survives the vertical trip from graph to header/footer and spreads gently
        // across neighbouring controls before fading away.
        const auto vertical_distance = std::abs(source.point.y - clip_bounds.getCentreY());
        const auto vertical_reach = source.radius * 1.62f;
        const auto t = juce::jlimit(0.f, 1.f, vertical_distance / vertical_reach);
        const auto vertical_gain = std::pow(juce::jmax(0.f, 1.f - t), .46f);
        if (vertical_gain <= .001f) return;

        const auto half_width = juce::jmax(clip_bounds.getHeight() * 4.2f,
                                          source.radius * .68f);
        const auto effective_alpha = alpha * vertical_gain;

        juce::Graphics::ScopedSaveState state(g);
        g.reduceClipRegion(clip_bounds.toNearestInt());

        juce::ColourGradient spread(
            source.colour.withAlpha(0.f), source.point.x - half_width, 0.f,
            source.colour.withAlpha(0.f), source.point.x + half_width, 0.f, false);
        spread.addColour(.035, source.colour.withAlpha(effective_alpha * .018f));
        spread.addColour(.12, source.colour.withAlpha(effective_alpha * .075f));
        spread.addColour(.23, source.colour.withAlpha(effective_alpha * .24f));
        spread.addColour(.36, source.colour.withAlpha(effective_alpha * .63f));
        spread.addColour(.50, source.colour.interpolatedWith(juce::Colours::white, .010f)
                                      .withAlpha(effective_alpha));
        spread.addColour(.64, source.colour.withAlpha(effective_alpha * .63f));
        spread.addColour(.77, source.colour.withAlpha(effective_alpha * .24f));
        spread.addColour(.88, source.colour.withAlpha(effective_alpha * .075f));
        spread.addColour(.965, source.colour.withAlpha(effective_alpha * .018f));
        g.setGradientFill(spread);
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
