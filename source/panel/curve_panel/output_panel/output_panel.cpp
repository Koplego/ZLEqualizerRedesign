// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.

#include "output_panel.hpp"
#include "BinaryData.h"

namespace zlpanel {
    OutputPanel::OutputPanel(PluginProcessor& p, zlgui::UIBase& base,
                             const multilingual::TooltipHelper& tooltip_helper) :
        p_ref_(p), base_(base), updater_(), control_background_(base), name_laf_(base),
        gain_label_("", "Output"), scale_label_("", "Scale"),
        gain_slider_("", base_, tooltip_helper.getToolTipText(multilingual::kOutputGain), 1.25f),
        gain_attach_(gain_slider_.getSlider1(), p.parameters_, zlp::POutputGain::kID, updater_),
        scale_slider_("", base, tooltip_helper.getToolTipText(multilingual::kGainScale), 1.25f),
        scale_attach_(scale_slider_.getSlider1(), p.parameters_, zlp::PGainScale::kID, updater_),
        sgc_drawable_(juce::Drawable::createFromImageData(BinaryData::dline_s_svg, BinaryData::dline_s_svgSize)),
        sgc_button_(base, sgc_drawable_.get(), sgc_drawable_.get(), tooltip_helper.getToolTipText(multilingual::kStaticGC)),
        sgc_attach_(sgc_button_.getButton(), p.parameters_, zlp::PStaticGain::kID, updater_),
        lm_drawable_(juce::Drawable::createFromImageData(BinaryData::dline_l_svg, BinaryData::dline_l_svgSize)),
        lm_button_(base, lm_drawable_.get(), lm_drawable_.get(), tooltip_helper.getToolTipText(multilingual::kLoudnessGC)),
        agc_drawable_(juce::Drawable::createFromImageData(BinaryData::dline_a_svg, BinaryData::dline_a_svgSize)),
        agc_button_(base, agc_drawable_.get(), agc_drawable_.get(), tooltip_helper.getToolTipText(multilingual::kAutoGC)),
        agc_attach_(agc_button_.getButton(), p.parameters_, zlp::PAutoGain::kID, updater_),
        phase_drawable_(juce::Drawable::createFromImageData(BinaryData::phase_svg, BinaryData::phase_svgSize)),
        phase_button_(base, phase_drawable_.get(), phase_drawable_.get(), tooltip_helper.getToolTipText(multilingual::kPhaseFlip)),
        phase_attach_(phase_button_.getButton(), p.parameters_, zlp::PPhaseFlip::kID, updater_),
        lookahead_label_("", "Lookahead"),
        lookahead_slider_("", base, tooltip_helper.getToolTipText(multilingual::kLookahead)),
        lookahead_attach_(lookahead_slider_.getSlider(), p.parameters_, zlp::PLookahead::kID, updater_) {

        control_background_.setBufferedToImage(true);
        addAndMakeVisible(control_background_);

        name_laf_.setFontScale(.78f);
        for (auto* l : {&gain_label_, &scale_label_}) {
            l->setLookAndFeel(&name_laf_);
            l->setJustificationType(juce::Justification::centred);
            l->setAlpha(.74f);
            l->setBufferedToImage(true);
            addAndMakeVisible(*l);
        }

        gain_slider_.setComponentID(zlp::POutputGain::kID);
        gain_slider_.setBufferedToImage(true);
        addAndMakeVisible(gain_slider_);
        scale_slider_.setComponentID(zlp::PGainScale::kID);
        scale_slider_.setBufferedToImage(true);
        addAndMakeVisible(scale_slider_);

        lm_button_.getButton().onClick = [this]() {
            if (lm_button_.getToggleState()) {
                p_ref_.getController().setLoudnessMatchON(true);
            } else {
                p_ref_.getController().setLoudnessMatchON(false);
                const auto c_diff = static_cast<float>(p_ref_.getController().getLUFSMatcherDiff());
                auto* output_gain_para = p_ref_.parameters_.getParameter(zlp::POutputGain::kID);
                updateValue(output_gain_para, output_gain_para->convertTo0to1(-c_diff));
                auto* agc_para = p_ref_.parameters_.getParameter(zlp::PAutoGain::kID);
                updateValue(agc_para, 0.f);
            }
        };

        for (auto& b : {&sgc_button_, &lm_button_, &agc_button_, &phase_button_}) {
            b->setImageAlpha(.34f, .56f, .92f, .92f);
            b->setBufferedToImage(true);
            addAndMakeVisible(b);
        }

        lookahead_label_.setLookAndFeel(&name_laf_);
        lookahead_label_.setJustificationType(juce::Justification::centredLeft);
        lookahead_label_.setAlpha(.68f);
        lookahead_label_.setBufferedToImage(true);
        addAndMakeVisible(lookahead_label_);

        lookahead_slider_.getSlider().setSliderSnapsToMousePosition(false);
        lookahead_slider_.setBufferedToImage(true);
        addAndMakeVisible(lookahead_slider_);

        base_.setPanelProperty(zlgui::PanelSettingIdx::kOutputPanel, 0.);
        base_.getPanelValueTree().addListener(this);
    }

