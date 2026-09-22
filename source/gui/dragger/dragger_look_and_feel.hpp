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

        explicit DraggerLookAndFeel(UIBase& base) : base_(base) {
        }

        void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                              bool should_draw_button_as_highlighted,
                              bool should_draw_button_as_down) override {
            const auto active = should_draw_button_as_down || button.getToggleState() || is_selected_;
            const auto hover = should_draw_button_as_highlighted && !active;
            const auto visibility = juce::jlimit(0.f, 1.f, alpha_);

            // Reference node = small bright glass lens + large soft coloured halo. The halo
            // is intentionally much larger than the control itself; that bloom is one of the
            // dominant visual cues in the approved screenshot.
            if (dragger_shape_ == kRound) {
                const juce::DropShadow outer_halo{
                    colour_.withAlpha((active ? .40f : hover ? .28f : .20f) * visibility),
                    juce::jmax(5, juce::roundToInt(base_.getFontSize() * (active ? 1.62f : .94f))),
                    {0, 0}};
                outer_halo.drawForPath(g, outline_path_);

                const juce::DropShadow hot_halo{
                    colour_.interpolatedWith(juce::Colours::white, .22f)
                        .withAlpha((active ? .29f : .13f) * visibility),
                    juce::jmax(3, juce::roundToInt(base_.getFontSize() * (active ? .72f : .45f))),
                    {0, 0}};
                hot_halo.drawForPath(g, inner_path_);
            }

            // The target does not have a heavy black doughnut around the coloured lens.
            // Keep just enough dark body to establish transparent glass depth.
            g.setColour(juce::Colour(7, 25, 42).withAlpha((active ? .30f : .22f) * visibility));
            g.fillPath(outline_path_);

            g.setColour(colour_.interpolatedWith(juce::Colours::white, .12f)
                        .withAlpha((active ? .24f : hover ? .18f : .13f) * visibility));
            g.fillPath(outline_path_);

            // Outer white optical ring.
            g.setColour(juce::Colours::white.withAlpha((active ? .96f : hover ? .79f : .69f) * visibility));
            g.strokePath(outline_path_, juce::PathStrokeType(juce::jmax(1.0f, base_.getFontSize() * .080f)));

            const auto innerBounds = inner_path_.getBounds();
            juce::ColourGradient lens(
                colour_.interpolatedWith(juce::Colours::white, active ? .48f : .37f)
                    .withAlpha((active ? .98f : .93f) * visibility),
                innerBounds.getX() + innerBounds.getWidth() * .25f,
                innerBounds.getY() + innerBounds.getHeight() * .15f,
                colour_.interpolatedWith(juce::Colours::black, .04f)
                    .withAlpha((active ? .96f : .90f) * visibility),
                innerBounds.getRight(), innerBounds.getBottom(), true);
            lens.addColour(.45, colour_.interpolatedWith(juce::Colours::white, .18f)
                                      .withAlpha((active ? .98f : .94f) * visibility));
            g.setGradientFill(lens);
            g.fillPath(inner_path_);

            // Bright inner rim gives the lens the milky-white edge visible in the target.
            g.setColour(juce::Colours::white.withAlpha((active ? .99f : hover ? .86f : .76f) * visibility));
            g.strokePath(inner_path_, juce::PathStrokeType(juce::jmax(.95f, base_.getFontSize() * .072f)));

            if (dragger_shape_ == kRound) {
                auto glint = innerBounds.withSizeKeepingCentre(innerBounds.getWidth() * .38f,
                                                               innerBounds.getHeight() * .18f);
                glint.translate(-innerBounds.getWidth() * .13f, -innerBounds.getHeight() * .22f);
                g.setColour(juce::Colours::white.withAlpha((active ? .38f : .22f) * visibility));
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
            case kRound: {
                updateRoundPaths(reduced_bound);
                break;
            }
            case kRectangle: {
                updateRectanglePaths(reduced_bound);
                break;
            }
            case kUpDownArrow: {
                updateUpDownArrowPaths(reduced_bound);
                break;
            }
            case kRightArrow: {
                updateRightArrowPaths(reduced_bound);
                break;
            }
            case kLeftArrow: {
                updateLeftArrowPaths(reduced_bound);
                break;
            }
            }
        }

        void updateRoundPaths(juce::Rectangle<float>& bound) {
            const auto radius = bound.getWidth();
            // Preserve the large interaction target. Visually, the coloured lens nearly
            // fills the white ring in the reference, so use a much larger inner disc than
            // the previous 66% implementation.
            bound = bound.withSizeKeepingCentre(radius * .90f, radius * .90f);
            outline_path_.addEllipse(bound);
            bound = bound.withSizeKeepingCentre(radius * .76f, radius * .76f);
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

        void setLabel(const juce::String& l) {
            label_ = l;
        }

        void setLabelScale(const float x) {
            label_scale_ = x;
        }

        void setPaddingScale(const float x) {
            padding_scale_ = x;
        }

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
