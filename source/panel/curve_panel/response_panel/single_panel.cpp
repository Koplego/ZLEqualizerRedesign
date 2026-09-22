// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#include "single_panel.hpp"

namespace zlpanel {
    SinglePanel::SinglePanel(PluginProcessor& p,
                             zlgui::UIBase& base,
                             std::vector<size_t>& not_off_indices) :
        p_ref_(p), base_(base),
        not_off_indices_(not_off_indices) {
        juce::ignoreUnused(p_ref_);
        temp_db_.resize(kNumPoints);
        setInterceptsMouseClicks(false, false);
    }

    void SinglePanel::paintSameStereo(juce::Graphics& g) {
        const auto selected_band = base_.getSelectedBand();
        if (selected_band < zlp::kBandNum) {
            for (const auto& band : not_off_indices_) {
                if (band != selected_band && is_same_stereo_[band]) {
                    drawBand<false>(g, band);
                }
            }
            drawBand<true>(g, selected_band);
            if (target_fill_alpha_[selected_band] > 0.01f) {
                g.setColour(base_stroke_colour_[selected_band]);
                side_lines_[selected_band].pull();
                const auto line = side_lines_[selected_band].getReader();
                g.fillRect(line.getStartX(), side_y_ - curve_thickness_ * .35f,
                           line.getEndX() - line.getStartX(), curve_thickness_ * .7f);
            }
        }
    }

    void SinglePanel::paintDifferentStereo(juce::Graphics& g) {
        const auto selected_band = base_.getSelectedBand();
        if (selected_band < zlp::kBandNum) {
            for (const auto& band : not_off_indices_) {
                if (band != selected_band && !is_same_stereo_[band]) {
                    drawBand<false>(g, band);
                }
            }
        } else {
            for (const auto& band : not_off_indices_) {
                drawBand<false>(g, band);
            }
        }
    }

    void SinglePanel::resized() {
        auto bound = getLocalBounds().toFloat();
        bound.removeFromBottom(static_cast<float>(getBottomAreaHeight(base_.getFontSize())));
        center_y_.store(bound.getHeight() * .5f, std::memory_order_relaxed);
        side_y_ = bound.getHeight() - base_.getFontSize() * kDraggerScale * .5f;
        lookAndFeelChanged();
    }

    void SinglePanel::updateDrawingParas(const size_t band,
                                         const zlp::FilterStatus filter_status,
                                         const bool is_dynamic_on, const bool is_same_stereo) {
        if (filter_status == zlp::FilterStatus::kOff) {
            base_fill_alpha_[band] = 0.f;
            target_fill_alpha_[band] = 0.f;
            base_stroke_alpha_[band] = 0.f;
            return;
        }
        float multiplier = 1.f;
        const auto selected_band = base_.getSelectedBand();
        const auto is_selected = (band == selected_band);
        if (filter_status == zlp::FilterStatus::kBypass) {
            multiplier *= kBypassAlphaMultiplier;
        }
        if (!is_same_stereo) {
            multiplier *= kDiffStereoAlphaMultiplier;
        } else if (selected_band == zlp::kBandNum) {
            multiplier *= kNoBandSelectedAlphaMultiplier;
        }
        if (is_selected) {
            base_fill_alpha_[band] = (is_dynamic_on ? kFillingAlpha * .30f : kFillingAlpha) * multiplier;
            target_fill_alpha_[band] = (is_dynamic_on ? kDynamicFillingAlpha : 0.f) * multiplier;
        } else {
            base_fill_alpha_[band] = kFillingAlpha * .48f * multiplier;
            target_fill_alpha_[band] = (is_dynamic_on ? kDynamicFillingAlpha * .44f : 0.f) * multiplier;
            multiplier *= kNotSelectedAlphaMultiplier;
        }
        base_stroke_alpha_[band] = multiplier;

        const auto band_colour = base_.getColourMap1(band);
        base_stroke_colour_[band] = band_colour.interpolatedWith(juce::Colours::white, is_selected ? .10f : .12f)
            .withAlpha(std::clamp(multiplier * (is_selected ? .90f : .58f), .06f, .92f));

        is_same_stereo_[band] = is_same_stereo;
    }

