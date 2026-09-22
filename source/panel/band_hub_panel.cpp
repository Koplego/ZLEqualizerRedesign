// Copyright (C) 2026 - zsliu98
// Glass EQ contextual band workspace

#include "band_hub_panel.hpp"
#include "../gui/glass_tokens.hpp"
#include <array>
#include <cstdio>

namespace zlpanel {
    BandHubPanel::BandHubPanel(PluginProcessor& p, zlgui::UIBase& base,
                               const multilingual::TooltipHelper& tooltip_helper) :
        p_ref_(p), base_(base),
        collapse_button_(base, "⌄"),
        dynamic_button_(base, "Dynamic"),
        dynamics_page_button_(base, "Dynamics"),
        detector_page_button_(base, "Detector"),
        sidechain_page_button_(base, "Sidechain"),
        channel_box_(juce::StringArray{"Stereo", "Left", "Right", "Mid", "Side"}, base, ""),
        threshold_slider_("", base), range_slider_("", base), attack_slider_("", base), release_slider_("", base),
        knee_slider_("", base), rms_length_slider_("", base), rms_mix_slider_("", base), smooth_slider_("", base),
        learn_button_(base, "Learn"), relative_button_(base, "Relative"), dyn_bypass_button_(base, "Bypass"),
        side_type_box_(zlp::PSideFilterType::kChoices, base, ""),
        side_order_box_(zlp::PSideOrder::kChoices, base, ""),
        side_freq_slider_("", base), side_q_slider_("", base),
        side_link_button_(base, "Link"), side_swap_button_(base, "Swap") {
        juce::ignoreUnused(tooltip_helper);
        setOpaque(false);
        setInterceptsMouseClicks(true, true);

        for (auto* button : {&collapse_button_, &dynamic_button_, &dynamics_page_button_, &detector_page_button_,
                             &sidechain_page_button_, &learn_button_, &relative_button_, &dyn_bypass_button_,
                             &side_link_button_, &side_swap_button_}) {
            stylePill(*button, .60f);
            addAndMakeVisible(*button);
        }

        collapse_button_.getLAF().setFontScale(.72f);
        collapse_button_.getButton().onClick = [this]() { setExpanded(false); };

        dynamic_button_.getButton().setToggleable(true);
        dynamic_button_.getButton().setClickingTogglesState(false);
        dynamic_button_.getButton().onClick = [this]() {
            if (const auto band = base_.getSelectedBand(); band < zlp::kBandNum) {
                const auto max_idx = std::round(p_ref_.parameters_NA_.getRawParameterValue(
                    zlstate::PEQMaxDB::kID)->load(std::memory_order_relaxed));
                band_helper::turnOnOffDynamic(p_ref_, band, !dynamic_on_,
                                              base_.getCurveDBScale(static_cast<size_t>(max_idx)));
                dynamic_on_ = !dynamic_on_;
                dynamic_button_.getButton().setToggleState(dynamic_on_, juce::dontSendNotification);
                updateVisibility();
                repaint();
            }
        };

        for (auto* button : {&dynamics_page_button_, &detector_page_button_, &sidechain_page_button_}) {
            button->getButton().setToggleable(true);
            button->getButton().setClickingTogglesState(false);
        }
        dynamics_page_button_.getButton().onClick = [this]() { setPage(Page::dynamics); };
        detector_page_button_.getButton().onClick = [this]() { setPage(Page::detector); };
        sidechain_page_button_.getButton().onClick = [this]() { setPage(Page::sidechain); };

        for (auto* button : {&learn_button_, &relative_button_, &dyn_bypass_button_, &side_link_button_, &side_swap_button_}) {
            button->getButton().setToggleable(true);
            button->getButton().setClickingTogglesState(true);
        }

        channel_box_.getLAF().setFontScale(.62f);
        channel_box_.getLAF().setBoxAlpha(.20f);
        channel_box_.getLAF().setLabelJustification(juce::Justification::centred);
        channel_box_.setScrollEnabled(true);
        addAndMakeVisible(channel_box_);

        for (auto* box : {&side_type_box_, &side_order_box_}) {
            box->getLAF().setFontScale(.60f);
            box->getLAF().setBoxAlpha(.21f);
            box->getLAF().setLabelJustification(juce::Justification::centred);
            addAndMakeVisible(*box);
        }

        auto setupSlider = [this](auto& slider, const float scale = .68f) {
            slider.setFontScale(scale);
            slider.getSlider().setSliderSnapsToMousePosition(false);
            slider.setBufferedToImage(true);
            addAndMakeVisible(slider);
        };

        setupSlider(threshold_slider_);
        threshold_slider_.setPrecision(3);
        threshold_slider_.value_formatter_ = [](const double v) -> std::string {
            char b[32]; snprintf(b, sizeof(b), "%.1f dB", v); return b;
        };

        setupSlider(range_slider_);
        range_slider_.setPrecision(3);
        range_slider_.value_formatter_ = [this](const double target) -> std::string {
            double gain = 0.0;
            if (const auto band = base_.getSelectedBand(); band < zlp::kBandNum) {
                if (const auto* value = p_ref_.parameters_.getRawParameterValue(zlp::PGain::kID + std::to_string(band)))
                    gain = value->load(std::memory_order_relaxed);
            }
            char b[32]; snprintf(b, sizeof(b), "%+.2f dB", target - gain); return b;
        };
        range_slider_.string_formatter_ = [this](const std::string& text) -> std::optional<double> {
            double gain = 0.0;
            if (const auto band = base_.getSelectedBand(); band < zlp::kBandNum) {
                if (const auto* value = p_ref_.parameters_.getRawParameterValue(zlp::PGain::kID + std::to_string(band)))
                    gain = value->load(std::memory_order_relaxed);
            }
            return gain + juce::String(text).getDoubleValue();
        };

        setupSlider(attack_slider_);
        attack_slider_.setPrecision(4);
        attack_slider_.value_formatter_ = [](const double v) -> std::string {
            char b[32]; snprintf(b, sizeof(b), v < 100.0 ? "%.1f ms" : "%.0f ms", v); return b;
        };

        setupSlider(release_slider_);
        release_slider_.setPrecision(4);
        release_slider_.value_formatter_ = [](const double v) -> std::string {
            char b[32];
            if (v >= 1000.0) snprintf(b, sizeof(b), "%.2f s", v * .001);
            else snprintf(b, sizeof(b), "%.0f ms", v);
            return b;
        };

        setupSlider(knee_slider_);
        knee_slider_.value_formatter_ = [](const double v) -> std::string {
            char b[32]; snprintf(b, sizeof(b), "%.1f dB", v); return b;
        };
        setupSlider(rms_length_slider_);
        rms_length_slider_.value_formatter_ = [](const double v) -> std::string {
            char b[32]; snprintf(b, sizeof(b), "%.1f ms", v); return b;
        };
        setupSlider(rms_mix_slider_);
        rms_mix_slider_.value_formatter_ = [](const double v) -> std::string {
            char b[32]; snprintf(b, sizeof(b), "%.0f%%", v); return b;
        };
        setupSlider(smooth_slider_);
        smooth_slider_.value_formatter_ = [](const double v) -> std::string {
            char b[32]; snprintf(b, sizeof(b), "%.0f%%", v); return b;
        };
        setupSlider(side_freq_slider_);
        side_freq_slider_.setPrecision(4);
        side_freq_slider_.value_formatter_ = [](const double v) -> std::string {
            char b[32];
            if (v >= 1000.0) snprintf(b, sizeof(b), "%.2f kHz", v * .001);
            else snprintf(b, sizeof(b), v >= 100.0 ? "%.0f Hz" : "%.1f Hz", v); return b;
        };
        setupSlider(side_q_slider_);
        side_q_slider_.value_formatter_ = [](const double v) -> std::string {
            char b[32]; snprintf(b, sizeof(b), "Q %.2f", v); return b;
        };

        updateVisibility();
        setVisible(false);
    }

