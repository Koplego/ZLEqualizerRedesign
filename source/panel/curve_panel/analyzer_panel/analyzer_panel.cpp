// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#include "analyzer_panel.hpp"
#include "../../../gui/glass_tokens.hpp"
#include "BinaryData.h"

namespace zlpanel {
    AnalyzerPanel::AnalyzerPanel(PluginProcessor& p, zlgui::UIBase& base,
                                 const multilingual::TooltipHelper& tooltip_helper) :
        base_(base),
        control_background_(base),
        pre_button_(base, "Pre", tooltip_helper.getToolTipText(multilingual::kFFTPre)),
        pre_attach_(pre_button_.getButton(), p.parameters_NA_, zlstate::PFFTPreON::kID, updater_),
        post_button_(base, "Post", tooltip_helper.getToolTipText(multilingual::kFFTPost)),
        post_attach_(post_button_.getButton(), p.parameters_NA_, zlstate::PFFTPostON::kID, updater_),
        side_button_(base, "Side", tooltip_helper.getToolTipText(multilingual::kFFTSide)),
        side_attach_(side_button_.getButton(), p.parameters_NA_, zlstate::PFFTSideON::kID, updater_),
        speed_box_(zlstate::PFFTSpeed::kChoices, base, tooltip_helper.getToolTipText(multilingual::kFFTSpeed)),
        speed_attach_(speed_box_.getBox(), p.parameters_NA_, zlstate::PFFTSpeed::kID, updater_),
        slope_box_(zlstate::PFFTTilt::kChoices, base, tooltip_helper.getToolTipText(multilingual::kFFTSlope)),
        slope_attach_(slope_box_.getBox(), p.parameters_NA_, zlstate::PFFTTilt::kID, updater_),
        smooth_oct_value_box_(zlstate::PFFTSmoothOCTValue::kChoices, base, ""),
        smooth_oct_value_attach_(smooth_oct_value_box_.getBox(), p.parameters_NA_, zlstate::PFFTSmoothOCTValue::kID, updater_),
        smooth_erb_value_box_(zlstate::PFFTSmoothERBValue::kChoices, base, ""),
        smooth_erb_value_attach_(smooth_erb_value_box_.getBox(), p.parameters_NA_, zlstate::PFFTSmoothERBValue::kID, updater_),
        smooth_type_box_(zlstate::PFFTSmoothType::kChoices, base, ""),
        smooth_type_attach_(smooth_type_box_.getBox(), p.parameters_NA_, zlstate::PFFTSmoothType::kID, updater_),
        freeze_drawable_(juce::Drawable::createFromImageData(BinaryData::freeze_svg, BinaryData::freeze_svgSize)),
        freeze_button_(base, freeze_drawable_.get(), freeze_drawable_.get(), tooltip_helper.getToolTipText(multilingual::kFFTFreeze)),
        freeze_attach_(freeze_button_.getButton(), p.parameters_NA_, zlstate::PFFTFreezeON::kID, updater_),
        lr_box_([]() -> std::vector<std::unique_ptr<juce::Drawable>> {
            std::vector<std::unique_ptr<juce::Drawable>> icons;
            icons.emplace_back(juce::Drawable::createFromImageData(BinaryData::stereo_svg, BinaryData::stereo_svgSize));
            icons.emplace_back(juce::Drawable::createFromImageData(BinaryData::left_svg, BinaryData::left_svgSize));
            icons.emplace_back(juce::Drawable::createFromImageData(BinaryData::right_svg, BinaryData::right_svgSize));
            icons.emplace_back(juce::Drawable::createFromImageData(BinaryData::mid_svg, BinaryData::mid_svgSize));
            icons.emplace_back(juce::Drawable::createFromImageData(BinaryData::side_svg, BinaryData::side_svgSize));
            return icons;
        }(), base, "", {"Stereo", "Left", "Right", "Mid", "Side"}),
        lr_attachment_(lr_box_.getBox(), p.parameters_NA_, zlstate::PFFTStereo::kID, updater_),
        collision_drawable_(juce::Drawable::createFromImageData(BinaryData::collision_svg, BinaryData::collision_svgSize)),
        collision_button_(base, collision_drawable_.get(), collision_drawable_.get(), tooltip_helper.getToolTipText(multilingual::kFFTCollision)),
        collision_attach_(collision_button_.getButton(), p.parameters_NA_, zlstate::PCollisionON::kID, updater_),
        label_laf_(base), strength_label_("", "Collision Strength"),
        strength_slider_("", base, tooltip_helper.getToolTipText(multilingual::kFFTCollisionStrength)),
        strength_attach_(strength_slider_.getSlider(), p.parameters_NA_, zlstate::PCollisionStrength::kID, updater_) {

        control_background_.setBufferedToImage(true);
        addAndMakeVisible(control_background_);

        // Pre, Post, Side and analyzer speed are first-class controls in the footer now.
        // Keep these attachment-owning components alive, but never show a second copy here.
        pre_button_.setVisible(false);
        post_button_.setVisible(false);
        side_button_.setVisible(false);
        speed_box_.setVisible(false);

        for (auto& c : {&slope_box_, &smooth_type_box_}) {
            c->getLAF().setFontScale(.74f);
            c->getLAF().setBoxAlpha(.36f);
            c->getLAF().setLabelJustification(juce::Justification::centred);
            c->setBufferedToImage(true);
            addAndMakeVisible(c);
        }

        for (auto* c : {&smooth_oct_value_box_, &smooth_erb_value_box_}) {
            c->getLAF().setFontScale(.72f);
            c->getLAF().setBoxAlpha(.30f);
            c->getLAF().setLabelJustification(juce::Justification::centred);
            c->setBufferedToImage(true);
        }
        addChildComponent(smooth_oct_value_box_);
        addAndMakeVisible(smooth_erb_value_box_);

        smooth_type_box_.getBox().onChange = [this]() {
            const auto selected_index = smooth_type_box_.getBox().getSelectedItemIndex();
            smooth_oct_value_box_.setVisible(selected_index == 0);
            smooth_erb_value_box_.setVisible(selected_index == 1);
        };

        const auto popup_option = juce::PopupMenu::Options().withPreferredPopupDirection(
            juce::PopupMenu::Options::PopupDirection::downwards);
        lr_box_.getLAF().setOption(popup_option);
        lr_box_.getLAF().setBoxAlpha(.30f);
        lr_box_.setBufferedToImage(true);
        addAndMakeVisible(lr_box_);

        for (auto& b : {&freeze_button_, &collision_button_}) {
            b->setImageAlpha(.34f, .60f, .94f, .94f);
            b->setBufferedToImage(true);
            addAndMakeVisible(b);
        }

        label_laf_.setFontScale(.74f);
        strength_label_.setJustificationType(juce::Justification::centredLeft);
        strength_label_.setLookAndFeel(&label_laf_);
        strength_label_.setAlpha(.64f);
        strength_label_.setBufferedToImage(true);
        addAndMakeVisible(strength_label_);

        strength_slider_.getSlider().setSliderSnapsToMousePosition(false);
        strength_slider_.setBufferedToImage(true);
        addAndMakeVisible(strength_slider_);

        base_.setPanelProperty(zlgui::PanelSettingIdx::kAnalyzerPanel, 0.);
        base_.getPanelValueTree().addListener(this);
    }

