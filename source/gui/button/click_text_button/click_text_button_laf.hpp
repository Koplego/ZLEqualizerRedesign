// Copyright (C) 2026 - zsliu98
// This file is part of ZLSpectrumEqualizer
//
// ZLSpectrumEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLSpectrumEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLSpectrumEqualizer. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <cmath>
#include <functional>
#include <utility>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../interface_definitions.hpp"

namespace zlgui {
    class ClickTextButtonLookAndFeel final : public juce::LookAndFeel_V4,
                                             private juce::Timer {
    public:
        using BackgroundPainter = std::function<void(juce::Graphics&, juce::Button&, bool, bool)>;

        explicit ClickTextButtonLookAndFeel(UIBase& base) : base_(base) {
        }

        void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                  const juce::Colour&, const bool highlight,
                                  const bool down) override {
            animated_button_ = &button;
            hover_target_ = highlight ? 1.f : 0.f;
            press_target_ = down ? 1.f : 0.f;
            if (std::abs(hover_amount_ - hover_target_) > .01f
                || std::abs(press_amount_ - press_target_) > .01f)
                startTimerHz(60);
            if (background_painter_) {
                background_painter_(g, button, highlight, down);
            }
            const auto energy = hover_amount_ * .045f + press_amount_ * .075f;
            if (energy > .002f) {
                const auto lens = button.getLocalBounds().toFloat().reduced(.75f);
                const auto radius = lens.getHeight() * .48f;
                g.setColour(juce::Colours::white.withAlpha(energy));
                g.fillRoundedRectangle(lens, radius);
                g.setColour(juce::Colours::white.withAlpha(energy * 1.3f));
                g.drawRoundedRectangle(lens, radius, .7f);
            }
        }

        void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                            const bool highlight, const bool down) override {
            if (down || button.getToggleState()) {
                g.setColour(base_.getTextColour());
            } else if (highlight) {
                g.setColour(base_.getTextColour().withAlpha(.75f));
            } else {
                g.setColour(base_.getTextColour().withAlpha(.5f));
            }
            g.setFont(base_.getFontSize() * font_scale_);
            // LookAndFeel painting is already in the button's local coordinate space.
            // Using getBounds() here offset every label by its parent position, which
            // caused the footer and inspector text to clip or overlap at real sizes.
            auto textBounds = button.getLocalBounds().reduced(
                juce::jmax(3, juce::roundToInt(base_.getFontSize() * .34f)), 1);
            g.drawText(button.getButtonText(), textBounds, justification_, true);
        }

        void setJustification(const juce::Justification justification) {
            justification_ = justification;
        }

        void setFontScale(const float font_scale) {
            font_scale_ = font_scale;
        }

        void setBackgroundPainter(BackgroundPainter painter) {
            background_painter_ = std::move(painter);
        }

    private:
        void timerCallback() override {
            hover_amount_ += (hover_target_ - hover_amount_) * .28f;
            press_amount_ += (press_target_ - press_amount_) * .38f;
            if (std::abs(hover_amount_ - hover_target_) < .01f
                && std::abs(press_amount_ - press_target_) < .01f) {
                hover_amount_ = hover_target_;
                press_amount_ = press_target_;
                stopTimer();
            }
            if (animated_button_ != nullptr) animated_button_->repaint();
        }

        UIBase& base_;
        juce::Button* animated_button_{nullptr};
        float hover_amount_{0.f}, press_amount_{0.f};
        float hover_target_{0.f}, press_target_{0.f};

        float font_scale_{1.f};
        juce::Justification justification_{juce::Justification::centredLeft};
        BackgroundPainter background_painter_;
    };
}
