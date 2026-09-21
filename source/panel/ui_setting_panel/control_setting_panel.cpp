// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#include "control_setting_panel.hpp"
#include "../../gui/glass_tokens.hpp"

namespace zlpanel {
    ControlSettingPanel::ControlSettingPanel(PluginProcessor& p, zlgui::UIBase& base) :
        p_ref_(p),
        base_(base), name_laf_(base),
        sensitivity_sliders_{
            {
                zlgui::slider::CompactLinearSlider<true, true, true>("Rough", base),
                zlgui::slider::CompactLinearSlider<true, true, true>("Fine", base),
                zlgui::slider::CompactLinearSlider<true, true, true>("Rough", base),
                zlgui::slider::CompactLinearSlider<true, true, true>("Fine", base),
                zlgui::slider::CompactLinearSlider<true, true, true>("Rough", base),
                zlgui::slider::CompactLinearSlider<true, true, true>("Fine", base),
                zlgui::slider::CompactLinearSlider<true, true, true>("Menu", base)
            }
        },
        wheel_reverse_box_(zlstate::PWheelShiftReverse::kChoices, base),
        rotary_style_box_(zlstate::PRotaryStyle::kChoices, base),
        rotary_drag_sensitivity_slider_("Distance", base),
        slider_double_click_box_(zlstate::PSliderDoubleClickFunc::kChoices, base),
        action_mouse_boxes_{
            zlgui::combobox::CompactCombobox(zlstate::PEnterSoloMouse::kChoices, base),
            zlgui::combobox::CompactCombobox(zlstate::PExitSoloMouse::kChoices, base),
            zlgui::combobox::CompactCombobox(zlstate::PRightClickMenuMouse::kChoices, base),
            zlgui::combobox::CompactCombobox(zlstate::PToggleDynamicMouse::kChoices, base),
            zlgui::combobox::CompactCombobox(zlstate::PToggleBypassMouse::kChoices, base),
            zlgui::combobox::CompactCombobox(zlstate::PDeleteBandMouse::kChoices, base)
        },
        action_key_boxes_{
            zlgui::combobox::CompactCombobox(zlstate::PEnterSoloKey::kChoices, base),
            zlgui::combobox::CompactCombobox(zlstate::PExitSoloKey::kChoices, base),
            zlgui::combobox::CompactCombobox(zlstate::PRightClickMenuKey::kChoices, base),
            zlgui::combobox::CompactCombobox(zlstate::PToggleDynamicKey::kChoices, base),
            zlgui::combobox::CompactCombobox(zlstate::PToggleBypassKey::kChoices, base),
            zlgui::combobox::CompactCombobox(zlstate::PDeleteBandKey::kChoices, base)
        } {
        juce::ignoreUnused(p_ref_);
        setOpaque(false);
        name_laf_.setFontScale(.76f);

        const auto configure_label = [this](juce::Label& label, const juce::String& text) {
            label.setText(text, juce::dontSendNotification);
            label.setJustificationType(juce::Justification::centredLeft);
            label.setLookAndFeel(&name_laf_);
            label.setAlpha(.76f);
            addAndMakeVisible(label);
        };

        configure_label(wheel_label_, "Mouse Wheel");
        configure_label(slider_label_, "Sliders");
        configure_label(dragger_label_, "EQ Nodes");
        configure_label(rotary_style_label_, "Rotary Controls");
        configure_label(slider_double_click_label_, "Double Click");

        for (auto& s : sensitivity_sliders_) {
            s.setFontScale(.72f);
            s.getSlider().setRange(0.01, 1.0, 0.01);
            s.getSlider().setSliderSnapsToMousePosition(false);
            addAndMakeVisible(s);
        }
        sensitivity_sliders_[0].getSlider().setDoubleClickReturnValue(true, 1.0);
        sensitivity_sliders_[1].getSlider().setDoubleClickReturnValue(true, 0.12);
        sensitivity_sliders_[2].getSlider().setDoubleClickReturnValue(true, 1.0);
        sensitivity_sliders_[3].getSlider().setDoubleClickReturnValue(true, 0.25);
        sensitivity_sliders_[4].getSlider().setDoubleClickReturnValue(true, 1.0);
        sensitivity_sliders_[5].getSlider().setDoubleClickReturnValue(true, 0.25);
        sensitivity_sliders_[6].getSlider().setDoubleClickReturnValue(true, 0.5);

        rotary_drag_sensitivity_slider_.getSlider().setRange(2.0, 32.0, 0.01);
        rotary_drag_sensitivity_slider_.setFontScale(.72f);
        rotary_drag_sensitivity_slider_.getSlider().setDoubleClickReturnValue(true, 10.0);
        rotary_drag_sensitivity_slider_.getSlider().setSliderSnapsToMousePosition(false);
        addAndMakeVisible(rotary_drag_sensitivity_slider_);

        auto style_combo = [](zlgui::combobox::CompactCombobox& combo) {
            combo.getLAF().setFontScale(.72f);
            combo.getLAF().setBoxAlpha(.38f);
            combo.getLAF().setLabelJustification(juce::Justification::centred);
            combo.setBufferedToImage(true);
        };

        style_combo(wheel_reverse_box_);
        style_combo(rotary_style_box_);
        style_combo(slider_double_click_box_);
        addAndMakeVisible(wheel_reverse_box_);
        addAndMakeVisible(rotary_style_box_);
        addAndMakeVisible(slider_double_click_box_);

        action_labels_[0].setText("Enter Solo", juce::dontSendNotification);
        action_labels_[1].setText("Exit Solo", juce::dontSendNotification);
        action_labels_[2].setText("Context Menu", juce::dontSendNotification);
        action_labels_[3].setText("Toggle Dynamic", juce::dontSendNotification);
        action_labels_[4].setText("Toggle Bypass", juce::dontSendNotification);
        action_labels_[5].setText("Delete Band", juce::dontSendNotification);

        for (size_t i = 0; i < 6; ++i) {
            action_labels_[i].setJustificationType(juce::Justification::centredLeft);
            action_labels_[i].setLookAndFeel(&name_laf_);
            action_labels_[i].setAlpha(.76f);
            addAndMakeVisible(action_labels_[i]);
            style_combo(action_mouse_boxes_[i]);
            style_combo(action_key_boxes_[i]);
            addAndMakeVisible(action_mouse_boxes_[i]);
            addAndMakeVisible(action_key_boxes_[i]);
        }
    }

