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

    inline void paintBlendedAmbientStrip(juce::Graphics& g,
                                         const std::vector<AmbientLightSource>& sources,
                                         const juce::Rectangle<float> clip_bounds,
                                         const float alpha_scale) {
        if (sources.empty() || clip_bounds.isEmpty()) return;

        // Runtime screenshots showed that the previous receiver math changed numerically but
        // remained below the perceptual threshold: the header/footer still read as one blue
        // slab. Use two spatial scales instead. A stronger local lobe preserves the actual
        // lamp colour near its x-position, while a low-energy tail keeps the YouTube-style
        // ambient spread across the rest of the glass. The lamps themselves are not brighter;
        // the receiving material is simply more responsive to the same light.
        constexpr int kSamples = 33;
        constexpr float kReceiverTransmission = 5.4f;

        auto sampleColour = [&](const float portion) {
            const auto x = clip_bounds.getX() + clip_bounds.getWidth() * portion;
            float total = 0.f;
            float red = 0.f, green = 0.f, blue = 0.f;
            float dominant_weight = 0.f;
            juce::Colour dominant = juce::Colours::transparentBlack;

            for (const auto& source : sources) {
                if (source.strength <= .0001f || source.radius <= 1.f) continue;

                const auto perceptual_strength = std::pow(juce::jmax(0.f, source.strength), .36f);
                const auto vertical_distance = std::abs(source.point.y - clip_bounds.getCentreY());
                const auto vertical_reach = source.radius * 2.55f;
                const auto vertical_t = juce::jlimit(0.f, 1.f, vertical_distance / vertical_reach);
                const auto vertical_gain = std::pow(juce::jmax(0.f, 1.f - vertical_t), .28f);
                if (vertical_gain <= .001f) continue;

                const auto core_reach = juce::jmax(clip_bounds.getHeight() * 4.8f,
                                                   source.radius * .46f);
                const auto tail_reach = juce::jmax(clip_bounds.getHeight() * 9.0f,
                                                   source.radius * 1.22f);
                const auto core_dx = (x - source.point.x) / core_reach;
                const auto tail_dx = (x - source.point.x) / tail_reach;
                const auto core = std::exp(-1.55f * core_dx * core_dx);
                const auto tail = std::exp(-1.15f * tail_dx * tail_dx);
                const auto horizontal_gain = .78f * core + .22f * tail;
                const auto weight = perceptual_strength * vertical_gain * horizontal_gain;
                if (weight <= .00001f) continue;

                total += weight;
                red += source.colour.getFloatRed() * weight;
                green += source.colour.getFloatGreen() * weight;
                blue += source.colour.getFloatBlue() * weight;
                if (weight > dominant_weight) {
                    dominant_weight = weight;
                    dominant = source.colour;
                }
            }

            if (total <= .0001f) return juce::Colours::transparentBlack;

            auto mixed = juce::Colour::fromFloatRGBA(red / total, green / total, blue / total, 1.f);
            // Keep local hues legible when several long tails overlap. This is intentionally
            // colour-preserving rather than whitening the glow, so it stays stained-glass-like.
            mixed = mixed.interpolatedWith(dominant, .30f);

            const auto occupancy = 1.f - std::exp(-total * 1.10f);
            const auto alpha = juce::jlimit(0.f, .13f,
                alpha_scale * kReceiverTransmission * (.34f + .66f * occupancy));
            return mixed.withAlpha(alpha);
        };

        juce::Graphics::ScopedSaveState state(g);
        g.reduceClipRegion(clip_bounds.toNearestInt());

        juce::ColourGradient spread(sampleColour(0.f), clip_bounds.getX(), clip_bounds.getCentreY(),
                                    sampleColour(1.f), clip_bounds.getRight(), clip_bounds.getCentreY(), false);
        for (int i = 1; i < kSamples - 1; ++i) {
            const auto t = static_cast<float>(i) / static_cast<float>(kSamples - 1);
            spread.addColour(t, sampleColour(t));
        }
        g.setGradientFill(spread);
        g.fillRect(clip_bounds);
    }

    inline void paintAmbientSources(juce::Graphics& g,
                                    const std::vector<AmbientLightSource>& sources,
                                    const juce::Rectangle<float> clip_bounds,
                                    const float alpha_scale) {
        const auto wide_receiver = clip_bounds.getWidth() > clip_bounds.getHeight() * 4.f;
        if (wide_receiver) {
            paintBlendedAmbientStrip(g, sources, clip_bounds, alpha_scale);
            return;
        }

        constexpr float kReceiverTransmission = 2.60f;
        for (const auto& source : sources) {
            if (source.strength <= .0001f) continue;
            const auto perceptual_strength = std::pow(juce::jmax(0.f, source.strength), .36f);
            const auto alpha = alpha_scale * kReceiverTransmission * perceptual_strength;
            paintAmbientField(g, source.point, source.colour, source.radius, alpha, clip_bounds);
        }
    }
}
