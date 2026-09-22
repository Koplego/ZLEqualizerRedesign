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

        juce::ColourGradient makeNodeLineLight(const float centre_x,
                                               const float radius,
                                               const juce::Colour colour,
                                               const float alpha,
                                               const float white_mix) {
            const auto transparent = colour.withAlpha(0.f);
            juce::ColourGradient gradient(transparent, centre_x - radius, 0.f,
                                          transparent, centre_x + radius, 0.f, false);
            gradient.addColour(.08, colour.withAlpha(alpha * .008f));
            gradient.addColour(.22, colour.withAlpha(alpha * .045f));
            gradient.addColour(.36, colour.withAlpha(alpha * .18f));
            gradient.addColour(.46, colour.interpolatedWith(juce::Colours::white, white_mix * .40f)
                                           .withAlpha(alpha * .52f));
            gradient.addColour(.50, colour.interpolatedWith(juce::Colours::white, white_mix)
                                           .withAlpha(alpha));
            gradient.addColour(.54, colour.interpolatedWith(juce::Colours::white, white_mix * .40f)
                                           .withAlpha(alpha * .52f));
            gradient.addColour(.64, colour.withAlpha(alpha * .18f));
            gradient.addColour(.78, colour.withAlpha(alpha * .045f));
            gradient.addColour(.92, colour.withAlpha(alpha * .008f));
            return gradient;
        }

        void strokeBlendedResponse(juce::Graphics& g, const juce::Path& path,
                                   const SumPanel::GradientData& data,
                                   const float thickness, const float alpha,
                                   const float node_light_x = -1.f,
                                   const float node_light_radius = 0.f,
                                   const juce::Colour node_light_colour = juce::Colours::transparentBlack) {
            if (path.isEmpty()) return;

            if (!data.valid || data.xs.back() <= data.xs.front() + 1.f) {
                g.setColour(zlgui::glass::neutralResponse().withAlpha(.035f * alpha));
                g.strokePath(path, juce::PathStrokeType(thickness * 1.65f,
                                                        juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
                g.setColour(zlgui::glass::neutralResponse().withAlpha(.84f * alpha));
                g.strokePath(path, juce::PathStrokeType(thickness,
                                                        juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
            } else {
                g.setGradientFill(makeResponseGradient(data, .022f * alpha));
                g.strokePath(path, juce::PathStrokeType(thickness * 2.75f,
                                                        juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
                g.setGradientFill(makeResponseGradient(data, .070f * alpha));
                g.strokePath(path, juce::PathStrokeType(thickness * 1.45f,
                                                        juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
                g.setGradientFill(makeResponseGradient(data, .97f * alpha));
                g.strokePath(path, juce::PathStrokeType(thickness,
                                                        juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
            }

            // The selected node lights a long section of the blended response. The glow is
            // wider than before but slightly lower-energy, so it reads as reflection through
            // glass rather than a bright strip pasted onto the line.
            if (node_light_x >= 0.f && node_light_radius > 1.f && node_light_colour.getAlpha() > 0) {
                g.setGradientFill(makeNodeLineLight(node_light_x, node_light_radius,
                                                    node_light_colour, .18f * alpha, .18f));
                g.strokePath(path, juce::PathStrokeType(thickness * 2.55f,
                                                        juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));

                g.setGradientFill(makeNodeLineLight(node_light_x, node_light_radius * .58f,
                                                    node_light_colour, .62f * alpha, .42f));
                g.strokePath(path, juce::PathStrokeType(thickness * 1.06f,
                                                        juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
            }
        }
    }

    void SumPanel::paintSameStereo(juce::Graphics& g) {
        float node_light_x = -1.f;
        float node_light_radius = 0.f;
        auto node_light_colour = juce::Colours::transparentBlack;

        if (const auto selected_band = base_.getSelectedBand(); selected_band < zlp::kBandNum) {
            const auto* freq_ptr = p_ref_.parameters_.getRawParameterValue(
                zlp::PFreq::kID + std::to_string(selected_band));
            const auto sample_rate = p_ref_.getSampleRate();
            if (freq_ptr != nullptr && sample_rate > 1000.0 && getWidth() > 0) {
                const auto freq = juce::jmax(10.f, freq_ptr->load(std::memory_order_relaxed));
                const auto fft_max = static_cast<float>(freq_helper::getFFTMax(sample_rate));
                const auto denominator = std::log(fft_max * .1f);
                if (denominator > 1.0e-5f) {
                    const auto portion = static_cast<float>(kFFTSizeOverWidth) * std::log(freq * .1f) / denominator;
                    node_light_x = juce::jlimit(0.f, static_cast<float>(getWidth()),
                                               static_cast<float>(getWidth()) * portion);
                    node_light_radius = juce::jmax(base_.getFontSize() * 7.40f, 86.f);
                    node_light_colour = base_.getColourMap1(selected_band);
                }
            }
        }

        for (size_t lr = 0; lr < 5; ++lr) {
            const auto i = static_cast<size_t>(4) - lr;
            paths_[i].pull();
            gradients_[i].pull();
            const auto& path{paths_[i].getReader()};
            if (!path.isEmpty() && is_same_stereo_[i]) {
                strokeBlendedResponse(g, path, gradients_[i].getReader(), curve_thickness_, 1.f,
                                      node_light_x, node_light_radius, node_light_colour);
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
        if (!to_update) return;

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
                for (const size_t on_index : on_indices) sum += dynamic_mags[on_index][i];
                temp_db_[i] = std::fma(k, sum, b);
            }
        }

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
                                .interpolatedWith(juce::Colours::white, .055f);
                    const auto tint = juce::jlimit(.80f, .99f, .84f + total * .048f);
                    mixed = neutral.interpolatedWith(mixed, tint);
                }
                gradient.colours[stop] = mixed.withAlpha(.99f);
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
        curve_thickness_ = base_.getFontSize() * .092f * base_.getSumEQCurveThickness();
    }
}