    ControlSettingPanel::~ControlSettingPanel() = default;

    void ControlSettingPanel::loadSetting() {
        for (size_t i = 0; i < sensitivity_sliders_.size(); ++i) {
            sensitivity_sliders_[i].getSlider().setValue(static_cast<double>(base_.getSensitivity(
                static_cast<zlgui::SensitivityIdx>(i))));
        }
        wheel_reverse_box_.getBox().setSelectedItemIndex(static_cast<int>(base_.getIsMouseWheelShiftReverse()));
        rotary_style_box_.getBox().setSelectedItemIndex(static_cast<int>(base_.getRotaryStyleID()));
        rotary_drag_sensitivity_slider_.getSlider().setValue(static_cast<double>(base_.getRotaryDragSensitivity()));
        slider_double_click_box_.getBox().setSelectedItemIndex(
            static_cast<int>(base_.getIsSliderDoubleClickOpenEditor()));
        action_mouse_boxes_[0].getBox().setSelectedItemIndex(static_cast<int>(base_.getEnterSoloMouse()));
        action_key_boxes_[0].getBox().setSelectedItemIndex(static_cast<int>(base_.getEnterSoloKey()));
        action_mouse_boxes_[1].getBox().setSelectedItemIndex(static_cast<int>(base_.getExitSoloMouse()));
        action_key_boxes_[1].getBox().setSelectedItemIndex(static_cast<int>(base_.getExitSoloKey()));
        action_mouse_boxes_[2].getBox().setSelectedItemIndex(static_cast<int>(base_.getContextMenuMouse()));
        action_key_boxes_[2].getBox().setSelectedItemIndex(static_cast<int>(base_.getContextMenuKey()));
        action_mouse_boxes_[3].getBox().setSelectedItemIndex(static_cast<int>(base_.getToggleDynamicMouse()));
        action_key_boxes_[3].getBox().setSelectedItemIndex(static_cast<int>(base_.getToggleDynamicKey()));
        action_mouse_boxes_[4].getBox().setSelectedItemIndex(static_cast<int>(base_.getToggleBypassMouse()));
        action_key_boxes_[4].getBox().setSelectedItemIndex(static_cast<int>(base_.getToggleBypassKey()));
        action_mouse_boxes_[5].getBox().setSelectedItemIndex(static_cast<int>(base_.getDeleteBandMouse()));
        action_key_boxes_[5].getBox().setSelectedItemIndex(static_cast<int>(base_.getDeleteBandKey()));
    }