    OutputPanel::~OutputPanel() {
        base_.getPanelValueTree().removeListener(this);
        p_ref_.getController().setLoudnessMatchON(false);
    }

    int OutputPanel::getIdealWidth() const {
        const auto font = base_.getFontSize();
        return juce::jmax(juce::roundToInt(font * 18.0f), 2 * getSliderWidth(font) + 4 * getPaddingSize(font));
    }

    int OutputPanel::getIdealHeight() const {
        const auto font = base_.getFontSize();
        const auto slider = getSliderWidth(font);
        const auto row = getButtonSize(font);
        const auto padding = getPaddingSize(font);
        return slider + 3 * row + 4 * padding;
    }

    void OutputPanel::resized() {
        const auto font = base_.getFontSize();
        const auto slider_width = getSliderWidth(font);
        const auto row = getButtonSize(font);
        const auto padding = getPaddingSize(font);

        auto bound = getLocalBounds();
        control_background_.setBounds(bound);
        bound.reduce(2 * padding, padding);

        auto labels = bound.removeFromTop(row * 3 / 4);
        const auto control_gap = juce::jmax(padding, bound.getWidth() - 2 * slider_width);
        gain_label_.setBounds(labels.removeFromLeft(slider_width));
        labels.removeFromLeft(juce::jmin(control_gap, labels.getWidth()));
        scale_label_.setBounds(labels.removeFromRight(juce::jmin(slider_width, labels.getWidth())));
        bound.removeFromTop(padding / 3);

        auto sliders = bound.removeFromTop(slider_width);
        gain_slider_.setBounds(sliders.removeFromLeft(slider_width));
        scale_slider_.setBounds(sliders.removeFromRight(slider_width));
        bound.removeFromTop(padding / 2);

        {
            auto actions = bound.removeFromTop(row);
            const auto icon = juce::jmin(row, juce::roundToInt(font * 1.95f));
            const auto gap = juce::jmax(2, (actions.getWidth() - 4 * icon) / 5);
            actions.removeFromLeft(gap);
            sgc_button_.setBounds(actions.removeFromLeft(icon)); actions.removeFromLeft(gap);
            lm_button_.setBounds(actions.removeFromLeft(icon)); actions.removeFromLeft(gap);
            agc_button_.setBounds(actions.removeFromLeft(icon)); actions.removeFromLeft(gap);
            phase_button_.setBounds(actions.removeFromLeft(icon));
        }
        bound.removeFromTop(padding / 2);

        {
            auto lookahead = bound.removeFromTop(row);
            const auto label_w = juce::jmax(juce::roundToInt(font * 5.0f), lookahead.getWidth() / 2);
            lookahead_label_.setBounds(lookahead.removeFromLeft(label_w));
            lookahead.removeFromLeft(padding / 2);
            lookahead_slider_.setBounds(lookahead);
        }

        const auto dragging_distance = getSliderDraggingDistance(font);
        gain_slider_.setMouseDragSensitivity(dragging_distance);
        scale_slider_.setMouseDragSensitivity(dragging_distance);
        lookahead_slider_.setMouseDragSensitivity(dragging_distance);
    }

    void OutputPanel::repaintCallBackSlow() {
        updater_.updateComponents();
    }

    void OutputPanel::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property) {
        if (base_.isPanelIdentifier(zlgui::PanelSettingIdx::kOutputPanel, property)) {
            setVisible(static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kOutputPanel)) > .5);
        }
    }
}
