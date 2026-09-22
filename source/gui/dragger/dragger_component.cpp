// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#include "dragger_component.hpp"

namespace zlgui::dragger {
    Dragger::Dragger(UIBase& base)
        : base_(base), dragger_laf_(base) {
        dragger_laf_.setColour(base_.getColourMap1(1));
        button_.addMouseListener(this, false);
        button_.setClickingTogglesState(false);
        button_.setLookAndFeel(&dragger_laf_);
        addAndMakeVisible(button_);

        setInterceptsMouseClicks(false, true);
    }

    Dragger::~Dragger() {
        button_.removeMouseListener(this);
    }

    void Dragger::paint(juce::Graphics& g) {
        if (dragger_laf_.getDraggerShape() != DraggerLookAndFeel::kRound
            || !std::isfinite(button_pos_.x) || !std::isfinite(button_pos_.y)
            || button_pos_.x < -1000.f || button_pos_.y < -1000.f) {
            return;
        }

        const auto visibility = juce::jlimit(0.f, 1.f, dragger_laf_.getAlpha());
        if (visibility <= .001f) return;

        const auto active = button_.getToggleState() || dragger_laf_.getIsSelected();
        const auto hover = button_.isMouseOverOrDragging() && !active;
        const auto colour = dragger_laf_.getColour();
        const auto font = base_.getFontSize();

        // Reference-match the concept: no visible concentric rings, but a soft halo that
        // is strongest immediately outside the bright rim and becomes almost invisible
        // within roughly half a node radius. The opaque child disc hides the centre of
        // this gradient, so the apparent source is the perimeter rather than the middle.
        const auto radius = font * (active ? 1.72f : hover ? 1.62f : 1.55f);
        const auto centreAlpha = active ? .180f : hover ? .125f : .085f;
        juce::ColourGradient bloom(
            colour.interpolatedWith(juce::Colours::white, active ? .20f : .13f)
                  .withAlpha(centreAlpha * visibility),
            button_pos_.x, button_pos_.y,
            colour.withAlpha(0.f),
            button_pos_.x + radius, button_pos_.y, true);
        bloom.addColour(.52, colour.interpolatedWith(juce::Colours::white, .09f)
                                  .withAlpha(centreAlpha * .72f * visibility));
        bloom.addColour(.72, colour.withAlpha(centreAlpha * .38f * visibility));
        bloom.addColour(.90, colour.withAlpha(centreAlpha * .070f * visibility));
        bloom.addColour(.98, colour.withAlpha(0.f));
        g.setGradientFill(bloom);
        g.fillEllipse(button_pos_.x - radius, button_pos_.y - radius,
                      radius * 2.f, radius * 2.f);
    }

    bool Dragger::updateButton(const juce::Point<float>& center) {
        if (std::isfinite(center.x) && std::isfinite(center.y)) {
            if (std::abs(button_pos_.x - center.x) > 0.1f || std::abs(button_pos_.y - center.y) > 0.1f) {
                const auto old = button_pos_;
                button_pos_ = center;
                button_.setTransform(juce::AffineTransform::translation(button_pos_.x, button_pos_.y));

                const auto r = juce::roundToInt(base_.getFontSize() * 1.95f);
                if (std::isfinite(old.x) && std::isfinite(old.y) && old.x > -1000.f && old.y > -1000.f)
                    repaint(juce::roundToInt(old.x) - r, juce::roundToInt(old.y) - r, r * 2, r * 2);
                repaint(juce::roundToInt(center.x) - r, juce::roundToInt(center.y) - r, r * 2, r * 2);
                return true;
            }
        }
        return false;
    }

    bool Dragger::updateButton() {
        return updateButton(current_pos_);
    }

    void Dragger::mouseDown(const juce::MouseEvent& e) {
        current_pos_ = button_pos_;
        global_pos_ = e.position + button_pos_;
        button_.setToggleState(true, juce::NotificationType::sendNotificationSync);
        repaint();
        const BailOutChecker checker(this);
        listeners_.callChecked(checker, [&](Dragger::Listener& l) { l.dragStarted(this); });
    }

