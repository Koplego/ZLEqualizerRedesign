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
        collapse_button_(base, "—"),
        dynamic_button_(base, "Dynamic EQ"),
        dynamics_page_button_(base, "Dynamics"),
        detector_page_button_(base, "Detector"),
        sidechain_page_button_(base, "Sidechain"),
        channel_box_(juce::StringArray{"Stereo", "Left", "Right", "Mid", "Side"}, base, ""),
        threshold_slider_("", base), range_slider_("", base), attack_slider_("", base), release_slider_("", base),
        knee_slider_("", base), rms_length_slider_("", base), rms_mix_slider_("", base), smooth_slider_("", base),
        learn_button_(base, "Learn"), relative_button_(base, "Relative"), dyn_bypass_button_(base, "Dyn Bypass"),
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
            stylePill(*button);
            addAndMakeVisible(*button);
        }

        collapse_button_.getLAF().setFontScale(.78f);
        collapse_button_.getButton().onClick = [this]() { setExpanded(false); };

        dynamic_button_.getButton().setToggleable(true);
        dynamic_button_.getButton().setClickingTogglesState(false);
        dynamic_button_.getButton().onClick = [this]() {
            if (const auto band = base_.getSelectedBand(); band < zlp::kBandNum) {
                const auto max_idx = std::round(p_ref_.parameters_NA_.getRawParameterValue(
                    zlstate::PEQMaxDB::kID)->load(std::memory_order::relaxed));
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

        channel_box_.getLAF().setFontScale(.72f);
        channel_box_.getLAF().setBoxAlpha(.26f);
        channel_box_.getLAF().setLabelJustification(juce::Justification::centred);
        channel_box_.setScrollEnabled(true);
        addAndMakeVisible(channel_box_);

        side_type_box_.getLAF().setFontScale(.70f);
        side_type_box_.getLAF().setBoxAlpha(.28f);
        side_type_box_.getLAF().setLabelJustification(juce::Justification::centred);
        addAndMakeVisible(side_type_box_);
        side_order_box_.getLAF().setFontScale(.70f);
        side_order_box_.getLAF().setBoxAlpha(.28f);
        side_order_box_.getLAF().setLabelJustification(juce::Justification::centred);
        addAndMakeVisible(side_order_box_);

        auto setupSlider = [this](auto& slider, float scale = .82f) {
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
                    gain = value->load(std::memory_order::relaxed);
            }
            char b[32]; snprintf(b, sizeof(b), "%+.2f dB", target - gain); return b;
        };
        range_slider_.string_formatter_ = [this](const std::string& text) -> std::optional<double> {
            double gain = 0.0;
            if (const auto band = base_.getSelectedBand(); band < zlp::kBandNum) {
                if (const auto* value = p_ref_.parameters_.getRawParameterValue(zlp::PGain::kID + std::to_string(band)))
                    gain = value->load(std::memory_order::relaxed);
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
            else snprintf(b, sizeof(b), v >= 100.0 ? "%.0f Hz" : "%.1f Hz", v);
            return b;
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
                                               active ? .10f : .045f,
                                               active ? .15f : .075f,
                                               active ? .16f : .08f);
            }
        });
    }

    juce::Rectangle<float> BandHubPanel::expandedSurface() const {
        return getLocalBounds().toFloat().reduced(.5f);
    }

    juce::Rectangle<float> BandHubPanel::collapsedSurface() const {
        const auto full = getLocalBounds().toFloat();
        const auto h = juce::jmax(34.f, base_.getFontSize() * 2.15f);
        const auto w = juce::jmin(full.getWidth() * .44f, juce::jmax(210.f, base_.getFontSize() * 15.5f));
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
        const auto radius = juce::jmax(11.f, surface.getHeight() * (reveal_ < .35f ? .48f : .10f));
        zlgui::glass::fillGlassSurface(g, surface, radius, .085f, .145f, .17f);

        const auto accent = base_.getColourMap1(attached_band_);
        auto rim = surface.reduced(base_.getFontSize() * .72f, 0.f);
        rim.setHeight(1.1f);
        juce::ColourGradient glow(accent.withAlpha(.0f), rim.getX(), rim.getY(),
                                  accent.interpolatedWith(juce::Colours::white, .35f).withAlpha(.48f),
                                  rim.getCentreX(), rim.getY(), false);
        glow.addColour(.86, accent.withAlpha(.10f));
        g.setGradientFill(glow);
        g.fillRoundedRectangle(rim, .6f);

        if (reveal_ < .42f) {
            static constexpr std::array<const char*, 11> names{
                "Bell", "Low Shelf", "High Cut", "High Shelf", "Low Cut", "Notch",
                "Band Pass", "Tilt", "Flat Tilt", "All Pass", "Gain"
            };
            auto* typeValue = p_ref_.parameters_.getRawParameterValue(zlp::PFilterType::kID + std::to_string(attached_band_));
            const auto type = typeValue ? static_cast<int>(std::round(typeValue->load(std::memory_order::relaxed))) : 0;
            juce::String title = "Band " + juce::String(static_cast<int>(attached_band_ + 1));
            if (type >= 0 && type < static_cast<int>(names.size())) title += "  ·  " + juce::String(names[static_cast<size_t>(type)]);
            if (dynamic_on_) title += "  ·  Dynamic";
            g.setColour(zlgui::glass::textPrimary().withAlpha(.90f));
            g.setFont(juce::FontOptions(base_.getFontSize() * .74f));
            g.drawText(title, surface.toNearestInt().reduced(18, 0), juce::Justification::centred, false);
            return;
        }

        const auto childAlpha = juce::jlimit(0.f, 1.f, (reveal_ - .28f) / .72f);
        g.setColour(zlgui::glass::textPrimary().withAlpha(.94f * childAlpha));
        g.setFont(juce::FontOptions(base_.getFontSize() * .88f));
        g.drawText("Band " + juce::String(static_cast<int>(attached_band_ + 1)), title_bound_,
                   juce::Justification::centredLeft, false);

        g.setColour(accent.interpolatedWith(juce::Colours::white, .24f).withAlpha(.90f * childAlpha));
        g.setFont(juce::FontOptions(base_.getFontSize() * .68f));
        g.drawText(page_ == Page::dynamics ? "Dynamic EQ" : page_ == Page::detector ? "Detector" : "Sidechain",
                   page_title_bound_, juce::Justification::centredLeft, false);

        static constexpr std::array<const char*, 4> dynamics{"THRESHOLD", "RANGE", "ATTACK", "RELEASE"};
        static constexpr std::array<const char*, 4> detector{"KNEE", "RMS LENGTH", "RMS MIX", "SMOOTH"};
        static constexpr std::array<const char*, 4> side{"FILTER", "SLOPE", "FREQUENCY", "Q"};
        const auto& labels = page_ == Page::dynamics ? dynamics : page_ == Page::detector ? detector : side;
        if (dynamic_on_) {
            g.setColour(zlgui::glass::textSecondary().withAlpha(.72f * childAlpha));
            g.setFont(juce::FontOptions(base_.getFontSize() * .54f));
            for (size_t i = 0; i < labels.size(); ++i)
                g.drawText(labels[i], page_ == Page::dynamics ? primary_label_bounds_[i] : detail_label_bounds_[i],
                           juce::Justification::centredBottom, false);
        }

        if (!dynamic_on_) {
            auto message = surface.toNearestInt().reduced(24, 18);
            message.removeFromTop(juce::roundToInt(base_.getFontSize() * 4.2f));
            g.setColour(zlgui::glass::textSecondary().withAlpha(.74f * childAlpha));
            g.setFont(juce::FontOptions(base_.getFontSize() * .66f));
            g.drawText("Enable Dynamic EQ to reveal range, timing, detector and sidechain controls.",
                       message, juce::Justification::centred, true);
        }
    }
}
