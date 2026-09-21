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
            button->getLAF().setFontScale(.72f);
            button->getLAF().setJustification(juce::Justification::centred);
        }

        analyzer_button_.getButton().setToggleable(true);
        analyzer_button_.getButton().setClickingTogglesState(false);
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

        speed_box_.getLAF().setFontScale(.80f);
        speed_box_.getLAF().setLabelJustification(juce::Justification::centred);
        speed_box_.setBufferedToImage(true);

        phase_box_.getLAF().setFontScale(.80f);
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
            zlgui::glass::fillGlassSurface(g, r, r.getHeight() * .5f,
                                           selected ? .18f : (highlighted ? .105f : .045f),
                                           selected ? .27f : (highlighted ? .16f : .085f),
                                           selected ? .34f : (highlighted ? .20f : .10f));
            if (selected) {
                auto glow = r.reduced(r.getHeight() * .15f);
                juce::ColourGradient active(juce::Colour(124, 190, 255).withAlpha(.28f),
                                            glow.getCentreX(), glow.getCentreY(),
                                            juce::Colours::transparentBlack,
                                            glow.getRight(), glow.getCentreY(), true);
                g.setGradientFill(active);
                g.fillRoundedRectangle(glow, glow.getHeight() * .5f);
            }
        });
    }

    int GlassFooterPanel::getIdealHeight() const {
        const auto font = base_.getFontSize();
        return getButtonSize(font) + 2 * getPaddingSize(font) + juce::roundToInt(font * .18f);
    }

    void GlassFooterPanel::paint(juce::Graphics& g) {
        auto strip = getLocalBounds().toFloat().reduced(.5f);
        zlgui::glass::fillGlassSurface(g, strip, juce::jmax(9.f, strip.getHeight() * .34f),
                                      .105f, .19f, .22f);

        for (const auto group : {analyzer_group_bound_, tools_group_bound_, processing_group_bound_}) {
            if (!group.isEmpty()) {
                auto r = group.toFloat().reduced(.5f);
                zlgui::glass::fillGlassSurface(g, r, r.getHeight() * .5f, .035f, .075f, .085f);
            }
        }

        g.setColour(base_.getTextColour().withAlpha(.52f));
        g.setFont(juce::FontOptions(base_.getFontSize() * .68f));
        g.drawText("Spectrum", spectrum_label_bound_, juce::Justification::centred, false);
        g.drawText("Processing", processing_label_bound_, juce::Justification::centredRight, false);
    }

    void GlassFooterPanel::resized() {
        const auto font = base_.getFontSize();
        const auto padding = getPaddingSize(font);
        auto b = getLocalBounds().reduced(padding + padding / 2, padding / 2 + 1);

        const auto analyzer_w = juce::jmax(82, juce::roundToInt(font * 5.5f));
        analyzer_button_.setBounds(b.removeFromLeft(analyzer_w));
        b.removeFromLeft(padding / 3);

        const auto small_w = juce::jmax(48, juce::roundToInt(font * 3.1f));
        pre_button_.setBounds(b.removeFromLeft(small_w));
        b.removeFromLeft(padding / 4);
        post_button_.setBounds(b.removeFromLeft(small_w));
        b.removeFromLeft(padding / 3);

        const auto spectrum_w = juce::jmax(76, juce::roundToInt(font * 5.0f));
        spectrum_label_bound_ = b.removeFromLeft(spectrum_w);
        b.removeFromLeft(padding / 4);

        const auto speed_w = juce::jmax(88, juce::roundToInt(font * 5.55f));
        speed_box_.setBounds(b.removeFromLeft(speed_w));

        analyzer_group_bound_ = analyzer_button_.getBounds()
            .getUnion(pre_button_.getBounds())
            .getUnion(post_button_.getBounds())
            .getUnion(spectrum_label_bound_)
            .getUnion(speed_box_.getBounds())
            .expanded(juce::jmax(4, padding / 2), juce::jmax(3, padding / 3));

        const auto tool_w = juce::jmax(82, juce::roundToInt(font * 5.25f));
        auto tools = getLocalBounds().withSizeKeepingCentre(tool_w, getButtonSize(font));
        output_button_.setBounds(tools);
        tools_group_bound_ = output_button_.getBounds()
            .expanded(juce::jmax(3, padding / 2), juce::jmax(2, padding / 3));

        const auto phase_w = juce::jmax(144, juce::roundToInt(font * 9.25f));
        auto phase = b.removeFromRight(phase_w);
        phase_box_.setBounds(phase);
        b.removeFromRight(padding / 3);
        processing_label_bound_ = b.removeFromRight(juce::jmax(72, juce::roundToInt(font * 5.35f)));
        processing_group_bound_ = processing_label_bound_.getUnion(phase_box_.getBounds())
            .expanded(juce::jmax(3, padding / 2), juce::jmax(2, padding / 3));
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
