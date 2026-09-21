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
        phase_box_(zlp::PFilterStructure::kChoices, base, ""),
        phase_attach_(phase_box_.getBox(), p.parameters_, zlp::PFilterStructure::kID, updater_) {
        setOpaque(false);
        stylePill(analyzer_button_);
        stylePill(pre_button_);
        stylePill(post_button_);

        analyzer_button_.getLAF().setFontScale(.76f);
        pre_button_.getLAF().setFontScale(.76f);
        post_button_.getLAF().setFontScale(.76f);

        pre_button_.getButton().setToggleable(true);
        pre_button_.getButton().setClickingTogglesState(true);
        post_button_.getButton().setToggleable(true);
        post_button_.getButton().setClickingTogglesState(true);

        analyzer_button_.getButton().onClick = [this]() {
            const auto open = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kAnalyzerPanel));
            base_.setPanelProperty(zlgui::PanelSettingIdx::kAnalyzerPanel, open < .5 ? 1. : 0.);
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
        addAndMakeVisible(phase_box_);
        setInterceptsMouseClicks(false, true);
    }

    void GlassFooterPanel::stylePill(zlgui::button::ClickTextButton& button) {
        button.setBackgroundPainter([this](juce::Graphics& g, juce::Button& b,
                                           const bool highlighted, const bool down) {
            auto r = b.getLocalBounds().toFloat().reduced(.75f);
            const auto selected = b.getToggleState() || down;
            juce::ColourGradient fill(selected
                                          ? juce::Colour(124, 186, 255).withAlpha(.30f)
                                          : juce::Colour(246, 252, 255).withAlpha(highlighted ? .105f : .045f),
                                      r.getCentreX(), r.getY(),
                                      juce::Colour(31, 54, 72).withAlpha(selected ? .22f : .10f),
                                      r.getCentreX(), r.getBottom(), false);
            g.setGradientFill(fill);
            g.fillRoundedRectangle(r, r.getHeight() * .5f);
            g.setColour(juce::Colour(247, 252, 255).withAlpha(selected ? .25f : .10f));
            g.drawRoundedRectangle(r, r.getHeight() * .5f, .8f);
        });
    }

    int GlassFooterPanel::getIdealHeight() const {
        const auto font = base_.getFontSize();
        return getButtonSize(font) + 2 * getPaddingSize(font) + juce::roundToInt(font * .18f);
    }

    void GlassFooterPanel::paint(juce::Graphics& g) {
        // The footer is not a bar. Only the control clusters are glass, leaving the graph
        // visually continuous all the way to the lower edge.
        const auto paint_group = [&g](const juce::Rectangle<int>& ib, const juce::Colour tint) {
            if (ib.isEmpty()) return;
            auto group = ib.toFloat().reduced(.5f);
            juce::ignoreUnused(tint);
            zlgui::glass::fillGlassSurface(g, group, group.getHeight() * .5f, .07f, .12f, .11f);
        };
        paint_group(analyzer_group_bound_, juce::Colour(57, 93, 122));
        paint_group(processing_group_bound_, juce::Colour(62, 79, 105));

        g.setColour(base_.getTextColour().withAlpha(.38f));
        g.setFont(juce::FontOptions(base_.getFontSize() * .62f));
        g.drawText("Phase", processing_label_bound_, juce::Justification::centredRight, false);
    }

    void GlassFooterPanel::resized() {
        const auto font = base_.getFontSize();
        const auto padding = getPaddingSize(font);
        auto b = getLocalBounds().reduced(padding + padding / 2, padding);

        const auto analyzer_w = juce::jmax(74, juce::roundToInt(font * 5.15f));
        analyzer_button_.setBounds(b.removeFromLeft(analyzer_w));
        b.removeFromLeft(padding / 3);

        const auto small_w = juce::jmax(46, juce::roundToInt(font * 3.0f));
        pre_button_.setBounds(b.removeFromLeft(small_w));
        b.removeFromLeft(padding / 4);
        post_button_.setBounds(b.removeFromLeft(small_w));
        b.removeFromLeft(padding / 3);

        const auto speed_w = juce::jmax(82, juce::roundToInt(font * 5.25f));
        speed_box_.setBounds(b.removeFromLeft(speed_w));

        analyzer_group_bound_ = analyzer_button_.getBounds()
            .getUnion(pre_button_.getBounds())
            .getUnion(post_button_.getBounds())
            .getUnion(speed_box_.getBounds())
            .expanded(juce::jmax(3, padding / 2), juce::jmax(2, padding / 3));

        const auto phase_w = juce::jmax(138, juce::roundToInt(font * 9.0f));
        auto phase = b.removeFromRight(phase_w);
        phase_box_.setBounds(phase);
        b.removeFromRight(padding / 3);
        processing_label_bound_ = b.removeFromRight(juce::jmax(54, juce::roundToInt(font * 3.9f)));
        processing_group_bound_ = processing_label_bound_.getUnion(phase_box_.getBounds())
            .expanded(juce::jmax(3, padding / 2), juce::jmax(2, padding / 3));
    }

    void GlassFooterPanel::repaintCallbackSlow() {
        updater_.updateComponents();
        repaint();
    }
}
