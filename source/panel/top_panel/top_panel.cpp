// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.

#include "top_panel.hpp"
#include "../../gui/glass_tokens.hpp"
#include "BinaryData.h"

namespace zlpanel {
    TopPanel::TopPanel(PluginProcessor& p, zlgui::UIBase& base,
                       multilingual::TooltipHelper& tooltip_helper) :
        p_ref_(p), base_(base), updater_(),
        logo_panel_(p, base, tooltip_helper),
        output_label_(p, base),
        analyzer_label_(p, base),
        fstruct_box_(zlp::PFilterStructure::kChoices, base,
                     tooltip_helper.getToolTipText(multilingual::kFilterStructure),
                     {tooltip_helper.getToolTipText(multilingual::kMinimumPhase),
                      tooltip_helper.getToolTipText(multilingual::kStateVariable),
                      tooltip_helper.getToolTipText(multilingual::kParallel),
                      tooltip_helper.getToolTipText(multilingual::kMatchedPhase),
                      tooltip_helper.getToolTipText(multilingual::kMixedPhase),
                      tooltip_helper.getToolTipText(multilingual::kZeroPhase)}),
        fstruct_attach_(fstruct_box_.getBox(), p.parameters_, zlp::PFilterStructure::kID, updater_),
        preset_drawable_(juce::Drawable::createFromImageData(BinaryData::collections_bookmark_svg,
                                                             BinaryData::collections_bookmark_svgSize)),
        preset_button_(base, preset_drawable_.get(), nullptr, ""),
        bypass_drawable_(juce::Drawable::createFromImageData(BinaryData::bypass_svg,
                                                             BinaryData::bypass_svgSize)),
        bypass_button_(base, bypass_drawable_.get(), bypass_drawable_.get(),
                       tooltip_helper.getToolTipText(multilingual::kBypass)),
        bypass_attach_(bypass_button_.getButton(), p.parameters_, zlp::PBypass::kID, updater_),
        ext_drawable_(juce::Drawable::createFromImageData(BinaryData::externalside_svg,
                                                          BinaryData::externalside_svgSize)),
        ext_button_(base, ext_drawable_.get(), ext_drawable_.get(),
                    tooltip_helper.getToolTipText(multilingual::kExternalSideChain)),
        ext_attach_(ext_button_.getButton(), p.parameters_, zlp::PExtSide::kID, updater_),
        match_drawable_(juce::Drawable::createFromImageData(BinaryData::match_svg,
                                                            BinaryData::match_svgSize)),
        match_button_(base, match_drawable_.get(), match_drawable_.get(),
                      tooltip_helper.getToolTipText(multilingual::kEQMatch)) {
        setOpaque(false);

        // Legacy components remain alive where they carry attachments/behavior, while this
        // component owns the v1.2 visual presentation.
        logo_panel_.setVisible(false);
        analyzer_label_.setVisible(false);
        fstruct_box_.setVisible(false);
        preset_button_.setVisible(false);

        output_label_.setBufferedToImage(true);
        addAndMakeVisible(output_label_);

        bypass_button_.setImageAlpha(0.f, 0.f, 0.f, 0.f);
        bypass_button_.setBufferedToImage(true);
        addAndMakeVisible(bypass_button_);

        ext_button_.getButton().onClick = [this]() {
            if (ext_button_.getToggleState()) {
                auto* para = p_ref_.parameters_NA_.getParameter(zlstate::PFFTSideON::kID);
                updateValue(para, 1.f);
            }
        };
        ext_button_.setImageAlpha(0.f, 0.f, 0.f, 0.f);
        ext_button_.setBufferedToImage(true);
        addAndMakeVisible(ext_button_);

        match_button_.getButton().onClick = [this]() {
            base_.setPanelProperty(zlgui::PanelSettingIdx::kMatchPanel,
                                   static_cast<double>(match_button_.getToggleState()));
        };
        match_button_.setImageAlpha(0.f, 0.f, 0.f, 0.f);
        match_button_.setBufferedToImage(true);
        addAndMakeVisible(match_button_);

        setInterceptsMouseClicks(true, true);
    }

