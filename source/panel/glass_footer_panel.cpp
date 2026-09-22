// Copyright (C) 2026 - zsliu98
// Glass EQ personal UI fork

#include "glass_footer_panel.hpp"
#include "../gui/glass_tokens.hpp"

namespace zlpanel {
    GlassFooterPanel::GlassFooterPanel(PluginProcessor& p, zlgui::UIBase& base) :
        p_ref_(p), base_(base),
        analyzer_menu_button_(base, "Analyzer"),
        analyzer_toggle_button_(base, ""),
        pre_button_(base, "Pre"),
        post_button_(base, "Post"),
        side_button_(base, "Side"),
        pre_attach_(pre_button_.getButton(), p.parameters_NA_, zlstate::PFFTPreON::kID, updater_),
        post_attach_(post_button_.getButton(), p.parameters_NA_, zlstate::PFFTPostON::kID, updater_),
        side_attach_(side_button_.getButton(), p.parameters_NA_, zlstate::PFFTSideON::kID, updater_),
        speed_box_(zlstate::PFFTSpeed::kChoices, base, ""),
        speed_attach_(speed_box_.getBox(), p.parameters_NA_, zlstate::PFFTSpeed::kID, updater_),
        phase_box_(zlp::PFilterStructure::kChoices, base, ""),
        phase_attach_(phase_box_.getBox(), p.parameters_, zlp::PFilterStructure::kID, updater_) {
        setOpaque(false);

        for (auto* button : {&analyzer_menu_button_, &pre_button_, &post_button_, &side_button_}) {
            stylePill(*button);
            button->getLAF().setFontScale(.69f);
            button->getLAF().setJustification(juce::Justification::centred);
        }

        analyzer_menu_button_.getButton().setToggleable(true);
        analyzer_menu_button_.getButton().setClickingTogglesState(false);
        analyzer_menu_button_.getButton().onClick = [this]() {
            const auto open = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kAnalyzerPanel));
            base_.setPanelProperty(zlgui::PanelSettingIdx::kAnalyzerPanel, open < .5 ? 1. : 0.);
        };

        analyzer_toggle_button_.getButton().setToggleable(true);
        analyzer_toggle_button_.getButton().setClickingTogglesState(false);
        analyzer_toggle_button_.getButton().setButtonText("");
        analyzer_toggle_button_.setBackgroundPainter([this](juce::Graphics& g, juce::Button& b,
                                                             const bool highlighted, const bool down) {
            const auto active = b.getToggleState();
            auto toggle = b.getLocalBounds().toFloat().reduced(1.f)
                              .withSizeKeepingCentre(base_.getFontSize() * 2.05f,
                                                     base_.getFontSize() * 1.04f);
            if (highlighted || down) {
                const auto halo = toggle.expanded(2.f);
                g.setColour(juce::Colour(173, 215, 248).withAlpha(down ? .12f : .07f));
                g.fillRoundedRectangle(halo, halo.getHeight() * .5f);
            }
            g.setColour(juce::Colour(5, 20, 33).withAlpha(.38f));
            g.fillRoundedRectangle(toggle, toggle.getHeight() * .5f);
            if (active) {
                juce::ColourGradient on(juce::Colour(105, 182, 255).withAlpha(.84f), toggle.getX(), toggle.getY(),
                                        juce::Colour(63, 127, 213).withAlpha(.68f), toggle.getRight(),
                                        toggle.getBottom(), false);
                g.setGradientFill(on);
                g.fillRoundedRectangle(toggle, toggle.getHeight() * .5f);
            }
            g.setColour(juce::Colour(238, 247, 253).withAlpha(active ? .48f : .24f));
            g.drawRoundedRectangle(toggle, toggle.getHeight() * .5f, .72f);

            const auto knob = toggle.getHeight() * .76f;
            const auto knob_x = active ? toggle.getRight() - knob - toggle.getHeight() * .12f
                                       : toggle.getX() + toggle.getHeight() * .12f;
            juce::ColourGradient lens(juce::Colour(255, 255, 255).withAlpha(.98f), knob_x, toggle.getY(),
                                      juce::Colour(175, 209, 236).withAlpha(.91f), knob_x + knob,
                                      toggle.getBottom(), false);
            g.setGradientFill(lens);
            g.fillEllipse(knob_x, toggle.getCentreY() - knob * .5f, knob, knob);
        });
        analyzer_toggle_button_.getButton().onClick = [this]() { toggleAnalyzerEnabled(); };

        for (auto* button : {&pre_button_, &post_button_, &side_button_}) {
            button->getButton().setToggleable(true);
            button->getButton().setClickingTogglesState(true);
        }

        speed_box_.getLAF().setFontScale(.76f);
        speed_box_.getLAF().setLabelJustification(juce::Justification::centred);
        speed_box_.getLAF().setBoxAlpha(.28f);
        speed_box_.setBufferedToImage(true);

        phase_box_.getLAF().setFontScale(.76f);
        phase_box_.getLAF().setLabelJustification(juce::Justification::centred);
        phase_box_.getLAF().setBoxAlpha(.28f);
        phase_box_.setBufferedToImage(true);

        addAndMakeVisible(analyzer_menu_button_);
        addAndMakeVisible(analyzer_toggle_button_);
        addAndMakeVisible(pre_button_);
        addAndMakeVisible(post_button_);
        addAndMakeVisible(side_button_);
        addAndMakeVisible(speed_box_);
        addAndMakeVisible(phase_box_);
        setInterceptsMouseClicks(false, true);

        if (const auto mask = getAnalyzerMask(); mask != 0u) analyzer_restore_mask_ = mask;
    }

    void GlassFooterPanel::stylePill(zlgui::button::ClickTextButton& button) {
        button.setBackgroundPainter([](juce::Graphics& g, juce::Button& b,
                                      const bool highlighted, const bool down) {
            auto r = b.getLocalBounds().toFloat().reduced(.75f);
            const auto selected = b.getToggleState() || down;
            if (selected || highlighted) {
                g.setColour(juce::Colour(236, 247, 253).withAlpha(selected ? .090f : .045f));
                g.fillRoundedRectangle(r, r.getHeight() * .5f);
                g.setColour(juce::Colour(244, 251, 255).withAlpha(selected ? .22f : .11f));
                g.drawRoundedRectangle(r, r.getHeight() * .5f, .76f);
            }
            if (selected) {
                g.setColour(juce::Colour(115, 180, 245).withAlpha(.080f));
                g.fillRoundedRectangle(r.reduced(r.getHeight() * .10f), r.getHeight() * .42f);
            }
        });
    }

    bool GlassFooterPanel::isAnalyzerEnabled() const { return getAnalyzerMask() != 0u; }

    unsigned int GlassFooterPanel::getAnalyzerMask() const {
        unsigned int mask = 0u;
        const auto read = [this](const char* id) {
            if (const auto* value = p_ref_.parameters_NA_.getRawParameterValue(id))
                return value->load(std::memory_order_relaxed) > .5f;
            return false;
        };
        if (read(zlstate::PFFTPreON::kID)) mask |= 0b001u;
        if (read(zlstate::PFFTPostON::kID)) mask |= 0b010u;
        if (read(zlstate::PFFTSideON::kID)) mask |= 0b100u;
        return mask;
    }

    void GlassFooterPanel::setAnalyzerMask(const unsigned int mask) {
        const auto set = [this, mask](const char* id, const unsigned int bit) {
            if (auto* parameter = p_ref_.parameters_NA_.getParameter(id))
                updateValue(parameter, (mask & bit) != 0u ? 1.f : 0.f);
        };
        set(zlstate::PFFTPreON::kID, 0b001u);
        set(zlstate::PFFTPostON::kID, 0b010u);
        set(zlstate::PFFTSideON::kID, 0b100u);
    }

    void GlassFooterPanel::toggleAnalyzerEnabled() {
        const auto mask = getAnalyzerMask();
        if (mask != 0u) {
            analyzer_restore_mask_ = mask;
            setAnalyzerMask(0u);
        } else {
            setAnalyzerMask(analyzer_restore_mask_ != 0u ? analyzer_restore_mask_ : 0b011u);
        }
    }

    int GlassFooterPanel::getIdealHeight() const {
        const auto font = base_.getFontSize();
        return getButtonSize(font) + 2 * getPaddingSize(font) + juce::roundToInt(font * .12f);
    }

    void GlassFooterPanel::paint(juce::Graphics& g) {
        auto strip = getLocalBounds().toFloat().reduced(.5f);
        const auto radius = juce::jmax(9.f, strip.getHeight() * .43f);

        // Direct colour field sampled from the approved reference. The old footer's dark
        // glass fill was hiding the shell transmission and was the main reason the bottom
        // bar looked like a separate navy widget.
        juce::Path clip;
        clip.addRoundedRectangle(strip, radius);
        {
            juce::Graphics::ScopedSaveState state(g);
            g.reduceClipRegion(clip);
            juce::ColourGradient footer(juce::Colour(88, 84, 80), strip.getX(), strip.getCentreY(),
                                        juce::Colour(45, 64, 108), strip.getRight(), strip.getCentreY(), false);
            footer.addColour(.17, juce::Colour(61, 90, 105));
            footer.addColour(.29, juce::Colour(43, 111, 107));
            footer.addColour(.48, juce::Colour(31, 73, 111));
            footer.addColour(.68, juce::Colour(27, 60, 94));
            footer.addColour(.84, juce::Colour(57, 65, 123));
            footer.addColour(.93, juce::Colour(67, 71, 130));
            g.setGradientFill(footer);
            g.fillRect(strip.expanded(1.f));

            juce::ColourGradient sheen(juce::Colour(226, 242, 251).withAlpha(.13f), strip.getCentreX(), strip.getY(),
                                       juce::Colours::transparentBlack, strip.getCentreX(), strip.getBottom(), false);
            sheen.addColour(.34, juce::Colour(183, 219, 237).withAlpha(.045f));
            g.setGradientFill(sheen);
            g.fillRect(strip);
        }
        g.setColour(juce::Colour(239, 248, 253).withAlpha(.18f));
        g.drawRoundedRectangle(strip, radius, .78f);

        g.setColour(juce::Colour(233, 245, 252).withAlpha(.075f));
        if (!analyzer_group_bound_.isEmpty())
            g.drawVerticalLine(analyzer_group_bound_.getRight() + getPaddingSize(base_.getFontSize()) / 2,
                               strip.getY() + strip.getHeight() * .29f,
                               strip.getBottom() - strip.getHeight() * .29f);
        if (!processing_group_bound_.isEmpty())
            g.drawVerticalLine(processing_group_bound_.getX() - getPaddingSize(base_.getFontSize()) / 2,
                               strip.getY() + strip.getHeight() * .29f,
                               strip.getBottom() - strip.getHeight() * .29f);

        g.setColour(juce::Colour(236, 244, 249).withAlpha(.52f));
        g.setFont(juce::FontOptions(base_.getFontSize() * .63f));
        g.drawText("Spectrum", spectrum_label_bound_, juce::Justification::centred, false);
        g.drawText("Processing", processing_label_bound_, juce::Justification::centredRight, false);
    }

    void GlassFooterPanel::resized() {
        const auto font = base_.getFontSize();
        const auto padding = getPaddingSize(font);
        auto b = getLocalBounds().reduced(padding + padding / 2, padding / 2 + 1);

        const auto analyzer_label_w = juce::jmax(66, juce::roundToInt(font * 4.25f));
        analyzer_menu_button_.setBounds(b.removeFromLeft(analyzer_label_w));
        const auto switch_w = juce::jmax(39, juce::roundToInt(font * 2.45f));
        analyzer_toggle_button_.setBounds(b.removeFromLeft(switch_w));
        b.removeFromLeft(padding / 2);

        const auto small_w = juce::jmax(40, juce::roundToInt(font * 2.62f));
        pre_button_.setBounds(b.removeFromLeft(small_w));
        b.removeFromLeft(padding / 5);
        post_button_.setBounds(b.removeFromLeft(small_w));
        b.removeFromLeft(padding / 5);
        side_button_.setBounds(b.removeFromLeft(small_w));
        b.removeFromLeft(padding / 2);

        const auto spectrum_w = juce::jmax(62, juce::roundToInt(font * 4.10f));
        spectrum_label_bound_ = b.removeFromLeft(spectrum_w);
        b.removeFromLeft(padding / 5);

        const auto speed_w = juce::jmax(78, juce::roundToInt(font * 5.00f));
        speed_box_.setBounds(b.removeFromLeft(speed_w));

        analyzer_group_bound_ = analyzer_menu_button_.getBounds()
            .getUnion(analyzer_toggle_button_.getBounds())
            .getUnion(pre_button_.getBounds())
            .getUnion(post_button_.getBounds())
            .getUnion(side_button_.getBounds())
            .getUnion(spectrum_label_bound_)
            .getUnion(speed_box_.getBounds())
            .expanded(juce::jmax(3, padding / 2), juce::jmax(2, padding / 4));

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
        const auto mask = getAnalyzerMask();
        if (mask != 0u) analyzer_restore_mask_ = mask;
        analyzer_toggle_button_.getButton().setToggleState(mask != 0u, juce::dontSendNotification);
        analyzer_menu_button_.getButton().setToggleState(
            static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kAnalyzerPanel)) > .5,
            juce::dontSendNotification);
        repaint();
    }
}
