// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#include "colour_setting_panel.hpp"
#include "../../gui/glass_tokens.hpp"

namespace zlpanel {
    static juce::Colour getIntColour(const int r, const int g, const int b, float alpha) {
        return {
            static_cast<juce::uint8>(r),
            static_cast<juce::uint8>(g),
            static_cast<juce::uint8>(b),
            alpha
        };
    }

    ColourSettingPanel::ColourSettingPanel(PluginProcessor& p, zlgui::UIBase& base) :
        pRef(p), base_(base), name_laf_(base),
        c_map1_selector_(base), c_map2_selector_(base) {
        juce::ignoreUnused(pRef);
        setOpaque(false);
        if (!kSettingDirectory.isDirectory()) {
            const auto result = kSettingDirectory.createDirectory();
            juce::ignoreUnused(result);
        }
        name_laf_.setFontScale(.76f);
        for (size_t i = 0; i < kNumSelectors; ++i) {
            auto label = std::string(zlgui::kColourNames[i]);
            label[0] = static_cast<char>(std::toupper(label[0]));
            label += " Colour";
            selector_labels_[i].setText(label, juce::dontSendNotification);
            selector_labels_[i].setJustificationType(juce::Justification::centredLeft);
            selector_labels_[i].setLookAndFeel(&name_laf_);
            selector_labels_[i].setAlpha(.76f);
            addAndMakeVisible(selector_labels_[i]);

            selectors_[i] = std::make_unique<zlgui::colour_selector::ColourOpacitySelector>(
                base, *this, i > 1,
                12.f, 10.f, 5.f, 5.f);
            selectors_[i]->setFontScale(.72f);
            addAndMakeVisible(*selectors_[i]);
        }
        c_map1_label_.setText("Band Palette", juce::dontSendNotification);
        c_map1_label_.setJustificationType(juce::Justification::centredLeft);
        c_map1_label_.setLookAndFeel(&name_laf_);
        c_map1_label_.setAlpha(.76f);
        addAndMakeVisible(c_map1_label_);
        addAndMakeVisible(c_map1_selector_);
        c_map2_label_.setText("Secondary Palette", juce::dontSendNotification);
        c_map2_label_.setJustificationType(juce::Justification::centredLeft);
        c_map2_label_.setLookAndFeel(&name_laf_);
        c_map2_label_.setAlpha(.76f);
        addAndMakeVisible(c_map2_label_);
        addAndMakeVisible(c_map2_selector_);
    }

    ColourSettingPanel::~ColourSettingPanel() {
        for (size_t i = 0; i < kNumSelectors; ++i) {
            selector_labels_[i].setLookAndFeel(nullptr);
        }
        c_map1_label_.setLookAndFeel(nullptr);
        c_map2_label_.setLookAndFeel(nullptr);
    }

    void ColourSettingPanel::loadSetting() {
        for (size_t i = 0; i < kNumSelectors; ++i) {
            selectors_[i]->setColour(base_.getColourByIdx(static_cast<zlgui::ColourIdx>(i)));
        }
        c_map1_selector_.getBox().setSelectedId(static_cast<int>(base_.getCMap1Idx()) + 1);
        c_map2_selector_.getBox().setSelectedId(static_cast<int>(base_.getCMap2Idx()) + 1);
    }

    void ColourSettingPanel::saveSetting() {
        for (size_t i = 0; i < kNumSelectors; ++i) {
            base_.setColourByIdx(static_cast<zlgui::ColourIdx>(i), selectors_[i]->getColour());
        }
        c_map1_selector_.getBox().setSelectedId(juce::jmax(1, c_map1_selector_.getBox().getSelectedId()),
                                                juce::dontSendNotification);
        c_map2_selector_.getBox().setSelectedId(juce::jmax(1, c_map2_selector_.getBox().getSelectedId()),
                                                juce::dontSendNotification);
        base_.setCMap1Idx(static_cast<size_t>(c_map1_selector_.getBox().getSelectedId() - 1));
        base_.setCMap2Idx(static_cast<size_t>(c_map2_selector_.getBox().getSelectedId() - 1));
        base_.saveToAPVTS();
    }