    void TopPanel::paint(juce::Graphics& g) {
        auto bounds = getLocalBounds().toFloat();
        const auto padding = static_cast<float>(getPaddingSize(base_.getFontSize()));
        const auto radius = juce::jmax(14.f, base_.getFontSize() * 1.18f);

        auto title_capsule = bounds.removeFromLeft(juce::jmax(190.f, base_.getFontSize() * 12.8f)).reduced(.5f);
        zlgui::glass::fillGlassSurface(g, title_capsule, radius, .085f, .14f, .14f);

        const auto orb_size = juce::jmax(24.f, base_.getFontSize() * 1.9f);
        auto orb = juce::Rectangle<float>(title_capsule.getX() + padding,
                                          title_capsule.getCentreY() - orb_size * .5f,
                                          orb_size, orb_size);
        juce::ColourGradient orb_gradient(juce::Colour(245, 252, 255).withAlpha(.55f),
                                          orb.getX(), orb.getY(),
                                          juce::Colour(87, 140, 185).withAlpha(.24f),
                                          orb.getRight(), orb.getBottom(), false);
        g.setGradientFill(orb_gradient);
        g.fillEllipse(orb);
        g.setColour(juce::Colour(255, 255, 255).withAlpha(.42f));
        g.drawEllipse(orb, 1.f);
        auto crescent = orb.reduced(orb_size * .20f);
        crescent.translate(-orb_size * .10f, -orb_size * .10f);
        g.setColour(juce::Colour(255, 255, 255).withAlpha(.20f));
        juce::Path crescent_path;
        crescent_path.addCentredArc(crescent.getCentreX(), crescent.getCentreY(),
                                    crescent.getWidth() * .5f, crescent.getHeight() * .5f,
                                    0.f, .25f, 2.35f, true);
        g.strokePath(crescent_path, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));

        auto title_area = title_capsule.toNearestInt();
        title_area.setX(juce::roundToInt(orb.getRight() + padding * .9f));
        title_area.setWidth(juce::jmax(105, juce::roundToInt(base_.getFontSize() * 7.1f)));
        g.setColour(juce::Colour(248, 252, 255).withAlpha(.95f));
        g.setFont(juce::FontOptions(base_.getFontSize() * 1.02f));
        g.drawText("Glass EQ", title_area, juce::Justification::centredLeft, false);

        auto version = title_area;
        version.setX(title_area.getRight());
        version.setWidth(juce::roundToInt(base_.getFontSize() * 2.1f));
        g.setColour(juce::Colour(232, 244, 252).withAlpha(.36f));
        g.setFont(juce::FontOptions(base_.getFontSize() * .55f));
        g.drawText("1.2", version, juce::Justification::centredLeft, false);

