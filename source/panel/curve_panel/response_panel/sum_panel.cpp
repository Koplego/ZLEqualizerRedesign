// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#include "sum_panel.hpp"
#include "../../../gui/glass_tokens.hpp"
#include <cmath>

namespace zlpanel {
    SumPanel::SumPanel(PluginProcessor& p, zlgui::UIBase& base) :
        p_ref_(p), base_(base) {
        juce::ignoreUnused(p_ref_);
        temp_db_.resize(kNumPoints);
        setInterceptsMouseClicks(false, false);
    }

    namespace {
        juce::ColourGradient makeResponseGradient(const SumPanel::GradientData& data,
                                                  const float alpha) {
            juce::ColourGradient gradient(data.colours.front().withMultipliedAlpha(alpha),
                                           data.xs.front(), 0.f,
                                           data.colours.back().withMultipliedAlpha(alpha),
                                           data.xs.back(), 0.f, false);
            const auto width = data.xs.back() - data.xs.front();
            for (size_t i = 1; i + 1 < data.xs.size(); ++i) {
                const auto p = juce::jlimit(0.0, 1.0,
                    static_cast<double>((data.xs[i] - data.xs.front()) / width));
                gradient.addColour(p, data.colours[i].withMultipliedAlpha(alpha));
            }
            return gradient;
        }

        void strokeBlendedResponse(juce::Graphics& g, const juce::Path& path,
                                   const SumPanel::GradientData& data,
                                   const float thickness, const float alpha) {
            if (path.isEmpty()) return;

            if (!data.valid || data.xs.back() <= data.xs.front() + 1.f) {
                g.setColour(zlgui::glass::neutralResponse().withAlpha(.10f * alpha));
                g.strokePath(path, juce::PathStrokeType(thickness * 2.15f,
                                                        juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
                g.setColour(zlgui::glass::neutralResponse().withAlpha(.86f * alpha));
                g.strokePath(path, juce::PathStrokeType(thickness,
                                                        juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
                return;
            }

            // Both the glow and the crisp response use the band's blended gradient. This keeps
            // the hue readable at a glance instead of surrounding it with a pale cyan halo.
            g.setGradientFill(makeResponseGradient(data, .13f * alpha));
            g.strokePath(path, juce::PathStrokeType(thickness * 2.15f,
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

            g.setGradientFill(makeResponseGradient(data, .98f * alpha));
            g.strokePath(path, juce::PathStrokeType(thickness,
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        }
    }

    void SumPanel::paintSameStereo(juce::Graphics& g) {
        for (size_t lr = 0; lr < 5; ++lr) {
            const auto i = static_cast<size_t>(4) - lr;
            paths_[i].pull();
            gradients_[i].pull();
            const auto& path{paths_[i].getReader()};
            if (!path.isEmpty() && is_same_stereo_[i]) {
                strokeBlendedResponse(g, path, gradients_[i].getReader(), curve_thickness_, 1.f);
            }
        }
    }

    void SumPanel::paintDifferentStereo(juce::Graphics& g) {
        for (size_t lr = 0; lr < 5; ++lr) {
            const auto i = static_cast<size_t>(4) - lr;
            paths_[i].pull();
            gradients_[i].pull();
            const auto& path{paths_[i].getReader()};
            if (!path.isEmpty() && !is_same_stereo_[i]) {
                strokeBlendedResponse(g, path, gradients_[i].getReader(), curve_thickness_,
                                      kDiffStereoAlphaMultiplier);
            }
        }
    }

    void SumPanel::resized() {
        lookAndFeelChanged();
    }

    void SumPanel::run(const size_t lr, const bool to_update, const bool is_not_off,
                       const std::span<size_t> on_indices,
                       const std::span<float> xs, float k, float b,
                       std::array<zldsp::vector::aligned_vector<float>, zlp::kBandNum>& dynamic_mags) {
        if (!to_update) {
            return;
        }

        auto& path{paths_[lr].getWriter()};
        path.clear();

        if (on_indices.empty()) {
            if (is_not_off) {
                path.startNewSubPath(xs[0], b);
                path.lineTo(xs.back(), b);
            }
            auto& gradient = gradients_[lr].getWriter();
            gradient.valid = false;
            gradients_[lr].publish();
            paths_[lr].publish();
            return;
        }

        {
            namespace hn = hwy::HWY_NAMESPACE;
            static constexpr hn::ScalableTag<float> d;
            static constexpr size_t lanes = hn::MaxLanes(d);

            const auto vk = hn::Set(d, k);
            const auto vb = hn::Set(d, b);
            size_t i = 0;
            for (; i + lanes <= temp_db_.size(); i += lanes) {
                auto v_sum = hn::Zero(d);
                for (const size_t on_index : on_indices) {
                    const float* mag_ptr = dynamic_mags[on_index].data();
                    v_sum = hn::Add(v_sum, hn::Load(d, mag_ptr + i));
                }
                hn::Store(hn::MulAdd(vk, v_sum, vb), d, temp_db_.data() + i);
            }
            for (; i < temp_db_.size(); ++i) {
                float sum = 0.0f;
                for (const size_t on_index : on_indices) {
                    sum += dynamic_mags[on_index][i];
                }
                temp_db_[i] = std::fma(k, sum, b);
            }
        }

        // Build a horizontal colour field from the actual influence of every active band.
        // This is what gives the final response the mockup's blue→teal→amber→violet blending
        // instead of a single cyan sum line.
        auto& gradient = gradients_[lr].getWriter();
        gradient.valid = !on_indices.empty() && xs.size() >= kGradientStops;
        if (gradient.valid) {
            const auto neutral = zlgui::glass::neutralResponse();
            for (size_t stop = 0; stop < kGradientStops; ++stop) {
                const auto idx = juce::jmin(xs.size() - 1,
                    (stop * (xs.size() - 1)) / (kGradientStops - 1));
                gradient.xs[stop] = xs[idx];

                float total = 0.f, rr = 0.f, gg = 0.f, bb = 0.f;
                for (const auto band : on_indices) {
                    const auto influence = std::pow(std::abs(dynamic_mags[band][idx]), .72f);
                    if (influence <= 1.0e-4f) continue;
                    const auto c = base_.getColourMap1(band);
                    rr += c.getFloatRed() * influence;
                    gg += c.getFloatGreen() * influence;
                    bb += c.getFloatBlue() * influence;
                    total += influence;
                }

                juce::Colour mixed = neutral;
                if (total > 1.0e-4f) {
                    mixed = juce::Colour::fromFloatRGBA(rr / total, gg / total, bb / total, 1.f)
                                .interpolatedWith(juce::Colours::white, .08f);
                    // v1.0 leaned too far toward the neutral response colour. Keep the line
                    // pastel, but allow the active bands to visibly own the hue.
                    const auto tint = juce::jlimit(.68f, .96f, .72f + total * .060f);
                    mixed = neutral.interpolatedWith(mixed, tint);
                }
                gradient.colours[stop] = mixed.withAlpha(.98f);
            }
        }
        gradients_[lr].publish();

        PathMinimizer<1> minimizer(path);
        minimizer.drawPath<true, false>(xs, std::span(temp_db_));
        paths_[lr].publish();
    }

    void SumPanel::updateDrawingParas(const int lr, const bool is_same_stereo) {
        is_same_stereo_[static_cast<size_t>(lr)] = is_same_stereo;
    }

    void SumPanel::lookAndFeelChanged() {
        curve_thickness_ = base_.getFontSize() * .128f * base_.getSumEQCurveThickness();
    }
}