    void Dragger::mouseUp(const juce::MouseEvent& e) {
        juce::ignoreUnused(e);
        repaint();
        const BailOutChecker checker(this);
        listeners_.callChecked(checker, [&](Dragger::Listener& l) { l.dragEnded(this); });
    }

    void Dragger::mouseDrag(const juce::MouseEvent& e) {
        // calculate shift and update global position
        const auto new_global_pos = e.position + button_pos_;
        auto shift = new_global_pos - global_pos_;
        const auto old_shift = shift;
        // apply sensitivity
        if (e.mods.isShiftDown()) {
            shift.setX(shift.getX() * base_.getSensitivity(SensitivityIdx::kMouseDraggerFine));
            shift.setY(shift.getY() * base_.getSensitivity(SensitivityIdx::kMouseDraggerFine));
        } else {
            shift.setX(shift.getX() * base_.getSensitivity(SensitivityIdx::kMouseDragger));
            shift.setY(shift.getY() * base_.getSensitivity(SensitivityIdx::kMouseDragger));
        }
        if (e.mods.isCommandDown()) {
            if (e.mods.isLeftButtonDown()) {
                shift.setX(0.f);
            } else {
                shift.setY(0.f);
            }
        }
        if (!x_enabled_) {
            shift.setX(0.f);
        }
        if (!y_enabled_) {
            shift.setY(0.f);
        }
        // update current position
        const auto old_current_pos = current_pos_;
        if (check_center_) {
            current_pos_ = check_center_(current_pos_, current_pos_ + shift);
        } else {
            current_pos_ = current_pos_ + shift;
        }
        current_pos_ = button_area_.getConstrainedPoint(current_pos_);
        // shift global position accordingly
        const auto actual_shift = current_pos_ - old_current_pos;
        // consume pointer motion on a locked axis so it is not replayed when the axis is unlocked.
        if (std::abs(shift.x) > 1e-10f) {
            global_pos_.x += actual_shift.x / shift.x * old_shift.x;
        } else {
            global_pos_.x += old_shift.x;
        }
        if (std::abs(shift.y) > 1e-10f) {
            global_pos_.y += actual_shift.y / shift.y * old_shift.y;
        } else {
            global_pos_.y += old_shift.y;
        }
        // update x/y portion
        x_portion_ = (current_pos_.getX() - button_area_.getX()) / button_area_.getWidth();
        y_portion_ = 1.f - (current_pos_.getY() - button_area_.getY()) / button_area_.getHeight();
        // call listeners
        const BailOutChecker checker(this);
        listeners_.callChecked(checker, [&](Listener& l) { l.draggerValueChanged(this); });
    }

    void Dragger::setButtonArea(const juce::Rectangle<float> bound) {
        button_area_ = bound;

        const auto radius = static_cast<int>(std::round(base_.getFontSize() * scale_ * .5f));
        button_.setBounds(juce::Rectangle<int>(-radius, -radius, radius * 2, radius * 2));

        auto laf_bound = button_.getBounds().toFloat().withPosition(0.f, 0.f);
        dragger_laf_.updatePaths(laf_bound);
        repaint();
    }

    void Dragger::addListener(Listener* listener) {
        listeners_.add(listener);
    }

    void Dragger::removeListener(Listener* listener) {
        listeners_.remove(listener);
    }

    void Dragger::setXPortion(const float x) {
        x_portion_ = x;
        current_pos_.x = button_area_.getX() + x * button_area_.getWidth();
    }

    void Dragger::setYPortion(const float y) {
        y_portion_ = y;
        current_pos_.y = button_area_.getY() + (1.f - y) * button_area_.getHeight();
    }

    float Dragger::getXPortion() const {
        return x_portion_;
    }

    float Dragger::getYPortion() const {
        return y_portion_;
    }
}