    void SinglePanel::run(const size_t band,
                          const zlp::FilterStatus filter_status,
                          const bool to_update_base, const bool to_update_target,
                          std::span<float> xs, const float k, const float b,
                          zldsp::vector::aligned_vector<float>& base_mag,
                          zldsp::vector::aligned_vector<float>& target_mag,
                          const float center_x, const float center_mag, const float button_mag,
                          const float base_left_x, const float base_right_x,
                          const bool to_update_side, const float left_x, const float right_x,
                          const bool is_all_pass, const bool is_first_order) {
        const auto center_y = center_y_.load(std::memory_order_relaxed);
        auto& next_base_path{base_paths_[band].getWriter()};
        auto& next_base_fill{base_fills_[band].getWriter()};
        auto& next_button_line{button_lines_[band].getWriter()};
        auto& next_target_fill{target_fills_[band].getWriter()};
        auto& next_side_line{side_lines_[band].getWriter()};
        auto& next_all_pass_line{all_pass_lines_[band].getWriter()};

        if (to_update_base) {
            node_x_[band].store(center_x, std::memory_order_relaxed);
            node_y_[band].store(button_mag, std::memory_order_relaxed);
            next_base_path.clear();
            next_base_fill.clear();
            next_button_line.setEnd(-100.f, -100.f);
            next_all_pass_line.setStart(-100.f, 0.f);
            next_all_pass_line.setEnd(-100.f, 0.f);
            if (filter_status != zlp::FilterStatus::kOff) {
                zldsp::vector::fma(temp_db_.data(), base_mag.data(), k, b, temp_db_.size());
                if (is_all_pass) {
                    next_base_path.startNewSubPath(center_x, 0.f);
                    next_base_path.lineTo(center_x, center_y * 2.f);
                    if (!is_first_order) {
                        next_all_pass_line.setStart(base_left_x, 0.f);
                        next_all_pass_line.setEnd(base_right_x, 0.f);
                    }
                } else {
                    PathMinimizer<1> minimizer{next_base_path};
                    minimizer.drawPath<true, false>(xs, std::span(temp_db_));
                    next_base_fill = next_base_path;
                    next_base_fill.lineTo(xs.back(), center_y);
                    next_base_fill.lineTo(xs[0], center_y);
                    next_base_fill.closeSubPath();
                }
                if (!is_all_pass && std::abs(center_mag - button_mag) > 1e-6f) {
                    next_button_line.setStart(center_x, center_mag);
                    next_button_line.setEnd(center_x, button_mag);
                }
            }
        }
        if (to_update_target) {
            next_target_fill.clear();
            if (filter_status != zlp::FilterStatus::kOff && !is_all_pass) {
                zldsp::vector::fma(temp_db_.data(), target_mag.data(), k, b, temp_db_.size());
                next_target_fill = next_base_path;
                PathMinimizer<1> minimizer{next_target_fill};
                minimizer.drawPath<false, true>(xs, std::span(temp_db_));
                next_target_fill.closeSubPath();
            }
        }
        if (to_update_side) {
            next_side_line.setStart(left_x, 0.f);
            next_side_line.setEnd(right_x, 0.f);
        }
        if (to_update_base) {
            base_paths_[band].publish();
            base_fills_[band].publish();
            button_lines_[band].publish();
            all_pass_lines_[band].publish();
        }
        if (to_update_target) {
            target_fills_[band].publish();
        }
        if (to_update_side) {
            side_lines_[band].publish();
        }
    }

