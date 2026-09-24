// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.

#include "top_panel.hpp"
#include "../../gui/glass_tokens.hpp"
#include "BinaryData.h"

namespace zlpanel {
    TopPanel::TopPanel(PluginProcessor& p, zlgui::UIBase& base,
                       multilingual::TooltipHelper& tooltip_helper,
                       std::function<void()> settings_callback) :
        p_ref_(p), base_(base), updater_(),
        settings_callback_(std::move(settings_callback)),
        settings_button_(base, "", "Settings"),
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

        // Legacy controls stay alive where they own attachments, but the visible header is
        // intentionally reduced to the three zones in the concept: identity, preset, utility.
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

        settings_button_.setBackgroundPainter([](juce::Graphics&, juce::Button&, bool, bool) {});
        settings_button_.getButton().onClick = [this]() {
            if (settings_callback_) settings_callback_();
        };
        addAndMakeVisible(settings_button_);

        setInterceptsMouseClicks(true, true);
    }

    void TopPanel::paint(juce::Graphics& g) {
        auto bounds = getLocalBounds().toFloat();
        const auto font = base_.getFontSize();
        const auto padding = static_cast<float>(getPaddingSize(font));
        auto title_area_full = bounds.removeFromLeft(juce::jmax(205.f, font * 14.2f)).reduced(.5f);

        const auto orb_size = juce::jmax(23.f, font * 1.78f);
        auto orb = juce::Rectangle<float>(title_area_full.getX() + padding,
                                          title_area_full.getCentreY() - orb_size * .5f,
                                          orb_size, orb_size);
        juce::ColourGradient orb_gradient(juce::Colour(247, 252, 255).withAlpha(.48f),
                                          orb.getX(), orb.getY(),
                                          juce::Colour(143, 148, 153).withAlpha(.22f),
                                          orb.getRight(), orb.getBottom(), false);
        g.setGradientFill(orb_gradient);
        g.fillEllipse(orb);
        g.setColour(juce::Colour(255, 255, 255).withAlpha(.34f));
        g.drawEllipse(orb, .9f);
        auto crescent = orb.reduced(orb_size * .22f);
        crescent.translate(-orb_size * .09f, -orb_size * .09f);
        g.setColour(juce::Colour(255, 255, 255).withAlpha(.18f));
        juce::Path crescent_path;
        crescent_path.addCentredArc(crescent.getCentreX(), crescent.getCentreY(),
                                    crescent.getWidth() * .5f, crescent.getHeight() * .5f,
                                    0.f, .25f, 2.35f, true);
        g.strokePath(crescent_path, juce::PathStrokeType(1.05f, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));

        auto title = title_area_full.toNearestInt();
        title.setX(juce::roundToInt(orb.getRight() + padding * .82f));
        title.setWidth(juce::jmax(126, juce::roundToInt(font * 8.7f)));
        g.setColour(zlgui::glass::textPrimary().withAlpha(.94f));
        g.setFont(juce::FontOptions(font * .98f));
        g.drawText("ZL Equalizer 2", title, juce::Justification::centredLeft, false);

        auto edition = title;
        edition.setX(title.getRight());
        edition.setWidth(juce::roundToInt(font * 3.2f));
        g.setColour(zlgui::glass::textTertiary().withMultipliedAlpha(.86f));
        g.setFont(juce::FontOptions(font * .50f));
        g.drawText("GLASS", edition, juce::Justification::centredLeft, false);

        if (!preset_pill_bound_.isEmpty()) {
            auto pill = preset_pill_bound_.toFloat().reduced(.5f);
            zlgui::glass::fillGlassSurface(g, pill, pill.getHeight() * .5f, .065f, .10f, .17f);
            g.setColour(zlgui::glass::textPrimary().withAlpha(.82f));
            g.setFont(juce::FontOptions(font * .76f));
            auto text_bound = preset_pill_bound_.reduced(20, 0);
            text_bound.removeFromRight(juce::roundToInt(pill.getHeight() * .28f));
            const auto preset_name = preset_name_provider_ ? preset_name_provider_() : juce::String{"Default"};
            g.drawFittedText(preset_name, text_bound, juce::Justification::centred, 1);

            const auto cx = pill.getRight() - pill.getHeight() * .48f;
            const auto cy = pill.getCentreY();
            juce::Path chevron;
            chevron.startNewSubPath(cx - 3.f, cy - 1.f);
            chevron.lineTo(cx, cy + 2.f);
            chevron.lineTo(cx + 3.f, cy - 1.f);
            g.setColour(zlgui::glass::textSecondary().withMultipliedAlpha(.78f));
            g.strokePath(chevron, juce::PathStrokeType(.95f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
        }

        if (!utility_cluster_bound_.isEmpty()) {
            for (const auto& component_bound : {match_button_.getBounds(), ext_button_.getBounds(),
                                                bypass_button_.getBounds(), settings_button_.getBounds()}) {
                auto circle = component_bound.toFloat().reduced(1.f);
                zlgui::glass::fillGlassSurface(g, circle, circle.getHeight() * .5f, .075f, .125f, .14f);
            }

            const auto icon_stroke = juce::PathStrokeType(1.22f, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded);

            // EQ Match and Sidechain intentionally use a larger optical footprint than
            // before. Their circles were already the right size; only the glyphs were tiny.
            {
                auto mr = match_button_.getBounds().toFloat();
                auto r = mr.reduced(font * .50f);
                juce::Path p;
                p.startNewSubPath(r.getX(), r.getBottom() - r.getHeight() * .22f);
                p.cubicTo(r.getX() + r.getWidth() * .22f, r.getBottom() - r.getHeight() * .22f,
                          r.getX() + r.getWidth() * .28f, r.getY() + r.getHeight() * .16f,
                          r.getX() + r.getWidth() * .46f, r.getY() + r.getHeight() * .16f);
                p.cubicTo(r.getX() + r.getWidth() * .64f, r.getY() + r.getHeight() * .16f,
                          r.getX() + r.getWidth() * .70f, r.getBottom() - r.getHeight() * .34f,
                          r.getRight(), r.getBottom() - r.getHeight() * .34f);
                g.setColour(zlgui::glass::textPrimary().withAlpha(match_button_.getToggleState() ? .92f : .58f));
                g.strokePath(p, icon_stroke);
                const auto dot = juce::jmax(2.4f, font * .16f);
                g.fillEllipse(r.getX() + r.getWidth() * .42f, r.getY() + r.getHeight() * .10f, dot, dot);
            }

            {
                auto er = ext_button_.getBounds().toFloat().reduced(font * .52f);
                const auto alpha = ext_button_.getToggleState() ? .92f : .54f;
                g.setColour(zlgui::glass::textPrimary().withAlpha(alpha));
                juce::Path left_link, right_link;
                auto link = juce::Rectangle<float>(er.getCentreX() - er.getWidth() * .48f,
                                                   er.getCentreY() - er.getHeight() * .23f,
                                                   er.getWidth() * .56f, er.getHeight() * .46f);
                left_link.addRoundedRectangle(link, link.getHeight() * .48f);
                link.translate(er.getWidth() * .40f, 0.f);
                right_link.addRoundedRectangle(link, link.getHeight() * .48f);
                const auto rotation = juce::AffineTransform::rotation(-.55f, er.getCentreX(), er.getCentreY());
                g.strokePath(left_link, icon_stroke, rotation);
                g.strokePath(right_link, icon_stroke, rotation);
            }

            // Keep the bypass exactly as approved.
            auto pr = bypass_button_.getBounds().toFloat().reduced(font * .52f);
            g.setColour(zlgui::glass::textPrimary().withAlpha(bypass_button_.getToggleState() ? .38f : .90f));
            juce::Path power_arc;
            power_arc.addCentredArc(pr.getCentreX(), pr.getCentreY(), pr.getWidth() * .5f,
                                    pr.getHeight() * .5f, 0.f, .72f, 5.56f, true);
            g.strokePath(power_arc, juce::PathStrokeType(1.35f, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
            g.drawLine(pr.getCentreX(), pr.getY(), pr.getCentreX(), pr.getCentreY(), 1.35f);

            // Larger settings gear: the old reduction made it read like a speck inside its hit target.
            auto sr = settings_button_.getBounds().toFloat().reduced(font * .52f);
            g.setColour(zlgui::glass::textPrimary().withAlpha(.66f));
            g.drawEllipse(sr.withSizeKeepingCentre(sr.getWidth() * .72f, sr.getHeight() * .72f), 1.05f);
            g.drawEllipse(sr.withSizeKeepingCentre(sr.getWidth() * .24f, sr.getHeight() * .24f), 1.05f);
            for (int tooth = 0; tooth < 8; ++tooth) {
                const auto angle = juce::MathConstants<float>::twoPi * static_cast<float>(tooth) / 8.f;
                const auto inner = sr.getWidth() * .37f;
                const auto outer = sr.getWidth() * .50f;
                const auto centre = sr.getCentre();
                g.drawLine(centre.x + std::cos(angle) * inner, centre.y + std::sin(angle) * inner,
                           centre.x + std::cos(angle) * outer, centre.y + std::sin(angle) * outer, 1.05f);
            }
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

        const auto left_reserved = juce::jmax(210, juce::roundToInt(font_size * 14.4f));
        bound.removeFromLeft(left_reserved);

        const auto preset_w = juce::jmax(172, juce::roundToInt(font_size * 11.3f));
        preset_pill_bound_ = getLocalBounds().withSizeKeepingCentre(preset_w, button + padding / 3);

        const auto utility_button = juce::jmax(30, juce::roundToInt(button * .92f));
        settings_button_.setBounds(bound.removeFromRight(utility_button).withSizeKeepingCentre(utility_button, utility_button));
        bound.removeFromRight(padding / 3);
        bypass_button_.setBounds(bound.removeFromRight(utility_button).withSizeKeepingCentre(utility_button, utility_button));
        bound.removeFromRight(padding / 3);
        ext_button_.setBounds(bound.removeFromRight(utility_button).withSizeKeepingCentre(utility_button, utility_button));
        bound.removeFromRight(padding / 3);
        match_button_.setBounds(bound.removeFromRight(utility_button).withSizeKeepingCentre(utility_button, utility_button));
        bound.removeFromRight(padding / 2);
        const auto output_width = juce::jmax(68, juce::roundToInt(font_size * 4.7f));
        output_label_.setBounds(bound.removeFromRight(output_width));

        utility_cluster_bound_ = output_label_.getBounds()
            .getUnion(match_button_.getBounds())
            .getUnion(ext_button_.getBounds())
            .getUnion(bypass_button_.getBounds())
            .getUnion(settings_button_.getBounds())
            .expanded(juce::jmax(2, padding / 3), juce::jmax(2, padding / 4));

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
