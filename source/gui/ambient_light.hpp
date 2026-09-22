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

        // A header/footer should read as one piece of glass receiving several lamps, not as
        // several translucent gradients stacked on top of each other. Stacking caused four
        // band colours to average into the same blue-grey. Instead, sample the receiver along
        // its width, blend the nearby lamps first, then tint the glass once. This preserves a
        // warm left side, cool centre and violet right side while still letting each source
        // spread far beyond its own x-position.
        constexpr int kSamples = 17;
        constexpr float kReceiverTransmission = 3.0f;

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
                const auto vertical_reach = source.radius * 2.35f;
                const auto vertical_t = juce::jlimit(0.f, 1.f, vertical_distance / vertical_reach);
                const auto vertical_gain = std::pow(juce::jmax(0.f, 1.f - vertical_t), .31f);
                if (vertical_gain <= .001f) continue;

                // Wide Gaussian-style shoulder: long ambient tail, but enough localisation
                // that neighbouring colours remain spatially legible instead of turning grey.
                const auto horizontal_reach = juce::jmax(clip_bounds.getHeight() * 6.0f,
                                                         source.radius * .78f);
                const auto dx = (x - source.point.x) / horizontal_reach;
                const auto horizontal_gain = std::exp(-1.42f * dx * dx);
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
            // Retain a little of the nearest lamp's hue so overlapping sources still feel like
            // coloured light rather than a desaturated average.
            mixed = mixed.interpolatedWith(dominant, .18f);

            const auto occupancy = 1.f - std::exp(-total * .92f);
            const auto alpha = juce::jlimit(0.f, .085f,
                alpha_scale * kReceiverTransmission * (.42f + .58f * occupancy));
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
