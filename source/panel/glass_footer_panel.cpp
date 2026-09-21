// Copyright (C) 2026 - zsliu98
// Glass EQ personal UI fork

#include "glass_footer_panel.hpp"
#include "../gui/glass_tokens.hpp"

namespace zlpanel {
    GlassFooterPanel::GlassFooterPanel(PluginProcessor& p, zlgui::UIBase& base) :
        base_(base),
        analyzer_button_(base, "Analyzer"),
        pre_button_(base, "Pre"),
        post_button_(base, "Post"),
        pre_attach_(pre_button_.getButton(), p.parameters_NA_, zlstate::PFFTPreON::kID, updater_),
        post_attach_(post_button_.getButton(), p.parameters_NA_, zlstate::PFFTPostON::kID, updater_),
        speed_box_(zlstate::PFFTSpeed::kChoices, base, ""),
        speed_attach_(speed_box_.getBox(), p.parameters_NA_, zlstate::PFFTSpeed::kID, updater_),
        output_button_(base, "Output"),
        phase_box_(zlp::PFilterStructure::kChoices, base, ""),
        phase_attach_(phase_box_.getBox(), p.parameters_, zlp::PFilterStructure::kID, updater_) {
        setOpaque(false);
        for (auto* button : {&analyzer_button_, &pre_button_, &post_button_, &output_button_}) {
            stylePill(*button);
            button->getLAF().setFontScale(.69f);
            button->getLAF().setJustification(juce::Justification::centred);
        }

        analyzer_button_.getButton().setToggleable(true);
        analyzer_button_.getButton().setClickingTogglesState(false);
        analyzer_button_.getButton().setButtonText("");
        analyzer_button_.setBackgroundPainter([this](juce::Graphics& g, juce::Button& b,
                                                      const bool highlighted, const bool down) {
            const auto selected = b.getToggleState() || down;
            auto local = b.getLocalBounds().toFloat();
            g.setColour(zlgui::glass::textPrimary().withAlpha(selected ? .90f : highlighted ? .72f : .57f));
            g.setFont(juce::FontOptions(base_.getFontSize() * .67f));
            auto label = local.toNearestInt();
            label.removeFromRight(juce::roundToInt(base_.getFontSize() * 2.55f));
            g.drawText("Analyzer", label.reduced(2, 0), juce::Justification::centredLeft, false);

            auto toggle = local.removeFromRight(base_.getFontSize() * 2.22f)
                               .withSizeKeepingCentre(base_.getFontSize() * 2.02f,
                                                      base_.getFontSize() * 1.00f);
            g.setColour(juce::Colour(5, 20, 33).withAlpha(.42f));
            g.fillRoundedRectangle(toggle, toggle.getHeight() * .5f);
            g.setColour(zlgui::glass::rim().withMultipliedAlpha(.72f));
            g.drawRoundedRectangle(toggle, toggle.getHeight() * .5f, .7f);
            if (selected) {
                g.setColour(juce::Colour(105, 165, 255).withAlpha(.58f));
                g.fillRoundedRectangle(toggle, toggle.getHeight() * .5f);
            }
            const auto knob = toggle.getHeight() * .76f;
            const auto knob_x = selected ? toggle.getRight() - knob - toggle.getHeight() * .12f
                                         : toggle.getX() + toggle.getHeight() * .12f;
            juce::ColourGradient lens(juce::Colour(255, 255, 255).withAlpha(.91f), knob_x, toggle.getY(),
                                      juce::Colour(169, 205, 236).withAlpha(.82f), knob_x + knob,
                                      toggle.getBottom(), false);
            g.setGradientFill(lens);
            g.fillEllipse(knob_x, toggle.getCentreY() - knob * .5f, knob, knob);
        });
        output_button_.getButton().setToggleable(true);
        output_button_.getButton().setClickingTogglesState(false);
        pre_button_.getButton().setToggleable(true);
        pre_button_.getButton().setClickingTogglesState(true);
        post_button_.getButton().setToggleable(true);
        post_button_.getButton().setClickingTogglesState(true);

        analyzer_button_.getButton().onClick = [this]() {
            const auto open = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kAnalyzerPanel));
            base_.setPanelProperty(zlgui::PanelSettingIdx::kAnalyzerPanel, open < .5 ? 1. : 0.);
        };
        output_button_.getButton().onClick = [this]() {
            const auto open = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kOutputPanel));
            base_.setPanelProperty(zlgui::PanelSettingIdx::kOutputPanel, open < .5 ? 1. : 0.);
        };

        speed_box_.getLAF().setFontScale(.76f);
        speed_box_.getLAF().setLabelJustification(juce::Justification::centred);
        speed_box_.setBufferedToImage(true);

        phase_box_.getLAF().setFontScale(.76f);
        phase_box_.getLAF().setLabelJustification(juce::Justification::centred);
        phase_box_.setBufferedToImage(true);

        addAndMakeVisible(analyzer_button_);
        addAndMakeVisible(pre_button_);
        addAndMakeVisible(post_button_);
        addAndMakeVisible(speed_box_);
        addAndMakeVisible(output_button_);
        addAndMakeVisible(phase_box_);
        setInterceptsMouseClicks(false, true);
    }

    void GlassFooterPanel::stylePill(zlgui::button::ClickTextButton& button) {
        button.setBackgroundPainter([](juce::Graphics& g, juce::Button& b,
                                      const bool highlighted, const bool down) {
            auto r = b.getLocalBounds().toFloat().reduced(.75f);
            const auto selected = b.getToggleState() || down;
            if (selected || highlighted) {
                zlgui::glass::fillGlassSurface(g, r, r.getHeight() * .5f,
                                               selected ? .11f : .055f,
                                               selected ? .17f : .090f,
                                               selected ? .18f : .095f);
            }
            if (selected) {
                g.setColour(juce::Colour(116, 181, 245).withAlpha(.095f));
                g.fillRoundedRectangle(r.reduced(r.getHeight() * .12f), r.getHeight() * .42f);
            }
        });
    }

    int GlassFooterPanel::getIdealHeight() const {
        const auto font = base_.getFontSize();
        return getButtonSize(font) + 2 * getPaddingSize(font) + juce::roundToInt(font * .12f);
    }

    void GlassFooterPanel::paint(juce::Graphics& g) {
        auto strip = getLocalBounds().toFloat().reduced(.5f);
        zlgui::glass::fillGlassSurface(g, strip, juce::jmax(9.f, strip.getHeight() * .43f),
                                      .060f, .110f, .145f);

        g.setColour(zlgui::glass::rim().withMultipliedAlpha(.36f));
        if (!analyzer_group_bound_.isEmpty())
            g.drawVerticalLine(analyzer_group_bound_.getRight() + getPaddingSize(base_.getFontSize()) / 2,
                               strip.getY() + strip.getHeight() * .29f,
                               strip.getBottom() - strip.getHeight() * .29f);
        if (!processing_group_bound_.isEmpty())
            g.drawVerticalLine(processing_group_bound_.getX() - getPaddingSize(base_.getFontSize()) / 2,
                               strip.getY() + strip.getHeight() * .29f,
                               strip.getBottom() - strip.getHeight() * .29f);

        g.setColour(zlgui::glass::textSecondary().withMultipliedAlpha(.70f));
        g.setFont(juce::FontOptions(base_.getFontSize() * .63f));
        g.drawText("Spectrum", spectrum_label_bound_, juce::Justification::centred, false);
        g.drawText("Processing", processing_label_bound_, juce::Justification::centredRight, false);
    }

    void GlassFooterPanel::resized() {
        const auto font = base_.getFontSize();
        const auto padding = getPaddingSize(font);
        auto b = getLocalBounds().reduced(padding + padding / 2, padding / 2 + 1);

        const auto analyzer_w = juce::jmax(78, juce::roundToInt(font * 5.25f));
        analyzer_button_.setBounds(b.removeFromLeft(analyzer_w));
        b.removeFromLeft(padding / 2);

        const auto small_w = juce::jmax(44, juce::roundToInt(font * 2.85f));
        pre_button_.setBounds(b.removeFromLeft(small_w));
        b.removeFromLeft(padding / 4);
        post_button_.setBounds(b.removeFromLeft(small_w));
        b.removeFromLeft(padding / 2);

        const auto spectrum_w = juce::jmax(70, juce::roundToInt(font * 4.65f));
        spectrum_label_bound_ = b.removeFromLeft(spectrum_w);
        b.removeFromLeft(padding / 4);

        const auto speed_w = juce::jmax(82, juce::roundToInt(font * 5.15f));
        speed_box_.setBounds(b.removeFromLeft(speed_w));

        analyzer_group_bound_ = analyzer_button_.getBounds()
            .getUnion(pre_button_.getBounds())
            .getUnion(post_button_.getBounds())
            .getUnion(spectrum_label_bound_)
            .getUnion(speed_box_.getBounds())
            .expanded(juce::jmax(3, padding / 2), juce::jmax(2, padding / 4));

        const auto tool_w = juce::jmax(72, juce::roundToInt(font * 4.7f));
        auto tools = getLocalBounds().withSizeKeepingCentre(tool_w, getButtonSize(font));
        output_button_.setBounds(tools);
        tools_group_bound_ = output_button_.getBounds()
            .expanded(juce::jmax(2, padding / 3), juce::jmax(2, padding / 4));

        const auto phase_w = juce::jmax(136, juce::roundToInt(font * 8.65f));
        auto phase = b.removeFromRight(phase_w);
        phase_box_.setBounds(phase);
        b.removeFromRight(padding / 3);
        processing_label_bound_ = b.removeFromRight(juce::jmax(68, juce::roundToInt(font * 4.95f)));
        processing_group_bound_ = processing_label_bound_.getUnion(phase_box_.getBounds())
            .expanded(juce::jmax(3, padding / 2), juce::jmax(2, padding / 4));
    }

    void GlassFooterPanel::repaintCallbackSlow() {
        updater_.updateComponents();
        analyzer_button_.getButton().setToggleState(
            static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kAnalyzerPanel)) > .5,
            juce::dontSendNotification);
        output_button_.getButton().setToggleState(
            static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kOutputPanel)) > .5,
            juce::dontSendNotification);
        repaint();
    }
}