    void ControlSettingPanel::saveSetting() {
        for (size_t i = 0; i < sensitivity_sliders_.size(); ++i) {
            base_.setSensitivity(static_cast<float>(sensitivity_sliders_[i].getSlider().getValue()),
                                 static_cast<zlgui::SensitivityIdx>(i));
        }
        base_.setIsMouseWheelShiftReverse(static_cast<bool>(wheel_reverse_box_.getBox().getSelectedItemIndex()));
        base_.setRotaryStyleID(static_cast<size_t>(rotary_style_box_.getBox().getSelectedItemIndex()));
        base_.setRotaryDragSensitivity(static_cast<float>(rotary_drag_sensitivity_slider_.getSlider().getValue()));
        base_.setIsSliderDoubleClickOpenEditor(
            static_cast<bool>(slider_double_click_box_.getBox().getSelectedItemIndex()));
        base_.setEnterSoloMouse(
            static_cast<zlgui::MouseActionType>(action_mouse_boxes_[0].getBox().getSelectedItemIndex()));
        base_.setEnterSoloKey(static_cast<zlgui::KeyActionType>(action_key_boxes_[0].getBox().getSelectedItemIndex()));
        base_.setExitSoloMouse(
            static_cast<zlgui::MouseActionType>(action_mouse_boxes_[1].getBox().getSelectedItemIndex()));
        base_.setExitSoloKey(static_cast<zlgui::KeyActionType>(action_key_boxes_[1].getBox().getSelectedItemIndex()));
        base_.setContextMenuMouse(
            static_cast<zlgui::MouseActionType>(action_mouse_boxes_[2].getBox().getSelectedItemIndex()));
        base_.setContextMenuKey(
            static_cast<zlgui::KeyActionType>(action_key_boxes_[2].getBox().getSelectedItemIndex()));
        base_.setToggleDynamicMouse(
            static_cast<zlgui::MouseActionType>(action_mouse_boxes_[3].getBox().getSelectedItemIndex()));
        base_.setToggleDynamicKey(
            static_cast<zlgui::KeyActionType>(action_key_boxes_[3].getBox().getSelectedItemIndex()));
        base_.setToggleBypassMouse(
            static_cast<zlgui::MouseActionType>(action_mouse_boxes_[4].getBox().getSelectedItemIndex()));
        base_.setToggleBypassKey(
            static_cast<zlgui::KeyActionType>(action_key_boxes_[4].getBox().getSelectedItemIndex()));
        base_.setDeleteBandMouse(
            static_cast<zlgui::MouseActionType>(action_mouse_boxes_[5].getBox().getSelectedItemIndex()));
        base_.setDeleteBandKey(static_cast<zlgui::KeyActionType>(action_key_boxes_[5].getBox().getSelectedItemIndex()));
        base_.saveToAPVTS();
    }

    void ControlSettingPanel::resetSetting() {
    }

    int ControlSettingPanel::getIdealHeight() const {
        const auto font = base_.getFontSize();
        const auto padding = juce::roundToInt(font * .62f);
        const auto row = juce::roundToInt(font * 2.18f);
        const auto section = juce::roundToInt(font * 1.42f);
        return 2 * section + 11 * row + 12 * padding;
    }

