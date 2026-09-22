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
        center_y_.store(bound.getHeight() * .5f, std::memory_order::relaxed);
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
            base_fill_alpha_[band] = (is_dynamic_on ? kFillingAlpha * .24f : kFillingAlpha) * multiplier;
            target_fill_alpha_[band] = (is_dynamic_on ? kDynamicFillingAlpha : 0.f) * multiplier;
        } else {
            base_fill_alpha_[band] = kFillingAlpha * .30f * multiplier;
            target_fill_alpha_[band] = (is_dynamic_on ? kDynamicFillingAlpha * .28f : 0.f) * multiplier;
            multiplier *= kNotSelectedAlphaMultiplier;
        }
        base_stroke_alpha_[band] = multiplier;

        const auto band_colour = base_.getColourMap1(band);
        base_stroke_colour_[band] = band_colour.interpolatedWith(juce::Colours::white, is_selected ? .10f : .06f)
            .withAlpha(std::clamp(multiplier * (is_selected ? .93f : .56f), .06f, .93f));

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
        // Keep illumination local to the band control.  The reference is dark blue glass
        // with coloured light sources, not a graph-wide colour field.
        const auto glow_radius = juce::jmax(base_.getFontSize() * 4.15f, 44.f);

        if (base_fill_alpha_[band] > 0.01f) {
            base_fills_[band].pull();
            const auto& fill = base_fills_[band].getReader();

            // A faint structural fill keeps the complete filter shape legible.
            g.setColour(colour.withAlpha(base_fill_alpha_[band] * .42f));
            g.fillPath(fill);

            if constexpr (thick) {
                // The selected band gets a compact radial transmission bloom centred on the
                // control point.  It fades quickly so neighbouring controls remain neutral.
                juce::ColourGradient illumination(
                    colour.interpolatedWith(juce::Colours::white, .10f).withAlpha(.145f),
                    node_x, node_y,
                    colour.withAlpha(0.f), node_x + glow_radius, node_y, true);
                illumination.addColour(.26, colour.withAlpha(.085f));
                illumination.addColour(.58, colour.withAlpha(.024f));
                illumination.addColour(.82, colour.withAlpha(.004f));
                g.setGradientFill(illumination);
                g.fillPath(fill);
            }
        }

        if (target_fill_alpha_[band] > 0.01f) {
            target_fills_[band].pull();
            const auto& fill = target_fills_[band].getReader();
            g.setColour(colour.withAlpha(target_fill_alpha_[band] * .36f));
            g.fillPath(fill);
            if constexpr (thick) {
                juce::ColourGradient target_light(
                    colour.interpolatedWith(juce::Colours::white, .08f).withAlpha(.105f),
                    node_x, node_y,
                    colour.withAlpha(0.f), node_x + glow_radius * .70f, node_y, true);
                target_light.addColour(.46, colour.withAlpha(.030f));
                g.setGradientFill(target_light);
                g.fillPath(fill);
            }
        }

        const auto curve_thickness = thick ? curve_thickness_ * kThickMultiplier : curve_thickness_;
        if (base_stroke_alpha_[band] > 0.01f) {
            base_paths_[band].pull();
            const auto& path = base_paths_[band].getReader();

            if constexpr (thick) {
                juce::ColourGradient outer_light(
                    colour.interpolatedWith(juce::Colours::white, .10f).withAlpha(.042f),
                    node_x, node_y,
                    colour.withAlpha(0.f), node_x + glow_radius * .88f, node_y, true);
                g.setGradientFill(outer_light);
                g.strokePath(path, juce::PathStrokeType(curve_thickness * 2.05f,
                                                        juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));

                juce::ColourGradient core_light(
                    colour.interpolatedWith(juce::Colours::white, .18f).withAlpha(.095f),
                    node_x, node_y,
                    colour.withAlpha(0.f), node_x + glow_radius * .42f, node_y, true);
                g.setGradientFill(core_light);
                g.strokePath(path, juce::PathStrokeType(curve_thickness * 1.18f,
                                                        juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
            }

            if constexpr (thick) {
                juce::ColourGradient lit_stroke(
                    colour.interpolatedWith(juce::Colours::white, .26f).withAlpha(.98f),
                    node_x, node_y,
                    colour.interpolatedWith(juce::Colours::white, .025f).withAlpha(.40f),
                    node_x + glow_radius * 1.02f, node_y, true);
                lit_stroke.addColour(.36, colour.interpolatedWith(juce::Colours::white, .12f).withAlpha(.82f));
                lit_stroke.addColour(.72, colour.interpolatedWith(juce::Colours::white, .04f).withAlpha(.54f));
                g.setGradientFill(lit_stroke);
            } else {
                g.setColour(base_stroke_colour_[band]);
            }

            g.strokePath(path, juce::PathStrokeType(curve_thickness,
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

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