        if (!preset_pill_bound_.isEmpty()) {
            auto pill = preset_pill_bound_.toFloat().reduced(.5f);
            zlgui::glass::fillGlassSurface(g, pill, pill.getHeight() * .5f, .12f, .17f, .18f);
            g.setColour(base_.getTextColour().withAlpha(.80f));
            g.setFont(juce::FontOptions(base_.getFontSize() * .78f));
            g.drawText("Presets", preset_pill_bound_.reduced(18, 0), juce::Justification::centred, false);
            const auto cx = pill.getRight() - pill.getHeight() * .52f;
            const auto cy = pill.getCentreY();
            juce::Path chevron;
            chevron.startNewSubPath(cx - 3.f, cy - 1.f);
            chevron.lineTo(cx, cy + 2.f);
            chevron.lineTo(cx + 3.f, cy - 1.f);
            g.setColour(juce::Colour(244, 251, 255).withAlpha(.48f));
            g.strokePath(chevron, juce::PathStrokeType(1.f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
        }

        if (!utility_cluster_bound_.isEmpty()) {
            auto utility = utility_cluster_bound_.toFloat().reduced(.5f);
            zlgui::glass::fillGlassSurface(g, utility, utility.getHeight() * .5f, .09f, .16f, .15f);

            // EQ Match is a first-class workspace in v1.2, so unlike the previous cryptic wave
            // icon it gets a stable text label while retaining the waveform cue.
            {
                auto mr = match_button_.getBounds().toFloat();
                auto wave_area = mr.removeFromLeft(juce::jmin(mr.getHeight(), mr.getWidth() * .38f));
                auto r = wave_area.reduced(base_.getFontSize() * .45f);
                juce::Path p;
                p.startNewSubPath(r.getX(), r.getCentreY());
                p.cubicTo(r.getX() + r.getWidth() * .22f, r.getY(),
                          r.getX() + r.getWidth() * .28f, r.getBottom(),
                          r.getX() + r.getWidth() * .50f, r.getCentreY());
                p.cubicTo(r.getX() + r.getWidth() * .72f, r.getY(),
                          r.getX() + r.getWidth() * .78f, r.getBottom(),
                          r.getRight(), r.getCentreY());
                g.setColour(juce::Colour(243, 250, 255).withAlpha(match_button_.getToggleState() ? .92f : .55f));
                g.strokePath(p, juce::PathStrokeType(1.25f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
                g.setFont(juce::FontOptions(base_.getFontSize() * .64f));
                g.drawText("Match", mr.toNearestInt(), juce::Justification::centredLeft, false);
            }

            auto er = ext_button_.getBounds().toFloat().reduced(base_.getFontSize() * .58f);
            const auto dot = juce::jmax(3.f, er.getHeight() * .24f);
            g.setColour(juce::Colour(243, 250, 255).withAlpha(ext_button_.getToggleState() ? .90f : .45f));
            g.drawEllipse(er.getX(), er.getCentreY() - dot * .5f, dot, dot, 1.2f);
            g.drawEllipse(er.getRight() - dot, er.getCentreY() - dot * .5f, dot, dot, 1.2f);
            g.drawLine(er.getX() + dot, er.getCentreY(), er.getRight() - dot, er.getCentreY(), 1.2f);

            auto pr = bypass_button_.getBounds().toFloat().reduced(base_.getFontSize() * .50f);
            g.setColour(juce::Colour(248, 252, 255).withAlpha(bypass_button_.getToggleState() ? .42f : .92f));
            juce::Path power_arc;
            power_arc.addCentredArc(pr.getCentreX(), pr.getCentreY(), pr.getWidth() * .5f,
                                    pr.getHeight() * .5f, 0.f, .72f, 5.56f, true);
            g.strokePath(power_arc, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
            g.drawLine(pr.getCentreX(), pr.getY(), pr.getCentreX(), pr.getCentreY(), 1.5f);
        }
    }

    int TopPanel::getIdealHeight() const {
        const auto font_size = base_.getFontSize();
        return getButtonSize(font_size) + 3 * getPaddingSize(font_size);
    }

    void TopPanel::resized() {
        const auto font_size = base_.getFontSize();
        const auto padding = getPaddingSize(font_size);
        const auto button = getButtonSize(font_size);
        auto bound = getLocalBounds().reduced(padding, padding);

        logo_panel_.setBounds({});
        analyzer_label_.setBounds({});
        fstruct_box_.setBounds({});
        preset_button_.setBounds({});

        const auto left_reserved = juce::jmax(195, juce::roundToInt(font_size * 13.1f));
        bound.removeFromLeft(left_reserved);

        const auto preset_w = juce::jmax(132, juce::roundToInt(font_size * 8.9f));
        preset_pill_bound_ = getLocalBounds().withSizeKeepingCentre(preset_w, button + padding / 2);

        bypass_button_.setBounds(bound.removeFromRight(button));
        bound.removeFromRight(padding / 4);
        ext_button_.setBounds(bound.removeFromRight(button));
        bound.removeFromRight(padding / 4);
        const auto match_width = juce::jmax(button * 2, juce::roundToInt(font_size * 5.2f));
        match_button_.setBounds(bound.removeFromRight(match_width));
        bound.removeFromRight(padding / 3);
        const auto output_width = juce::jmax(72, juce::roundToInt(font_size * 5.0f));
        output_label_.setBounds(bound.removeFromRight(output_width));

        utility_cluster_bound_ = output_label_.getBounds()
            .getUnion(match_button_.getBounds())
            .getUnion(ext_button_.getBounds())
            .getUnion(bypass_button_.getBounds())
            .expanded(juce::jmax(3, padding / 2), juce::jmax(2, padding / 3));

        repaint();
    }

    void TopPanel::repaintCallbackSlow() {
        output_label_.repaintCallbackSlow();
        updater_.updateComponents();
        match_button_.getButton().setToggleState(
            static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kMatchPanel)) > .5,
            juce::dontSendNotification);
        repaint();
    }

    void TopPanel::mouseDown(const juce::MouseEvent& event) {
        if (preset_pill_bound_.contains(event.getPosition())) {
            const auto panel_open = static_cast<float>(
                base_.getPanelProperty(zlgui::PanelSettingIdx::kPresetBrowser));
            base_.setPanelProperty(zlgui::PanelSettingIdx::kPresetBrowser, panel_open < .5f ? 1.f : 0.f);
        }
    }
}