    void ControlSettingPanel::resized() {
        const auto font = base_.getFontSize();
        const auto padding = juce::jmax(5, juce::roundToInt(font * .62f));
        const auto row = juce::jmax(28, juce::roundToInt(font * 2.18f));
        const auto section = juce::jmax(18, juce::roundToInt(font * 1.42f));
        const auto label_w = juce::jmax(126, juce::roundToInt(font * 9.6f));
        const auto gap = juce::jmax(4, padding / 2);

        row_bounds_.clear();
        auto bound = getLocalBounds().reduced(padding, 0);
        feel_title_bound_ = bound.removeFromTop(section);
        bound.removeFromTop(padding / 3);

        const auto add_row = [&](juce::Rectangle<int> r) {
            row_bounds_.push_back(r);
        };

        // Wheel: Rough / Fine / Menu / direction.
        {
            auto r = bound.removeFromTop(row); add_row(r);
            auto inner = r.reduced(padding / 2, 1);
            wheel_label_.setBounds(inner.removeFromLeft(juce::jmin(label_w, inner.getWidth() / 3)));
            inner.removeFromLeft(gap);
            const auto field_w = juce::jmax(54, inner.getWidth() / 5);
            sensitivity_sliders_[0].setBounds(inner.removeFromLeft(field_w)); inner.removeFromLeft(gap);
            sensitivity_sliders_[1].setBounds(inner.removeFromLeft(field_w)); inner.removeFromLeft(gap);
            sensitivity_sliders_[6].setBounds(inner.removeFromLeft(field_w)); inner.removeFromLeft(gap);
            wheel_reverse_box_.setBounds(inner.reduced(0, padding / 5));
            bound.removeFromTop(padding / 3);
        }
        // Slider sensitivity.
        {
            auto r = bound.removeFromTop(row); add_row(r);
            auto inner = r.reduced(padding / 2, 1);
            slider_label_.setBounds(inner.removeFromLeft(juce::jmin(label_w, inner.getWidth() / 3)));
            inner.removeFromLeft(gap);
            const auto field_w = juce::jmax(62, (inner.getWidth() - gap) / 2);
            sensitivity_sliders_[2].setBounds(inner.removeFromLeft(field_w)); inner.removeFromLeft(gap);
            sensitivity_sliders_[3].setBounds(inner);
            bound.removeFromTop(padding / 3);
        }
        // EQ node sensitivity.
        {
            auto r = bound.removeFromTop(row); add_row(r);
            auto inner = r.reduced(padding / 2, 1);
            dragger_label_.setBounds(inner.removeFromLeft(juce::jmin(label_w, inner.getWidth() / 3)));
            inner.removeFromLeft(gap);
            const auto field_w = juce::jmax(62, (inner.getWidth() - gap) / 2);
            sensitivity_sliders_[4].setBounds(inner.removeFromLeft(field_w)); inner.removeFromLeft(gap);
            sensitivity_sliders_[5].setBounds(inner);
            bound.removeFromTop(padding / 3);
        }
        // Rotary behavior.
        {
            auto r = bound.removeFromTop(row); add_row(r);
            auto inner = r.reduced(padding / 2, 1);
            rotary_style_label_.setBounds(inner.removeFromLeft(juce::jmin(label_w, inner.getWidth() / 3)));
            inner.removeFromLeft(gap);
            auto distance = inner.removeFromRight(juce::jmax(78, inner.getWidth() / 3));
            inner.removeFromRight(gap);
            rotary_style_box_.setBounds(inner.reduced(0, padding / 5));
            rotary_drag_sensitivity_slider_.setBounds(distance);
            bound.removeFromTop(padding / 3);
        }
        // Double-click behavior.
        {
            auto r = bound.removeFromTop(row); add_row(r);
            auto inner = r.reduced(padding / 2, 1);
            slider_double_click_label_.setBounds(inner.removeFromLeft(juce::jmin(label_w, inner.getWidth() / 3)));
            inner.removeFromLeft(gap);
            slider_double_click_box_.setBounds(inner.reduced(0, padding / 5));
            bound.removeFromTop(padding / 2);
        }

        shortcuts_title_bound_ = bound.removeFromTop(section);
        bound.removeFromTop(padding / 4);

        // Shortcuts use a stable three-column grid: action | mouse | keyboard.
        const auto available_w = bound.getWidth();
        const auto action_w = juce::jmax(label_w, available_w * 38 / 100);
        const auto field_w = juce::jmax(86, (available_w - action_w - 2 * gap) / 2);
        mouse_column_bound_ = {bound.getX() + action_w + gap, shortcuts_title_bound_.getY(),
                               field_w, shortcuts_title_bound_.getHeight()};
        key_column_bound_ = {mouse_column_bound_.getRight() + gap, shortcuts_title_bound_.getY(),
                             field_w, shortcuts_title_bound_.getHeight()};

        for (size_t i = 0; i < 6; ++i) {
            auto r = bound.removeFromTop(row); add_row(r);
            auto inner = r.reduced(padding / 2, 1);
            action_labels_[i].setBounds(inner.removeFromLeft(juce::jmin(action_w, inner.getWidth())));
            inner.removeFromLeft(gap);
            action_mouse_boxes_[i].setBounds(inner.removeFromLeft(juce::jmin(field_w, inner.getWidth())).reduced(0, padding / 5));
            if (inner.getWidth() > gap) inner.removeFromLeft(gap);
            action_key_boxes_[i].setBounds(inner.reduced(0, padding / 5));
            bound.removeFromTop(padding / 3);
        }
    }

    void ControlSettingPanel::paint(juce::Graphics& g) {
        const auto font = base_.getFontSize();
        g.setColour(zlgui::glass::textPrimary().withAlpha(.84f));
        g.setFont(juce::FontOptions(font * .72f));
        g.drawText("INTERACTION FEEL", feel_title_bound_, juce::Justification::centredLeft, false);
        g.drawText("SHORTCUTS", shortcuts_title_bound_, juce::Justification::centredLeft, false);

        g.setColour(zlgui::glass::textTertiary().withMultipliedAlpha(.92f));
        g.setFont(juce::FontOptions(font * .60f));
        g.drawText("MOUSE", mouse_column_bound_, juce::Justification::centred, false);
        g.drawText("KEY", key_column_bound_, juce::Justification::centred, false);

        for (const auto& row : row_bounds_) {
            auto r = row.toFloat().reduced(.5f);
            g.setColour(juce::Colour(238, 248, 255).withAlpha(.022f));
            g.fillRoundedRectangle(r, juce::jmax(6.f, font * .55f));
            g.setColour(zlgui::glass::rim().withMultipliedAlpha(.22f));
            g.drawRoundedRectangle(r, juce::jmax(6.f, font * .55f), .55f);
        }
    }
}
