// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../interface_definitions.hpp"
#include "../glass_tokens.hpp"

namespace zlgui::dragger {
    class DraggerLookAndFeel final : public juce::LookAndFeel_V4 {
    public:
        enum DraggerShape { kRound, kRectangle, kUpDownArrow, kRightArrow, kLeftArrow };

        explicit DraggerLookAndFeel(UIBase& base) : base_(base) {}

        void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                              bool should_draw_button_as_highlighted,
                              bool should_draw_button_as_down) override {
            const auto active = should_draw_button_as_down || button.getToggleState() || is_selected_;
            const auto hover = should_draw_button_as_highlighted && !active;
            const auto visibility = juce::jlimit(0.f, 1.f, alpha_);

            // The large halo is now painted by SinglePanel where it cannot be clipped by the
            // button bounds. Keep only a small hot bloom here around the actual glass lens.
            if (dragger_shape_ == kRound) {
                const juce::DropShadow hot_halo{
                    colour_.interpolatedWith(juce::Colours::white, .18f)
                        .withAlpha((active ? .28f : hover ? .19f : .12f) * visibility),
                    juce::jmax(3, juce::roundToInt(base_.getFontSize() * .55f)), {0, 0}};
                hot_halo.drawForPath(g, outline_path_);
            }

            g.setColour(colour_.interpolatedWith(juce::Colours::white, .08f)
                        .withAlpha((active ? .42f : hover ? .34f : .28f) * visibility));
            g.fillPath(outline_path_);

            // Strong milky outer rim from the reference.
            g.setColour(juce::Colours::white.withAlpha((active ? .98f : hover ? .88f : .80f) * visibility));
            g.strokePath(outline_path_, juce::PathStrokeType(juce::jmax(1.15f, base_.getFontSize() * .115f)));

            const auto innerBounds = inner_path_.getBounds();
            juce::ColourGradient lens(
                colour_.interpolatedWith(juce::Colours::white, active ? .48f : .43f)
                    .withAlpha((active ? .99f : .95f) * visibility),
                innerBounds.getX() + innerBounds.getWidth() * .24f,
                innerBounds.getY() + innerBounds.getHeight() * .13f,
                colour_.interpolatedWith(juce::Colours::black, .02f)
                    .withAlpha((active ? .98f : .94f) * visibility),
                innerBounds.getRight(), innerBounds.getBottom(), true);
            lens.addColour(.46, colour_.interpolatedWith(juce::Colours::white, .14f)
                                      .withAlpha((active ? .99f : .96f) * visibility));
            g.setGradientFill(lens);
            g.fillPath(inner_path_);

            g.setColour(juce::Colours::white.withAlpha((active ? .34f : hover ? .30f : .26f) * visibility));
            g.strokePath(inner_path_, juce::PathStrokeType(juce::jmax(.95f, base_.getFontSize() * .074f)));

            if (dragger_shape_ == kRound) {
                auto glint = innerBounds.withSizeKeepingCentre(innerBounds.getWidth() * .40f,
                                                               innerBounds.getHeight() * .18f);
                glint.translate(-innerBounds.getWidth() * .13f, -innerBounds.getHeight() * .22f);
                g.setColour(juce::Colours::white.withAlpha((active ? .42f : .27f) * visibility));
                g.fillEllipse(glint);
            }

            if (label_.length() > 0) {
                g.setColour(base_.getTextColour().withAlpha(.88f * visibility));
                g.setFont(base_.getFontSize() * label_scale_);
                auto bound = button.getLocalBounds().toFloat();
                const auto radius = std::min(bound.getHeight(), bound.getWidth());
                bound = bound.withSizeKeepingCentre(radius, radius);
                g.drawText(label_, bound, juce::Justification::centred);
            }
        }

        inline void setColour(const juce::Colour c) {
            colour_ = c;
            filling_colour_ = base_.getColourBlendedWithBackground(c, alpha_);
        }
        void setIsSelected(const bool f) { is_selected_ = f; }
        [[nodiscard]] bool getIsSelected() const { return is_selected_; }
        void setDraggerShape(const DraggerShape s) { dragger_shape_ = s; }

