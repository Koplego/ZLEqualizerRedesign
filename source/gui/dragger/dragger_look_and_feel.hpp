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

            // The node is the emitter, not a glossy bead. The larger environmental light
            // pool is painted by Dragger::paint(); this face is kept small, hot and bright
            // so it reads as the physical origin of that light.
            if (dragger_shape_ == kRound) {
                const juce::DropShadow closeBloom{
                    colour_.withAlpha((active ? .42f : hover ? .26f : .17f) * visibility),
                    juce::jmax(2, juce::roundToInt(base_.getFontSize() * (active ? .72f : .48f))),
                    {0, 0}};
                closeBloom.drawForPath(g, outline_path_);
            }

            const auto innerBounds = inner_path_.getBounds();
            const auto centre = innerBounds.getCentre();
            const auto radius = juce::jmax(1.f, innerBounds.getWidth() * .52f);

            // Quiet dark carrier underneath the emitter keeps the edge readable against
            // very bright FFT content without making the node look like a shaded sphere.
            g.setColour(juce::Colour(7, 22, 34).withAlpha((active ? .34f : .24f) * visibility));
            g.fillPath(outline_path_);

            // Coloured emission ring. Keep it translucent so the centre remains the hot spot.
            g.setColour(colour_.withAlpha((active ? .72f : hover ? .56f : .42f) * visibility));
            g.fillPath(outline_path_);

            // Radial emitter: near-white centre -> band colour -> transparent-ish edge.
            juce::ColourGradient emission(
                colour_.interpolatedWith(juce::Colours::white, active ? .88f : .76f)
                    .withAlpha((active ? 1.f : .96f) * visibility),
                centre.x, centre.y,
                colour_.withAlpha((active ? .94f : .84f) * visibility),
                centre.x + radius, centre.y, true);
            emission.addColour(.38, colour_.interpolatedWith(juce::Colours::white, .46f)
                                          .withAlpha((active ? .99f : .93f) * visibility));
            emission.addColour(.78, colour_.withAlpha((active ? .86f : .72f) * visibility));
            g.setGradientFill(emission);
            g.fillPath(inner_path_);

            // A single luminous rim is enough. No offset specular glint: the brightness is
            // energy coming out of the node, not light reflecting off a shiny ball.
            g.setColour(colour_.interpolatedWith(juce::Colours::white, .72f)
                        .withAlpha((active ? .92f : hover ? .76f : .62f) * visibility));
            g.strokePath(outline_path_, juce::PathStrokeType(juce::jmax(.72f, base_.getFontSize() * .070f)));

            if (dragger_shape_ == kRound) {
                auto hotCore = innerBounds.withSizeKeepingCentre(innerBounds.getWidth() * (active ? .28f : .22f),
                                                                 innerBounds.getHeight() * (active ? .28f : .22f));
                g.setColour(juce::Colours::white.withAlpha((active ? .84f : .64f) * visibility));
                g.fillEllipse(hotCore);
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
