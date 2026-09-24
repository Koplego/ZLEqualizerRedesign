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
            const auto signalBlue = juce::Colour(73, 157, 255);
            auto toggle = b.getLocalBounds().toFloat().reduced(1.f)
                              .withSizeKeepingCentre(base_.getFontSize() * 2.05f,
                                                     base_.getFontSize() * 1.04f);
            juce::Path track;
            track.addRoundedRectangle(toggle, toggle.getHeight() * .5f);
            if (active) {
                juce::DropShadow(signalBlue.withAlpha(.52f),
                    juce::jmax(5, juce::roundToInt(base_.getFontSize() * .65f)), {0, 0})
                    .drawForPath(g, track);
            }
            zlgui::glass::fillGlassSurface(g, toggle, toggle.getHeight() * .5f,
                .08f, .12f, active ? .31f : .16f);
            if (active) {
                juce::ColourGradient lit(signalBlue.withAlpha(.60f), toggle.getX(), toggle.getCentreY(),
                                         signalBlue.withAlpha(.33f), toggle.getRight(), toggle.getCentreY(), false);
                g.setGradientFill(lit);
                g.fillRoundedRectangle(toggle.reduced(1.f), toggle.getHeight() * .5f - 1.f);
            } else if (highlighted || down) {
                g.setColour(juce::Colours::white.withAlpha(.09f));
                g.fillRoundedRectangle(toggle.reduced(1.f), toggle.getHeight() * .5f - 1.f);
            }

            const auto knob = toggle.getHeight() * .76f;
            const auto knob_x = active ? toggle.getRight() - knob - toggle.getHeight() * .12f
                                       : toggle.getX() + toggle.getHeight() * .12f;
            juce::ColourGradient lens(juce::Colour(255, 255, 255).withAlpha(.91f), knob_x, toggle.getY(),
                                      juce::Colour(204, 223, 239).withAlpha(.70f), knob_x + knob,
                                      toggle.getBottom(), false);
            g.setGradientFill(lens);
            g.fillEllipse(knob_x, toggle.getCentreY() - knob * .5f, knob, knob);
            g.setColour(juce::Colours::white.withAlpha(.72f));
            g.drawEllipse(knob_x + .5f, toggle.getCentreY() - knob * .5f + .5f,
                          knob - 1.f, knob - 1.f, .75f);
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
        button.setBackgroundPainter([this](juce::Graphics& g, juce::Button& b,
                                      const bool highlighted, const bool down) {
            auto r = b.getLocalBounds().toFloat().reduced(.75f);
            const auto analyzer = b.getButtonText() == "Analyzer";
            const auto lit = analyzer && isAnalyzerEnabled();
            const auto selected = (analyzer ? lit : b.getToggleState()) || down;
            if (lit) {
                juce::Path silhouette;
                silhouette.addRoundedRectangle(r, r.getHeight() * .5f);
                juce::DropShadow(juce::Colour(73, 157, 255).withAlpha(.24f),
                    juce::jmax(5, juce::roundToInt(base_.getFontSize() * .55f)), {0, 0})
                    .drawForPath(g, silhouette);
            }
            zlgui::glass::fillGlassSurface(g, r, r.getHeight() * .5f,
                selected ? .095f : .030f, selected ? .12f : .045f,
                selected ? .23f : highlighted ? .16f : .10f);
            if (selected) {
                const auto tint = lit ? juce::Colour(73, 157, 255) : juce::Colours::white;
                juce::ColourGradient core(tint.withAlpha(lit ? .25f : .065f),
                    r.getCentreX(), r.getY(), tint.withAlpha(0.f),
                    r.getCentreX(), r.getBottom(), false);
                g.setGradientFill(core);
                g.fillRoundedRectangle(r.reduced(2.f), r.getHeight() * .5f - 2.f);
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

        // This is a transparent neutral glass strip, not a pre-coloured gradient. The light
        // field behind it is generated by the live EQ nodes in MainPanel, so a yellow node
        // can warm the left side and a violet node can tint the right only when those nodes
        // actually exist in those positions.
        zlgui::glass::fillGlassSurface(g, strip, radius, .038f, .078f, .16f);

        juce::ColourGradient sheen(juce::Colour(242, 249, 253).withAlpha(.075f),
                                   strip.getCentreX(), strip.getY(),
                                   juce::Colours::transparentWhite,
                                   strip.getCentreX(), strip.getY() + strip.getHeight() * .62f, false);
        sheen.addColour(.28, juce::Colour(221, 237, 246).withAlpha(.028f));
        g.setGradientFill(sheen);
        g.fillRoundedRectangle(strip.reduced(1.f), juce::jmax(1.f, radius - 1.f));

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
            mask != 0u,
            juce::dontSendNotification);
        repaint();
    }
}