        void updatePaths(const juce::Rectangle<float>& bound) {
            outline_path_.clear();
            inner_path_.clear();
            const auto padding = padding_scale_ * base_.getFontSize();
            auto reduced_bound = bound.reduced(padding * .5f);
            switch (dragger_shape_) {
            case kRound: updateRoundPaths(reduced_bound); break;
            case kRectangle: updateRectanglePaths(reduced_bound); break;
            case kUpDownArrow: updateUpDownArrowPaths(reduced_bound); break;
            case kRightArrow: updateRightArrowPaths(reduced_bound); break;
            case kLeftArrow: updateLeftArrowPaths(reduced_bound); break;
            }
        }

        void updateRoundPaths(juce::Rectangle<float>& bound) {
            const auto radius = bound.getWidth();
            // At the same editor width the target's visible nodes are about 15-20% larger
            // than the regressed build. Keep the hit target unchanged and let the lens occupy it.
            bound = bound.withSizeKeepingCentre(radius * .995f, radius * .995f);
            outline_path_.addEllipse(bound);
            bound = bound.withSizeKeepingCentre(radius * .93f, radius * .93f);
            inner_path_.addEllipse(bound);
        }

        void updateRectanglePaths(juce::Rectangle<float>& bound) {
            const auto radius = bound.getWidth() * 0.75f;
            bound = bound.withSizeKeepingCentre(radius, radius);
            outline_path_.addRectangle(bound);
            bound = bound.withSizeKeepingCentre(radius * .7f, radius * .7f);
            inner_path_.addRectangle(bound);
        }

        void updateUpDownArrowPaths(juce::Rectangle<float>& bound) {
            const auto updateOnePath = [](juce::Path& path, const juce::Rectangle<float>& temp) {
                path.startNewSubPath(temp.getCentreX(), temp.getY());
                path.lineTo(temp.getCentreX() + temp.getWidth() * .33f, temp.getCentreY());
                path.lineTo(temp.getCentreX(), temp.getBottom());
                path.lineTo(temp.getCentreX() - temp.getWidth() * .33f, temp.getCentreY());
                path.closeSubPath();
            };
            updateOnePath(outline_path_, bound);
            bound = bound.withSizeKeepingCentre(bound.getWidth() * .625f, bound.getHeight() * .625f);
            updateOnePath(inner_path_, bound);
        }

        void updateRightArrowPaths(juce::Rectangle<float>& bound) {
            const auto updateOnePath = [](juce::Path& path, const juce::Rectangle<float> temp) {
                const auto center = temp.getCentre();
                path.startNewSubPath(center.getX() + temp.getWidth() * .5f, center.getY());
                path.lineTo(center.getX(), center.getY() + temp.getHeight() * std::sqrt(3.f) * .25f);
                path.lineTo(center.getX(), center.getY() - temp.getHeight() * std::sqrt(3.f) * .25f);
                path.closeSubPath();
            };
            updateOnePath(outline_path_, bound);
            bound = bound.withSizeKeepingCentre(bound.getWidth() * .75f, bound.getHeight() * .75f);
            updateOnePath(inner_path_, bound);
        }

        void updateLeftArrowPaths(juce::Rectangle<float>& bound) {
            const auto updateOnePath = [](juce::Path& path, const juce::Rectangle<float> temp) {
                const auto center = temp.getCentre();
                path.startNewSubPath(center.getX() - temp.getWidth() * .5f, center.getY());
                path.lineTo(center.getX(), center.getY() + temp.getHeight() * std::sqrt(3.f) * .25f);
                path.lineTo(center.getX(), center.getY() - temp.getHeight() * std::sqrt(3.f) * .25f);
                path.closeSubPath();
            };
            updateOnePath(outline_path_, bound);
            bound = bound.withSizeKeepingCentre(bound.getWidth() * .75f, bound.getHeight() * .75f);
            updateOnePath(inner_path_, bound);
        }

        void setLabel(const juce::String& l) { label_ = l; }
        void setLabelScale(const float x) { label_scale_ = x; }
        void setPaddingScale(const float x) { padding_scale_ = x; }
        void setAlpha(const float a) {
            alpha_ = a;
            filling_colour_ = base_.getColourBlendedWithBackground(colour_, alpha_);
        }

    private:
        juce::Colour colour_, filling_colour_;
        juce::Path outline_path_, inner_path_;
        bool is_selected_{false};
        DraggerShape dragger_shape_{DraggerShape::kRound};
        juce::String label_;
        float label_scale_ = 1.f;
        float padding_scale_ = 0.1f;
        float alpha_ = 1.f;
        UIBase& base_;
    };
}