    void BandHubPanel::stylePill(zlgui::button::ClickTextButton& button, const float font_scale) {
        button.getLAF().setFontScale(font_scale);
        button.getLAF().setJustification(juce::Justification::centred);
        button.setBackgroundPainter([](juce::Graphics& g, juce::Button& b, bool highlighted, bool down) {
            auto r = b.getLocalBounds().toFloat().reduced(.6f);
            const auto active = b.getToggleState() || down;
            if (active || highlighted) {
                zlgui::glass::fillGlassSurface(g, r, r.getHeight() * .5f,
                                               active ? .095f : .038f,
                                               active ? .14f : .065f,
                                               active ? .15f : .07f);
            }
        });
    }

    juce::Rectangle<float> BandHubPanel::expandedSurface() const {
        return getLocalBounds().toFloat().reduced(.5f);
    }

    juce::Rectangle<float> BandHubPanel::collapsedSurface() const {
        const auto full = getLocalBounds().toFloat();
        const auto h = juce::jmax(27.f, base_.getFontSize() * 1.75f);
        const auto w = juce::jmin(full.getWidth() * .42f, juce::jmax(176.f, base_.getFontSize() * 11.5f));
        return {full.getCentreX() - w * .5f, full.getBottom() - h - .5f, w, h};
    }