    void ColourSettingPanel::resetSetting() {
        for (size_t i = 0; i < kNumSelectors; ++i) {
            const auto dv = zlstate::kColourDefaults[i];
            selectors_[i]->setColour(getIntColour(dv.r, dv.g, dv.b, dv.opacity));
        }
        c_map1_selector_.getBox().setSelectedId(zlstate::PColourMap1Idx::kDefaultI + 1);
        c_map2_selector_.getBox().setSelectedId(zlstate::PColourMap2Idx::kDefaultI + 1);
        saveSetting();
    }

    int ColourSettingPanel::getIdealHeight() const {
        const auto font = base_.getFontSize();
        const auto padding = juce::roundToInt(font * .68f);
        const auto row = juce::roundToInt(font * 2.15f);
        const auto section = juce::roundToInt(font * 1.45f);
        return section * 2 + row * static_cast<int>(kNumSelectors + 2) +
               padding * static_cast<int>(kNumSelectors + 7);
    }

    void ColourSettingPanel::resized() {
        const auto font = base_.getFontSize();
        const auto padding = juce::jmax(5, juce::roundToInt(font * .68f));
        const auto row = juce::jmax(28, juce::roundToInt(font * 2.15f));
        const auto section_h = juce::jmax(18, juce::roundToInt(font * 1.45f));
        const auto label_w = juce::jmax(132, juce::roundToInt(font * 10.4f));

        row_bounds_.clear();
        auto bound = getLocalBounds().reduced(padding, 0);
        palette_title_bound_ = bound.removeFromTop(section_h);
        bound.removeFromTop(padding / 3);

        for (size_t i = 0; i < kNumSelectors; ++i) {
            auto row_bound = bound.removeFromTop(row);
            row_bounds_.push_back(row_bound);
            auto inner = row_bound.reduced(padding / 2, 1);
            selector_labels_[i].setBounds(inner.removeFromLeft(juce::jmin(label_w, inner.getWidth() / 2)));
            inner.removeFromLeft(padding);
            selectors_[i]->setBounds(inner);
            bound.removeFromTop(padding / 3);
        }

        bound.removeFromTop(padding / 2);
        maps_title_bound_ = bound.removeFromTop(section_h);
        bound.removeFromTop(padding / 3);

        auto layout_map = [&](juce::Label& label, juce::Component& selector) {
            auto row_bound = bound.removeFromTop(row);
            row_bounds_.push_back(row_bound);
            auto inner = row_bound.reduced(padding / 2, 1);
            label.setBounds(inner.removeFromLeft(juce::jmin(label_w, inner.getWidth() / 2)));
            inner.removeFromLeft(padding);
            selector.setBounds(inner.reduced(0, padding / 4));
            bound.removeFromTop(padding / 3);
        };
        layout_map(c_map1_label_, c_map1_selector_);
        layout_map(c_map2_label_, c_map2_selector_);
    }

    void ColourSettingPanel::paint(juce::Graphics& g) {
        const auto font = base_.getFontSize();
        g.setColour(zlgui::glass::textPrimary().withAlpha(.84f));
        g.setFont(juce::FontOptions(font * .72f));
        g.drawText("INTERFACE PALETTE", palette_title_bound_, juce::Justification::centredLeft, false);
        g.drawText("BAND COLOUR MAPS", maps_title_bound_, juce::Justification::centredLeft, false);

        for (const auto& row : row_bounds_) {
            auto r = row.toFloat().reduced(.5f);
            g.setColour(juce::Colour(238, 248, 255).withAlpha(.024f));
            g.fillRoundedRectangle(r, juce::jmax(6.f, font * .55f));
            g.setColour(zlgui::glass::rim().withMultipliedAlpha(.24f));
            g.drawRoundedRectangle(r, juce::jmax(6.f, font * .55f), .55f);
        }
    }
}