    AnalyzerPanel::~AnalyzerPanel() {
        base_.getPanelValueTree().removeListener(this);
    }

    int AnalyzerPanel::getIdealWidth() const {
        const auto font = base_.getFontSize();
        return juce::jmax(juce::roundToInt(font * 17.2f), 3 * getSliderWidth(font));
    }

    int AnalyzerPanel::getIdealHeight() const {
        const auto font = base_.getFontSize();
        const auto row = juce::jmax(getButtonSize(font), juce::roundToInt(font * 1.90f));
        const auto padding = getPaddingSize(font);
        return 4 * row + 4 * padding;
    }

    void AnalyzerPanel::resized() {
        const auto font = base_.getFontSize();
        const auto row = juce::jmax(getButtonSize(font), juce::roundToInt(font * 1.90f));
        const auto padding = getPaddingSize(font);

        auto bound = getLocalBounds();
        control_background_.setBounds(bound);
        bound.reduce(2 * padding, padding);

        // Explicitly clear the footer-owned duplicates.
        pre_button_.setBounds({});
        post_button_.setBounds({});
        side_button_.setBounds({});
        speed_box_.setBounds({});

        // Detail row 1: analyzer tilt.
        slope_box_.setBounds(bound.removeFromTop(row));
        bound.removeFromTop(padding / 2);

        // Detail row 2: smoothing amount and domain.
        {
            auto r = bound.removeFromTop(row);
            const auto gap = juce::jmax(2, padding / 3);
            const auto left_w = (r.getWidth() - gap) / 2;
            smooth_oct_value_box_.setBounds(r.removeFromLeft(left_w));
            smooth_erb_value_box_.setBounds(smooth_oct_value_box_.getBounds());
            r.removeFromLeft(gap);
            smooth_type_box_.setBounds(r);
        }
        bound.removeFromTop(padding / 2);

        // Detail row 3: freeze, stereo domain and collision detection.
        {
            auto r = bound.removeFromTop(row);
            const auto icon_size = juce::jmin(row, juce::roundToInt(font * 2.08f));
            const auto gap = juce::jmax(padding, (r.getWidth() - 3 * icon_size) / 4);
            r.removeFromLeft(gap);
            freeze_button_.setBounds(r.removeFromLeft(icon_size)); r.removeFromLeft(gap);
            lr_box_.setBounds(r.removeFromLeft(icon_size)); r.removeFromLeft(gap);
            collision_button_.setBounds(r.removeFromLeft(icon_size));
        }
        bound.removeFromTop(padding / 2);

        // Detail row 4: collision strength only. Nothing here mirrors the footer.
        {
            auto r = bound.removeFromTop(row);
            const auto label_w = juce::jmax(juce::roundToInt(font * 7.0f), r.getWidth() / 2);
            strength_label_.setBounds(r.removeFromLeft(label_w));
            r.removeFromLeft(padding / 2);
            strength_slider_.setBounds(r);
        }

        strength_slider_.setMouseDragSensitivity(getSliderDraggingDistance(font));
    }

    void AnalyzerPanel::repaintCallBackSlow() {
        updater_.updateComponents();
    }

    void AnalyzerPanel::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property) {
        if (base_.isPanelIdentifier(zlgui::PanelSettingIdx::kAnalyzerPanel, property)) {
            const auto open = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kAnalyzerPanel)) > .5;
            auto& animator = juce::Desktop::getInstance().getAnimator();
            animator.cancelAnimation(this, false);
            if (open) {
                setAlpha(0.f);
                setVisible(true);
                animator.fadeIn(this, 180);
            } else if (isVisible()) {
                animator.fadeOut(this, 140);
            }
        }
    }
}
