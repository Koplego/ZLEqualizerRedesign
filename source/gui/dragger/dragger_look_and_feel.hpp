// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../interface_definitions.hpp"
#include "../glass_tokens.hpp"

namespace zlgui::dragger {
    class DraggerLookAndFeel final : public juce::LookAndFeel_V4 {
    public:
        enum DraggerShape {
            kRound,
            kRectangle,
            kUpDownArrow,
            kRightArrow,
            kLeftArrow
        };

        explicit DraggerLookAndFeel(UIBase& base) : base_(base) {}

        void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                              bool should_draw_button_as_highlighted,
                              bool should_draw_button_as_down) override {
            const auto active = should_draw_button_as_down || button.getToggleState() || is_selected_;
            const auto hover = should_draw_button_as_highlighted && !active;
            const auto visibility = juce::jlimit(0.f, 1.f, alpha_);

            if (dragger_shape_ == kRound) {
                // The node is the lamp. The reference has a thick luminous perimeter and a
                // soft corona that is clearly visible beyond the disc, while the centre stays
                // saturated and comparatively calm.
                const juce::DropShadow edgeBloom{
                    colour_.interpolatedWith(juce::Colours::white, .12f)
                           .withAlpha((active ? .34f : hover ? .27f : .20f) * visibility),
                    juce::jmax(3, juce::roundToInt(base_.getFontSize() * (active ? .72f : .58f))),
                    {0, 0}};
                edgeBloom.drawForPath(g, outline_path_);

                const auto outerBounds = outline_path_.getBounds();
                const auto centre = outerBounds.getCentre();
                const auto radius = juce::jmax(1.f, outerBounds.getWidth() * .50f);

                juce::ColourGradient body(
                    colour_.interpolatedWith(juce::Colours::black, active ? .035f : .060f)
                           .withAlpha((active ? .98f : .93f) * visibility),
                    centre.x, centre.y,
                    colour_.interpolatedWith(juce::Colours::white, active ? .09f : .055f)
                           .withAlpha((active ? .98f : .92f) * visibility),
                    centre.x + radius, centre.y, true);
                body.addColour(.72, colour_.withAlpha((active ? .99f : .94f) * visibility));
                body.addColour(.92, colour_.interpolatedWith(juce::Colours::white, active ? .10f : .06f)
                                           .withAlpha((active ? .99f : .93f) * visibility));
                g.setGradientFill(body);
                g.fillPath(outline_path_);

                // Soft colour transition beneath the luminous edge.
                g.setColour(colour_.withAlpha((active ? .18f : hover ? .14f : .095f) * visibility));
                g.strokePath(outline_path_, juce::PathStrokeType(
                    juce::jmax(3.6f, base_.getFontSize() * .31f),
                    juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

                // The edge is the emitting surface: broad, pale and brighter than the centre.
                const auto rimThickness = juce::jmax(active ? 3.35f : 2.80f,
                    base_.getFontSize() * (active ? .245f : hover ? .225f : .205f));
                g.setColour(colour_.interpolatedWith(juce::Colours::white,
                                                     active ? .80f : hover ? .75f : .69f)
                            .withAlpha((active ? .995f : hover ? .94f : .88f) * visibility));
                g.strokePath(outline_path_, juce::PathStrokeType(
                    rimThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            } else {
                g.setColour(colour_.withAlpha((active ? .88f : .68f) * visibility));
                g.fillPath(outline_path_);
                g.setColour(glass::textPrimary().withAlpha((active ? .82f : .58f) * visibility));
                g.strokePath(outline_path_, juce::PathStrokeType(juce::jmax(.75f, base_.getFontSize() * .080f)));
            }

            if (label_.length() > 0) {
                g.setColour(base_.getTextColour().withAlpha(.90f * visibility));
                g.setFont(base_.getFontSize() * label_scale_);
                auto bound = button.getLocalBounds().toFloat();
                const auto d = std::min(bound.getHeight(), bound.getWidth());
                bound = bound.withSizeKeepingCentre(d, d);
                g.drawText(label_, bound, juce::Justification::centred);
            }
        }

        inline void setColour(const juce::Colour c) {
            colour_ = c;
            filling_colour_ = base_.getColourBlendedWithBackground(c, alpha_);
        }

        [[nodiscard]] juce::Colour getColour() const { return colour_; }
        [[nodiscard]] float getAlpha() const { return alpha_; }
        [[nodiscard]] DraggerShape getDraggerShape() const { return dragger_shape_; }

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
            outline_path_.addEllipse(bound);
            bound = bound.withSizeKeepingCentre(radius * .75f, radius * .75f);
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
            auto updateOnePath = [](juce::Path& path, const juce::Rectangle<float>& temp) {
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
            auto updateOnePath = [](juce::Path& path, const juce::Rectangle<float>& temp) {
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
            auto updateOnePath = [](juce::Path& path, const juce::Rectangle<float>& temp) {
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
