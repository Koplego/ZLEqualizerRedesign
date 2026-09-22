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
        ambient.addColour(.14, colour.withAlpha(alpha * .995f));
        ambient.addColour(.32, colour.withAlpha(alpha * .92f));
        ambient.addColour(.52, colour.withAlpha(alpha * .74f));
        ambient.addColour(.70, colour.withAlpha(alpha * .49f));
        ambient.addColour(.85, colour.withAlpha(alpha * .27f));
        ambient.addColour(.95, colour.withAlpha(alpha * .115f));
        ambient.addColour(.992, colour.withAlpha(alpha * .020f));
        g.setGradientFill(ambient);
        g.fillEllipse(source.x - radius, source.y - radius, radius * 2.f, radius * 2.f);
    }

    inline void paintBlendedAmbientStrip(juce::Graphics& g,
                                         const std::vector<AmbientLightSource>& sources,
                                         const juce::Rectangle<float> clip_bounds,
                                         const float alpha_scale) {
        if (sources.empty() || clip_bounds.isEmpty()) return;

        // Match the agreed reference perceptually, not merely numerically. The reference has
        // unmistakable amber/mint/blue/violet regions in the header and footer. Earlier passes
        // were still below that threshold. Keep the lamp colours saturated, give them a wide
        // ambient tail, and make the receiving glass responsive enough that those zones remain
        // visible far from the graph without whitening into neon.
        constexpr int kSamples = 49;
        constexpr float kReceiverTransmission = 15.5f;

        auto sampleColour = [&](const float portion) {
            const auto x = clip_bounds.getX() + clip_bounds.getWidth() * portion;
            float total = 0.f;
            float red = 0.f, green = 0.f, blue = 0.f;
            float dominant_weight = 0.f;
            juce::Colour dominant = juce::Colours::transparentBlack;

            for (const auto& source : sources) {
                if (source.strength <= .0001f || source.radius <= 1.f) continue;

                // Every enabled band remains a real lamp. Selection changes emphasis only.
                const auto perceptual_strength = .74f + .26f *
                    std::pow(juce::jlimit(0.f, 1.f, source.strength), .40f);

                // Wide shelves should always receive some of the room light. The reference
                // clearly shows node colour in both header and footer, even from distant nodes.
                const auto vertical_distance = std::abs(source.point.y - clip_bounds.getCentreY());
                const auto vertical_reach = source.radius * 3.1f;
                const auto vertical_t = juce::jlimit(0.f, 1.f, vertical_distance / vertical_reach);
                const auto vertical_gain = .72f + .28f *
                    std::pow(juce::jmax(0.f, 1.f - vertical_t), .22f);

                const auto core_reach = juce::jmax(clip_bounds.getHeight() * 3.2f,
                                                   clip_bounds.getWidth() * .115f);
                const auto tail_reach = juce::jmax(clip_bounds.getHeight() * 8.5f,
                                                   clip_bounds.getWidth() * .46f);
                const auto core_dx = (x - source.point.x) / core_reach;
                const auto tail_dx = (x - source.point.x) / tail_reach;
                const auto core = std::exp(-1.70f * core_dx * core_dx);
                const auto tail = std::exp(-1.02f * tail_dx * tail_dx);
                const auto horizontal_gain = .84f * core + .16f * tail;
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
            // Preserve the nearest lamp aggressively enough to keep the spatial colour zones
            // readable when long tails overlap.
            mixed = mixed.interpolatedWith(dominant, .58f);

            const auto occupancy = 1.f - std::exp(-total * 1.22f);
            const auto alpha = juce::jlimit(0.f, .34f,
                alpha_scale * kReceiverTransmission * (.44f + .56f * occupancy));
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

        constexpr float kReceiverTransmission = 4.5f;
        for (const auto& source : sources) {
            if (source.strength <= .0001f) continue;
            const auto perceptual_strength = .58f + .42f *
                std::pow(juce::jlimit(0.f, 1.f, source.strength), .40f);
            const auto alpha = alpha_scale * kReceiverTransmission * perceptual_strength;
            paintAmbientField(g, source.point, source.colour, source.radius, alpha, clip_bounds);
        }
    }
}
