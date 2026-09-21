// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.

#include "output_panel.hpp"
#include "../../../gui/glass_tokens.hpp"
#include "BinaryData.h"
#include <cstdio>

namespace zlpanel {
    OutputPanel::OutputPanel(PluginProcessor& p, zlgui::UIBase& base,
                             const multilingual::TooltipHelper& tooltip_helper) :
        p_ref_(p), base_(base), updater_(), control_background_(base), name_laf_(base),
        gain_label_("", "Output Gain"), scale_label_("", "Gain Scale"),
        gain_slider_("", base_, tooltip_helper.getToolTipText(multilingual::kOutputGain)),
        gain_attach_(gain_slider_.getSlider(), p.parameters_, zlp::POutputGain::kID, updater_),
        scale_slider_("", base, tooltip_helper.getToolTipText(multilingual::kGainScale)),
        scale_attach_(scale_slider_.getSlider(), p.parameters_, zlp::PGainScale::kID, updater_),
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

        setOpaque(false);
        control_background_.setBufferedToImage(true);
        addAndMakeVisible(control_background_);

        name_laf_.setFontScale(.76f);
        for (auto* l : {&gain_label_, &scale_label_, &lookahead_label_}) {
            l->setLookAndFeel(&name_laf_);
            l->setJustificationType(juce::Justification::centredLeft);
            l->setAlpha(.68f);
            l->setBufferedToImage(true);
            addAndMakeVisible(*l);
        }

        auto setupValue = [this](auto& slider) {
            slider.setFontScale(.88f);
            slider.getSlider().setSliderSnapsToMousePosition(false);
            slider.setBufferedToImage(true);
            addAndMakeVisible(slider);
        };

        setupValue(gain_slider_);
        gain_slider_.setComponentID(zlp::POutputGain::kID);
        gain_slider_.setPrecision(3);
        gain_slider_.permitted_characters_ = "-+0123456789.";
        gain_slider_.value_formatter_ = [](const double value) -> std::string {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%+.2f dB", value);
            return buffer;
        };

