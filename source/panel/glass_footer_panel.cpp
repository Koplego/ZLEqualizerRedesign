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

        // The switch is its own hit target. This is intentionally not the menu button:
        // it means exactly one thing — whether the spectrum analyzer is active.
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
                g.setColour(juce::Colour(173, 215, 248).withAlpha(down ? .10f : .055f));
                g.fillRoundedRectangle(halo, halo.getHeight() * .5f);
            }
            g.setColour(juce::Colour(5, 20, 33).withAlpha(.44f));
            g.fillRoundedRectangle(toggle, toggle.getHeight() * .5f);
            if (active) {
                juce::ColourGradient on(juce::Colour(123, 190, 255).withAlpha(.66f), toggle.getX(), toggle.getY(),
                                        juce::Colour(73, 134, 218).withAlpha(.48f), toggle.getRight(),
                                        toggle.getBottom(), false);
                g.setGradientFill(on);
                g.fillRoundedRectangle(toggle, toggle.getHeight() * .5f);
            }
            g.setColour(zlgui::glass::rim().withMultipliedAlpha(active ? .84f : .62f));
            g.drawRoundedRectangle(toggle, toggle.getHeight() * .5f, .72f);

            const auto knob = toggle.getHeight() * .76f;
            const auto knob_x = active ? toggle.getRight() - knob - toggle.getHeight() * .12f
                                       : toggle.getX() + toggle.getHeight() * .12f;
            juce::ColourGradient lens(juce::Colour(255, 255, 255).withAlpha(.94f), knob_x, toggle.getY(),
                                      juce::Colour(169, 205, 236).withAlpha(.84f), knob_x + knob,
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

    bool GlassFooterPanel::isAnalyzerEnabled() const {
        return getAnalyzerMask() != 0u;
    }

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

        // The agreed lighting reference has a brighter translucent shelf than the runtime
        // build: it is still dark glass, but it has enough optical density to visibly carry
        // amber, cyan, blue and violet from the nodes above it.
        zlgui::glass::fillGlassSurface(g, strip, juce::jmax(9.f, strip.getHeight() * .43f),
                                      .085f, .148f, .165f);

        // Inject the node field after the neutral material so the footer reads like one piece
        // of glass sitting in the same ambient room as the graph, not separate blue chrome.
        zlgui::glass::paintAmbientSources(g, ambient_sources_, strip, .024f);

        g.setColour(zlgui::glass::rim().withMultipliedAlpha(.40f));
        if (!analyzer_group_bound_.isEmpty())
            g.drawVerticalLine(analyzer_group_bound_.getRight() + getPaddingSize(base_.getFontSize()) / 2,
                               strip.getY() + strip.getHeight() * .29f,
                               strip.getBottom() - strip.getHeight() * .29f);
        if (!processing_group_bound_.isEmpty())
            g.drawVerticalLine(processing_group_bound_.getX() - getPaddingSize(base_.getFontSize()) / 2,
                               strip.getY() + strip.getHeight() * .29f,
                               strip.getBottom() - strip.getHeight() * .29f);

        g.setColour(zlgui::glass::textSecondary().withMultipliedAlpha(.74f));
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
