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

            const auto innerBounds = inner_path_.getBounds();
            // The node is a coloured emitter inside clear glass, not a solid coloured ball.
            // Keep the core translucent so the graph and the node's own light pass through it.
            juce::ColourGradient lens(
                juce::Colours::white.withAlpha((active ? .42f : .34f) * visibility),
                innerBounds.getX() + innerBounds.getWidth() * .24f,
                innerBounds.getY() + innerBounds.getHeight() * .16f,
                colour_.withAlpha((active ? .31f : .27f) * visibility),
                innerBounds.getRight(), innerBounds.getBottom(), true);
            lens.addColour(.52, colour_.interpolatedWith(juce::Colours::white, .35f)
                                      .withAlpha(.18f * visibility));
            g.setGradientFill(lens);
            g.fillPath(inner_path_);

            if (dragger_shape_ == kRound) {
                juce::Path crown;
                const auto cx = innerBounds.getCentreX();
                const auto cy = innerBounds.getCentreY();
                const auto rx = innerBounds.getWidth() * .5f;
                const auto ry = innerBounds.getHeight() * .5f;
                crown.startNewSubPath(cx - rx * .72f, cy - ry * .48f);
                crown.quadraticTo(cx - rx * .08f, cy - ry * 1.04f,
                                   cx + rx * .58f, cy - ry * .68f);
                juce::ColourGradient crownLight(juce::Colours::transparentWhite,
                    cx - rx * .8f, cy - ry,
                    juce::Colours::transparentWhite, cx + rx * .7f, cy - ry, false);
                crownLight.addColour(.36f, juce::Colours::white.withAlpha(.49f * visibility));
                crownLight.addColour(.68f, juce::Colours::white.withAlpha(.27f * visibility));
                g.setGradientFill(crownLight);
                g.strokePath(crown, juce::PathStrokeType(juce::jmax(.8f, base_.getFontSize() * .09f),
                    juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }

            const auto outerBounds = outline_path_.getBounds();
            juce::ColourGradient outerRim(
                juce::Colours::white.withAlpha((active ? .95f : hover ? .86f : .78f) * visibility),
                outerBounds.getX(), outerBounds.getY(),
                colour_.interpolatedWith(juce::Colours::white, .48f)
                    .withAlpha(.36f * visibility),
                outerBounds.getRight(), outerBounds.getBottom(), false);
            outerRim.addColour(.49, juce::Colours::white.withAlpha(.74f * visibility));
            g.setGradientFill(outerRim);
            g.strokePath(outline_path_, juce::PathStrokeType(juce::jmax(1.25f, base_.getFontSize() * .12f)));

            juce::ColourGradient innerRim(
                juce::Colours::white.withAlpha(.47f * visibility),
                innerBounds.getX(), innerBounds.getY(),
                juce::Colours::black.withAlpha(.16f * visibility),
                innerBounds.getRight(), innerBounds.getBottom(), false);
            innerRim.addColour(.42, juce::Colours::transparentWhite);
            g.setGradientFill(innerRim);
            g.strokePath(inner_path_, juce::PathStrokeType(juce::jmax(.8f, base_.getFontSize() * .07f)));

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
            // The optical lens sits inside the button's larger interactive hit area.
            bound = bound.withSizeKeepingCentre(radius * .85f, radius * .85f);
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