        setupValue(scale_slider_);
        scale_slider_.setComponentID(zlp::PGainScale::kID);
        scale_slider_.setPrecision(3);
        scale_slider_.permitted_characters_ = "-+0123456789.";
        scale_slider_.value_formatter_ = [](const double value) -> std::string {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%.0f%%", value);
            return buffer;
        };

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
            repaint();
        };

        // Preserve all four ZL actions, but discard their legacy SVG presentation. The
        // panel paints a single set of Glass action pills over these transparent hit targets.
        for (auto& b : {&sgc_button_, &lm_button_, &agc_button_, &phase_button_}) {
            b->setImageAlpha(0.f, 0.f, 0.f, 0.f);
            b->setBufferedToImage(true);
            addAndMakeVisible(b);
        }

        setupValue(lookahead_slider_);
        lookahead_slider_.setPrecision(3);
        lookahead_slider_.permitted_characters_ = "0123456789.";
        lookahead_slider_.value_formatter_ = [](const double value) -> std::string {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), value < 10.0 ? "%.2f ms" : "%.1f ms", value);
            return buffer;
        };

        base_.setPanelProperty(zlgui::PanelSettingIdx::kOutputPanel, 0.);
        base_.getPanelValueTree().addListener(this);
    }

    OutputPanel::~OutputPanel() {
        base_.getPanelValueTree().removeListener(this);
        p_ref_.getController().setLoudnessMatchON(false);
    }

    int OutputPanel::getIdealWidth() const {
        return juce::jmax(juce::roundToInt(base_.getFontSize() * 19.0f), 280);
    }

    int OutputPanel::getIdealHeight() const {
        const auto font = base_.getFontSize();
        const auto row = juce::jmax(getButtonSize(font), juce::roundToInt(font * 1.92f));
        const auto padding = getPaddingSize(font);
        const auto header = juce::roundToInt(font * 2.55f);
        return header + 4 * row + 5 * padding;
    }

    void OutputPanel::paint(juce::Graphics& g) {
        const auto font = base_.getFontSize();
        g.setColour(zlgui::glass::textPrimary().withAlpha(.94f));
        g.setFont(juce::FontOptions(font * .98f));
        g.drawText("Output", title_bound_, juce::Justification::centredLeft, false);

        g.setColour(zlgui::glass::textSecondary().withAlpha(.48f));
        g.setFont(juce::FontOptions(font * .58f));
        g.drawText("Gain staging & compensation", subtitle_bound_, juce::Justification::centredLeft, false);

        const auto drawRow = [&g](const juce::Rectangle<int>& bounds) {
            if (bounds.isEmpty()) return;
            const auto r = bounds.toFloat().reduced(.5f);
            zlgui::glass::fillGlassSurface(g, r, juce::jmax(7.f, r.getHeight() * .28f), .038f, .072f, .075f);
        };
        drawRow(gain_surface_bound_);
        drawRow(scale_surface_bound_);
        drawRow(lookahead_surface_bound_);

        for (const auto& bounds : action_bounds_) {
            if (bounds.isEmpty()) continue;
            const auto r = bounds.toFloat().reduced(.6f);
            zlgui::glass::fillGlassSurface(g, r, r.getHeight() * .48f, .034f, .068f, .070f);
        }
    }

    void OutputPanel::paintOverChildren(juce::Graphics& g) {
        static constexpr std::array<const char*, 4> labels{"Static", "Match", "Auto", "Phase"};
        const std::array<bool, 4> states{
            sgc_button_.getToggleState(), lm_button_.getToggleState(),
            agc_button_.getToggleState(), phase_button_.getToggleState()
        };

        const auto font = base_.getFontSize();
        for (size_t i = 0; i < action_bounds_.size(); ++i) {
            if (action_bounds_[i].isEmpty()) continue;
            auto r = action_bounds_[i].toFloat().reduced(.6f);
            if (states[i]) {
                g.setColour(juce::Colour(116, 181, 245).withAlpha(.12f));
                g.fillRoundedRectangle(r.reduced(1.f), r.getHeight() * .46f);
                g.setColour(zlgui::glass::rim().withMultipliedAlpha(.68f));
                g.drawRoundedRectangle(r, r.getHeight() * .48f, .7f);
            }

            g.setColour(zlgui::glass::textPrimary().withAlpha(states[i] ? .94f : .58f));
            g.setFont(juce::FontOptions(font * .67f));
            g.drawText(labels[i], action_bounds_[i], juce::Justification::centred, false);

            if (states[i]) {
                const auto dot = juce::jmax(2.f, font * .13f);
                g.setColour(juce::Colour(153, 205, 255).withAlpha(.92f));
                g.fillEllipse(action_bounds_[i].getCentreX() - dot * .5f,
                              static_cast<float>(action_bounds_[i].getBottom()) - font * .37f,
                              dot, dot);
            }
        }
    }

    void OutputPanel::resized() {
        const auto font = base_.getFontSize();
        const auto row = juce::jmax(getButtonSize(font), juce::roundToInt(font * 1.92f));
        const auto padding = getPaddingSize(font);
        const auto header_h = juce::roundToInt(font * 2.55f);

        auto bound = getLocalBounds();
        control_background_.setBounds(bound);
        bound.reduce(2 * padding, padding);

        auto header = bound.removeFromTop(header_h);
        title_bound_ = header.removeFromTop(juce::roundToInt(font * 1.36f));
        subtitle_bound_ = header;
        bound.removeFromTop(padding / 2);

        const auto layoutValueRow = [this, font, padding](juce::Rectangle<int> row_bounds,
                                                          juce::Label& label,
                                                          auto& slider,
                                                          juce::Rectangle<int>& surface) {
            surface = row_bounds;
            auto content = row_bounds.reduced(juce::roundToInt(font * .72f), 0);
            const auto label_w = juce::jmax(juce::roundToInt(font * 6.4f), content.getWidth() * 2 / 5);
            label.setBounds(content.removeFromLeft(label_w));
            content.removeFromLeft(padding / 2);
            slider.setBounds(content);
        };

        layoutValueRow(bound.removeFromTop(row), gain_label_, gain_slider_, gain_surface_bound_);
        bound.removeFromTop(padding / 2);
        layoutValueRow(bound.removeFromTop(row), scale_label_, scale_slider_, scale_surface_bound_);
        bound.removeFromTop(padding);

        {
            auto actions = bound.removeFromTop(row);
            const auto gap = juce::jmax(3, padding / 3);
            const auto cell = (actions.getWidth() - 3 * gap) / 4;
            std::array<zlgui::button::ClickButton*, 4> buttons{
                &sgc_button_, &lm_button_, &agc_button_, &phase_button_
            };
            for (size_t i = 0; i < buttons.size(); ++i) {
                action_bounds_[i] = actions.removeFromLeft(cell);
                buttons[i]->setBounds(action_bounds_[i]);
                if (i + 1 < buttons.size()) actions.removeFromLeft(gap);
            }
        }
        bound.removeFromTop(padding);

        layoutValueRow(bound.removeFromTop(row), lookahead_label_, lookahead_slider_, lookahead_surface_bound_);

        const auto dragging_distance = getSliderDraggingDistance(font);
        gain_slider_.setMouseDragSensitivity(dragging_distance);
        scale_slider_.setMouseDragSensitivity(dragging_distance);
        lookahead_slider_.setMouseDragSensitivity(dragging_distance);
    }

    void OutputPanel::repaintCallBackSlow() {
        updater_.updateComponents();
        gain_slider_.updateDisplayValue();
        scale_slider_.updateDisplayValue();
        lookahead_slider_.updateDisplayValue();
        repaint();
    }

    void OutputPanel::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property) {
        if (base_.isPanelIdentifier(zlgui::PanelSettingIdx::kOutputPanel, property)) {
            setVisible(static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kOutputPanel)) > .5);
        }
    }
}