    template <bool thick>
    void SinglePanel::drawBand(juce::Graphics& g, const size_t band) {
        const auto colour = base_.getColourMap1(band);
        const auto node_x = node_x_[band].load(std::memory_order_relaxed);
        const auto node_y = node_y_[band].load(std::memory_order_relaxed);

        // Match the concept: the fill receives a soft local wash, while the curve itself
        // becomes a little brighter near the node without turning into a thick neon tube.
        const auto glow_radius = juce::jmax(base_.getFontSize() * (thick ? 4.15f : 3.10f),
                                           thick ? 48.f : 36.f);
        const auto fill_light_alpha = thick ? .135f : .050f;
        const auto line_light_alpha = thick ? .58f : .24f;

        if (base_fill_alpha_[band] > 0.01f) {
            base_fills_[band].pull();
            const auto& fill = base_fills_[band].getReader();
            g.setColour(colour.withAlpha(base_fill_alpha_[band]));
            g.fillPath(fill);

            juce::ColourGradient illumination(
                colour.interpolatedWith(juce::Colours::white, thick ? .34f : .20f)
                      .withAlpha(fill_light_alpha),
                node_x, node_y,
                colour.withAlpha(0.f), node_x + glow_radius, node_y, true);
            illumination.addColour(.28, colour.interpolatedWith(juce::Colours::white, thick ? .17f : .10f)
                                               .withAlpha(fill_light_alpha * .55f));
            illumination.addColour(.62, colour.withAlpha(fill_light_alpha * .12f));
            g.setGradientFill(illumination);
            g.fillPath(fill);
        }

        if (target_fill_alpha_[band] > 0.01f) {
            target_fills_[band].pull();
            const auto& fill = target_fills_[band].getReader();
            g.setColour(colour.withAlpha(target_fill_alpha_[band]));
            g.fillPath(fill);

            juce::ColourGradient target_light(
                colour.interpolatedWith(juce::Colours::white, thick ? .28f : .17f)
                      .withAlpha(fill_light_alpha * .72f),
                node_x, node_y,
                colour.withAlpha(0.f), node_x + glow_radius * .82f, node_y, true);
            target_light.addColour(.48, colour.withAlpha(fill_light_alpha * .12f));
            g.setGradientFill(target_light);
            g.fillPath(fill);
        }

        const auto curve_thickness = thick ? curve_thickness_ * kThickMultiplier : curve_thickness_;
        if (base_stroke_alpha_[band] > 0.01f) {
            base_paths_[band].pull();
            const auto& path = base_paths_[band].getReader();

            g.setColour(base_stroke_colour_[band]);
            g.strokePath(path, juce::PathStrokeType(curve_thickness,
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

            // Feathered colour immediately around the source, just enough to make the
            // node feel physically attached to the response line.
            juce::ColourGradient bloom(
                colour.interpolatedWith(juce::Colours::white, thick ? .38f : .24f)
                      .withAlpha(line_light_alpha * .18f),
                node_x, node_y,
                colour.withAlpha(0.f), node_x + glow_radius * .72f, node_y, true);
            bloom.addColour(.42, colour.withAlpha(line_light_alpha * .055f));
            g.setGradientFill(bloom);
            g.strokePath(path, juce::PathStrokeType(curve_thickness * (thick ? 1.70f : 1.38f),
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

            // The visible line remains thin; locally it simply lifts toward the pale rim
            // colour shown in the mockup.
            juce::ColourGradient lit_line(
                colour.interpolatedWith(juce::Colours::white, thick ? .58f : .42f)
                      .withAlpha(line_light_alpha),
                node_x, node_y,
                colour.withAlpha(0.f), node_x + glow_radius * .46f, node_y, true);
            lit_line.addColour(.38, colour.interpolatedWith(juce::Colours::white, thick ? .30f : .20f)
                                           .withAlpha(line_light_alpha * .44f));
            g.setGradientFill(lit_line);
            g.strokePath(path, juce::PathStrokeType(curve_thickness * (thick ? 1.02f : 1.0f),
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

            g.setColour(base_stroke_colour_[band]);
            button_lines_[band].pull();
            if (const auto line = button_lines_[band].getReader(); line.getEndX() > 0.f) {
                if (line.getEndY() > line.getStartY()) {
                    g.fillRect(line.getStartX() - curve_thickness * .30f, line.getStartY(),
                               curve_thickness * .60f, line.getEndY() - line.getStartY());
                } else {
                    g.fillRect(line.getStartX() - curve_thickness * .30f, line.getEndY(),
                               curve_thickness * .60f, line.getStartY() - line.getEndY());
                }
            }
            if (thick) {
                all_pass_lines_[band].pull();
                const auto ap_line = all_pass_lines_[band].getReader();
                if (ap_line.getEndX() > 0.f || ap_line.getEndY() > 0.f) {
                    const auto center_y = center_y_.load(std::memory_order_relaxed);
                    g.fillRect(ap_line.getStartX() - curve_thickness * .30f, center_y * .5f,
                               curve_thickness * .60f, center_y);
                    g.fillRect(ap_line.getEndX() - curve_thickness * .30f, center_y * .5f,
                               curve_thickness * .60f, center_y);
                }
            }
        }
    }

    void SinglePanel::lookAndFeelChanged() {
        curve_thickness_ = base_.getFontSize() * .068f * base_.getSingleEQCurveThickness();
    }
}