    juce::Rectangle<float> BandHubPanel::currentSurface() const {
        const auto a = collapsedSurface();
        const auto b = expandedSurface();
        const auto lerp = [this](const float x, const float y) { return x + (y - x) * reveal_; };
        return {lerp(a.getX(), b.getX()), lerp(a.getY(), b.getY()),
                lerp(a.getWidth(), b.getWidth()), lerp(a.getHeight(), b.getHeight())};
    }

    void BandHubPanel::paint(juce::Graphics& g) {
        if (suppressed_ || attached_band_ >= zlp::kBandNum) return;

        auto surface = currentSurface();
        const auto radius = reveal_ < .30f
            ? surface.getHeight() * .50f
            : juce::jmax(10.f, base_.getFontSize() * .76f);
        zlgui::glass::fillGlassSurface(g, surface, radius, .072f, .12f, .14f);

        // True spatial transmission replaces the old decorative accent gradient. Because
        // these source coordinates have already been converted into Hub-local space, the
        // selected node illuminates whichever edge of the glass is physically nearest it.
        zlgui::glass::paintAmbientSources(g, ambient_sources_, surface, .030f);

        static constexpr std::array<const char*, 11> names{
            "Bell", "Low Shelf", "High Cut", "High Shelf", "Low Cut", "Notch",
            "Band Pass", "Tilt", "Flat Tilt", "All Pass", "Gain"
        };
        auto* typeValue = p_ref_.parameters_.getRawParameterValue(zlp::PFilterType::kID + std::to_string(attached_band_));
        const auto type = typeValue ? static_cast<int>(std::round(typeValue->load(std::memory_order_relaxed))) : 0;

        if (reveal_ < .38f) {
            juce::String title = "Band " + juce::String(static_cast<int>(attached_band_ + 1));
            if (type >= 0 && type < static_cast<int>(names.size())) title += "  ·  " + juce::String(names[static_cast<size_t>(type)]);
            if (dynamic_on_) title += "  ·  Dynamic";
            g.setColour(zlgui::glass::textPrimary().withAlpha(.90f));
            g.setFont(juce::FontOptions(base_.getFontSize() * .68f));
            g.drawFittedText(title, surface.toNearestInt().reduced(12, 0), juce::Justification::centred, 1);
            return;
        }

        const auto childAlpha = juce::jlimit(0.f, 1.f, (reveal_ - .24f) / .76f);
        juce::String bandTitle = "B" + juce::String(static_cast<int>(attached_band_ + 1));
        if (type >= 0 && type < static_cast<int>(names.size())) bandTitle += " · " + juce::String(names[static_cast<size_t>(type)]);
        g.setColour(zlgui::glass::textPrimary().withAlpha(.92f * childAlpha));
        g.setFont(juce::FontOptions(base_.getFontSize() * .67f));
        g.drawFittedText(bandTitle, title_bound_, juce::Justification::centredLeft, 1);

        static constexpr std::array<const char*, 4> dynamics{"THRESHOLD", "RANGE", "ATTACK", "RELEASE"};
        static constexpr std::array<const char*, 4> detector{"KNEE", "RMS LENGTH", "RMS MIX", "SMOOTH"};
        static constexpr std::array<const char*, 4> side{"FILTER", "SLOPE", "FREQUENCY", "Q"};
        const auto& labels = page_ == Page::dynamics ? dynamics : page_ == Page::detector ? detector : side;

        if (dynamic_on_) {
            g.setColour(zlgui::glass::textSecondary().withAlpha(.67f * childAlpha));
            g.setFont(juce::FontOptions(base_.getFontSize() * .47f));
            const auto& bounds = page_ == Page::dynamics ? primary_label_bounds_ : detail_label_bounds_;
            for (size_t i = 0; i < labels.size(); ++i)
                g.drawText(labels[i], bounds[i], juce::Justification::centredBottom, false);
        } else {
            auto message = currentSurface().toNearestInt().reduced(16, 8);
            message.removeFromTop(juce::jmax(22, juce::roundToInt(base_.getFontSize() * 1.65f)));
            g.setColour(zlgui::glass::textSecondary().withAlpha(.72f * childAlpha));
            g.setFont(juce::FontOptions(base_.getFontSize() * .58f));
            g.drawFittedText("Dynamic EQ is off — enable it to edit dynamics, detector and sidechain.",
                             message, juce::Justification::centred, 1);
        }
    }
}
