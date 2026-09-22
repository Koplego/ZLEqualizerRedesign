// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.

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
                if (band != selected_band && is_same_stereo_[band]) drawBand<false>(g, band);
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
                if (band != selected_band && !is_same_stereo_[band]) drawBand<false>(g, band);
            }
        } else {
            for (const auto& band : not_off_indices_) drawBand<false>(g, band);
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
        if (filter_status == zlp::FilterStatus::kBypass) multiplier *= kBypassAlphaMultiplier;
        if (!is_same_stereo) {
            multiplier *= kDiffStereoAlphaMultiplier;
        } else if (selected_band == zlp::kBandNum) {
            multiplier *= kNoBandSelectedAlphaMultiplier;
        }

        if (is_selected) {
            base_fill_alpha_[band] = (is_dynamic_on ? kFillingAlpha * .38f : kFillingAlpha * 1.08f) * multiplier;
            target_fill_alpha_[band] = (is_dynamic_on ? kDynamicFillingAlpha : 0.f) * multiplier;
        } else {
            // Non-selected bands remain visibly luminous in the reference; selection changes
            // emphasis, not whether the colour field exists at all.
            base_fill_alpha_[band] = kFillingAlpha * .78f * multiplier;
            target_fill_alpha_[band] = (is_dynamic_on ? kDynamicFillingAlpha * .46f : 0.f) * multiplier;
            multiplier *= juce::jmax(.78f, kNotSelectedAlphaMultiplier);
        }
        base_stroke_alpha_[band] = multiplier;

        const auto band_colour = base_.getColourMap1(band);
        base_stroke_colour_[band] = band_colour.interpolatedWith(juce::Colours::white, is_selected ? .20f : .11f)
            .withAlpha(std::clamp(multiplier * (is_selected ? 1.f : .90f), .12f, 1.f));
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
        if (to_update_target) target_fills_[band].publish();
        if (to_update_side) side_lines_[band].publish();
    }

    template <bool thick>
    void SinglePanel::drawBand(juce::Graphics& g, const size_t band) {
        const auto colour = base_.getColourMap1(band);
        const auto node_x = node_x_[band].load(std::memory_order_relaxed);
        const auto node_y = node_y_[band].load(std::memory_order_relaxed);
        const auto glow_radius = juce::jmax(base_.getFontSize() * (thick ? 6.35f : 5.85f), thick ? 72.f : 64.f);

        if (base_fill_alpha_[band] > 0.01f) {
            base_fills_[band].pull();
            const auto& fill = base_fills_[band].getReader();

            // Crucial difference from the regressed versions: the node light is painted by
            // the full graph component, not by the small button, so the halo cannot be clipped.
            juce::ColourGradient node_pool(
                colour.interpolatedWith(juce::Colours::white, thick ? .16f : .09f)
                    .withAlpha(thick ? .50f : .34f),
                node_x, node_y,
                colour.withAlpha(0.f), node_x + glow_radius, node_y, true);
            node_pool.addColour(.18, colour.withAlpha(thick ? .40f : .28f));
            node_pool.addColour(.42, colour.withAlpha(thick ? .20f : .135f));
            node_pool.addColour(.68, colour.withAlpha(thick ? .072f : .047f));
            node_pool.addColour(.88, colour.withAlpha(.010f));
            g.setGradientFill(node_pool);
            g.fillEllipse(node_x - glow_radius, node_y - glow_radius,
                          glow_radius * 2.f, glow_radius * 2.f);

            // Broad stained-glass fill inside the actual response shape.
            g.setColour(colour.withAlpha(juce::jlimit(.035f, .18f, base_fill_alpha_[band] * .88f)));
            g.fillPath(fill);

            juce::ColourGradient shape_light(
                colour.interpolatedWith(juce::Colours::white, thick ? .13f : .07f)
                    .withAlpha(thick ? .27f : .15f),
                node_x, node_y,
                colour.withAlpha(0.f), node_x + glow_radius * .95f, node_y, true);
            shape_light.addColour(.34, colour.withAlpha(thick ? .16f : .09f));
            shape_light.addColour(.67, colour.withAlpha(thick ? .045f : .028f));
            g.setGradientFill(shape_light);
            g.fillPath(fill);
        }

        if (target_fill_alpha_[band] > 0.01f) {
            target_fills_[band].pull();
            const auto& fill = target_fills_[band].getReader();
            g.setColour(colour.withAlpha(target_fill_alpha_[band] * .38f));
            g.fillPath(fill);
        }

        const auto curve_thickness = thick ? curve_thickness_ * kThickMultiplier : curve_thickness_;
        if (base_stroke_alpha_[band] > 0.01f) {
            base_paths_[band].pull();
            const auto& path = base_paths_[band].getReader();

            // Every band gets a soft optical halo. The reference does not collapse
            // non-selected curves into dim hairlines.
            g.setColour(colour.withAlpha(thick ? .13f : .075f));
            g.strokePath(path, juce::PathStrokeType(curve_thickness * (thick ? 3.25f : 2.45f),
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
            g.setColour(colour.interpolatedWith(juce::Colours::white, .16f)
                        .withAlpha(thick ? .30f : .18f));
            g.strokePath(path, juce::PathStrokeType(curve_thickness * (thick ? 1.65f : 1.38f),
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

            if constexpr (thick) {
                juce::ColourGradient lit_stroke(
                    colour.interpolatedWith(juce::Colours::white, .36f).withAlpha(1.f),
                    node_x, node_y,
                    colour.interpolatedWith(juce::Colours::white, .06f).withAlpha(.72f),
                    node_x + glow_radius * 1.15f, node_y, true);
                lit_stroke.addColour(.35, colour.interpolatedWith(juce::Colours::white, .21f).withAlpha(.96f));
                lit_stroke.addColour(.72, colour.interpolatedWith(juce::Colours::white, .08f).withAlpha(.79f));
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
        curve_thickness_ = base_.getFontSize() * .082f * base_.getSingleEQCurveThickness();
    }
}
