// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#include "float_pop_panel.hpp"
#include "../../../../gui/glass_tokens.hpp"
#include "BinaryData.h"
#include <array>
#include <cstdio>

namespace zlpanel {
    FloatPopPanel::FloatPopPanel(PluginProcessor& p, zlgui::UIBase& base,
                                 const multilingual::TooltipHelper& tooltip_helper) :
        p_ref_(p), base_(base), updater_(),
        control_background_(base, .18f),
        bypass_drawable_(juce::Drawable::createFromImageData(BinaryData::bypass_svg,
                                                             BinaryData::bypass_svgSize)),
        bypass_button_(base, bypass_drawable_.get(), bypass_drawable_.get(),
                       tooltip_helper.getToolTipText(multilingual::kBandBypass)),
        solo_drawable_(juce::Drawable::createFromImageData(BinaryData::solo_svg, BinaryData::solo_svgSize)),
        solo_button_(base, solo_drawable_.get(), solo_drawable_.get(),
                     tooltip_helper.getToolTipText(multilingual::kBandSolo)),
        close_drawable_(juce::Drawable::createFromImageData(BinaryData::trash_svg,
                                                            BinaryData::trash_svgSize)),
        close_button_(base, close_drawable_.get(), nullptr),
        dynamic_drawable_(juce::Drawable::createFromImageData(BinaryData::dynamic_svg,
                                                              BinaryData::dynamic_svgSize)),
        dynamic_button_(base, dynamic_drawable_.get(), dynamic_drawable_.get(),
                        tooltip_helper.getToolTipText(multilingual::kBandDynamic)),
        dynamics_page_button_(base, "Dynamic"),
        detector_page_button_(base, "Detector"),
        sidechain_page_button_(base, "Sidechain"),
        more_button_(base, "..."),
        ftype_box_([]() -> std::vector<std::unique_ptr<juce::Drawable>> {
            std::vector<std::unique_ptr<juce::Drawable>> icons;
            icons.emplace_back(juce::Drawable::createFromImageData(BinaryData::peak_svg, BinaryData::peak_svgSize));
            icons.emplace_back(juce::Drawable::createFromImageData(BinaryData::lowshelf_svg, BinaryData::lowshelf_svgSize));
            icons.emplace_back(juce::Drawable::createFromImageData(BinaryData::lowpass_svg, BinaryData::lowpass_svgSize));
            icons.emplace_back(juce::Drawable::createFromImageData(BinaryData::highshelf_svg, BinaryData::highshelf_svgSize));
            icons.emplace_back(juce::Drawable::createFromImageData(BinaryData::highpass_svg, BinaryData::highpass_svgSize));
            icons.emplace_back(juce::Drawable::createFromImageData(BinaryData::notch_svg, BinaryData::notch_svgSize));
            icons.emplace_back(juce::Drawable::createFromImageData(BinaryData::bandpass_svg, BinaryData::bandpass_svgSize));
            icons.emplace_back(juce::Drawable::createFromImageData(BinaryData::tiltshelf_svg, BinaryData::tiltshelf_svgSize));
            icons.emplace_back(juce::Drawable::createFromImageData(BinaryData::flatshelf_svg, BinaryData::flatshelf_svgSize));
            icons.emplace_back(juce::Drawable::createFromImageData(BinaryData::allpass_svg, BinaryData::allpass_svgSize));
            icons.emplace_back(juce::Drawable::createFromImageData(BinaryData::flatgain_svg, BinaryData::flatgain_svgSize));
            return icons;
        }(), base, "", {"Bell", "Low Shelf", "High Cut", "High Shelf", "Low Cut",
                          "Notch", "Band Pass", "Tilt", "Flat Tilt", "All Pass", "Gain"}),
        lr_box_([]() -> std::vector<std::unique_ptr<juce::Drawable>> {
            std::vector<std::unique_ptr<juce::Drawable>> icons;
            icons.emplace_back(juce::Drawable::createFromImageData(BinaryData::stereo_svg, BinaryData::stereo_svgSize));
            icons.emplace_back(juce::Drawable::createFromImageData(BinaryData::left_svg, BinaryData::left_svgSize));
            icons.emplace_back(juce::Drawable::createFromImageData(BinaryData::right_svg, BinaryData::right_svgSize));
            icons.emplace_back(juce::Drawable::createFromImageData(BinaryData::mid_svg, BinaryData::mid_svgSize));
            icons.emplace_back(juce::Drawable::createFromImageData(BinaryData::side_svg, BinaryData::side_svgSize));
            return icons;
        }(), base, "", {"Stereo", "Left", "Right", "Mid", "Side"}),
        slope_box_(zlp::POrder::kChoices, base, ""),
        freq_slider_("", base),
        gain_slider_("", base),
        q_slider_("", base),
        range_slider_("", base),
        threshold_slider_("", base),
        attack_slider_("", base),
        release_slider_("", base),
        knee_slider_("", base),
        rms_length_slider_("", base),
        rms_mix_slider_("", base),
        smooth_slider_("", base),
        learn_button_(base, "Learn"),
        relative_button_(base, "Relative"),
        dyn_bypass_button_(base, "Dyn Bypass"),
        side_type_box_(zlp::PSideFilterType::kChoices, base, ""),
        side_order_box_(zlp::PSideOrder::kChoices, base, ""),
        side_freq_slider_("", base),
        side_q_slider_("", base),
        side_link_button_(base, "Link"),
        side_swap_button_(base, "Swap") {
        control_background_.setBufferedToImage(true);
        addAndMakeVisible(control_background_);

        close_button_.setImageAlpha(0.f, 0.f, 0.f, 0.f);
        close_button_.setBufferedToImage(true);
        addAndMakeVisible(close_button_);
        close_button_.getButton().onClick = [this]() {
            if (const auto c_band = base_.getSelectedBand(); c_band < zlp::kBandNum) {
                band_helper::turnOffBand(p_ref_, c_band, base_.getSelectedBandSet());
                const auto band1 = band_helper::findClosestBand<true>(p_ref_, c_band);
                const auto band2 = band_helper::findClosestBand<false>(p_ref_, c_band);
                if (band1 < zlp::kBandNum) {
                    base_.setSelectedBand(band1);
                } else if (band2 < zlp::kBandNum) {
                    base_.setSelectedBand(band2);
                } else {
                    base_.setSelectedBand(zlp::kBandNum);
                }
            }
        };

        solo_button_.setImageAlpha(0.f, 0.f, 0.f, 0.f);
        solo_button_.setBufferedToImage(true);
        addAndMakeVisible(solo_button_);
        solo_button_.getButton().onClick = [this]() {
            if (solo_button_.getButton().getToggleState()) {
                if (const auto c_band = base_.getSelectedBand(); c_band < zlp::kBandNum) {
                    base_.setSoloWholeIdx(c_band);
                }
            } else {
                base_.setSoloWholeIdx(2 * zlp::kBandNum);
            }
        };

        bypass_button_.setImageAlpha(0.f, 0.f, 0.f, 0.f);
        bypass_button_.setBufferedToImage(true);
        addAndMakeVisible(bypass_button_);
        bypass_button_.getButton().onClick = [this]() {
            if (const auto c_band = base_.getSelectedBand(); c_band < zlp::kBandNum) {
                updateValue(p_ref_.parameters_.getParameter(zlp::PFilterStatus::kID + std::to_string(c_band)),
                            bypass_button_.getToggleState() ? 1.f : .5f);
            }
        };

        dynamic_button_.setImageAlpha(0.f, 0.f, 0.f, 0.f);
        dynamic_button_.setBufferedToImage(true);
        addAndMakeVisible(dynamic_button_);
        dynamic_button_.getButton().onClick = [this]() {
            if (const auto c_band = base_.getSelectedBand(); c_band < zlp::kBandNum) {
                const auto max_idx = std::round(p_ref_.parameters_NA_.getRawParameterValue(
                    zlstate::PEQMaxDB::kID)->load(std::memory_order::relaxed));
                band_helper::turnOnOffDynamic(p_ref_, c_band, dynamic_button_.getToggleState(),
                                              base_.getCurveDBScale(static_cast<size_t>(max_idx)));
            }
            if (!dynamic_button_.getToggleState() && base_.getSoloWholeIdx() < 2 * zlp::kBandNum) {
                base_.setSoloWholeIdx(2 * zlp::kBandNum);
            }
        };

        for (auto* b : {&dynamics_page_button_, &detector_page_button_, &sidechain_page_button_}) {
            styleDetailButton(*b);
            b->getButton().setToggleable(true);
            b->getButton().setClickingTogglesState(false);
            addAndMakeVisible(*b);
        }
        dynamics_page_button_.getButton().setToggleState(true, juce::dontSendNotification);
        dynamics_page_button_.getButton().onClick = [this]() { updateDetailPage(DetailPage::dynamics, true); };
        detector_page_button_.getButton().onClick = [this]() { updateDetailPage(DetailPage::detector, true); };
        sidechain_page_button_.getButton().onClick = [this]() { updateDetailPage(DetailPage::sidechain, true); };

        styleDetailButton(more_button_);
        more_button_.getLAF().setFontScale(.72f);
        more_button_.getButton().onClick = [this]() {
            advanced_open_ = !advanced_open_;
            if (auto* parent = getParentComponent()) parent->resized();
            resized();
            repaint();
        };
        addAndMakeVisible(more_button_);

        for (auto* b : {&learn_button_, &relative_button_, &dyn_bypass_button_, &side_link_button_, &side_swap_button_}) {
            styleDetailButton(*b);
            b->getButton().setToggleable(true);
            b->getButton().setClickingTogglesState(true);
            addAndMakeVisible(*b);
        }

        side_type_box_.setBufferedToImage(true);
        side_type_box_.getLAF().setFontScale(.72f);
        side_type_box_.getLAF().setBoxAlpha(.70f);
        side_type_box_.getLAF().setLabelJustification(juce::Justification::centred);
        addAndMakeVisible(side_type_box_);
        side_order_box_.setBufferedToImage(true);
        side_order_box_.getLAF().setFontScale(.72f);
        side_order_box_.getLAF().setBoxAlpha(.70f);
        side_order_box_.getLAF().setLabelJustification(juce::Justification::centred);
        addAndMakeVisible(side_order_box_);

        const auto popup_option1 = juce::PopupMenu::Options().withPreferredPopupDirection(
            juce::PopupMenu::Options::PopupDirection::upwards).withMinimumNumColumns(1);
        ftype_box_.getLAF().setOption(popup_option1);
        ftype_box_.setBufferedToImage(true);
        ftype_box_.setAlpha(.01f); // invisible hit target; the parent paints the Liquid Glass selector
        addAndMakeVisible(ftype_box_);

        const auto popup_option2 = juce::PopupMenu::Options().withPreferredPopupDirection(
            juce::PopupMenu::Options::PopupDirection::upwards).withMinimumNumColumns(1);
        lr_box_.setScrollEnabled(true);
        lr_box_.getLAF().setOption(popup_option2);
        lr_box_.setBufferedToImage(true);
        lr_box_.setAlpha(.01f); // keep the combobox clickable while the parent paints the clean text pill
        addAndMakeVisible(lr_box_);

        slope_box_.setScrollEnabled(true);
        slope_box_.getLAF().setOption(popup_option2);
        slope_box_.getLAF().setFontScale(.66f);
        slope_box_.getLAF().setBoxAlpha(.70f);
        slope_box_.getLAF().setLabelJustification(juce::Justification::centred);
        slope_box_.setBufferedToImage(true);
        addAndMakeVisible(slope_box_);

        auto setup_plain_slider = [this](auto& slider) {
            slider.setFontScale(1.10f);
            slider.getSlider().setSliderSnapsToMousePosition(false);
            slider.setBufferedToImage(true);
            addAndMakeVisible(slider);
        };

        setup_plain_slider(freq_slider_);
        freq_slider_.setPrecision(4);
        freq_slider_.permitted_characters_ = "0123456789.kK";
        freq_slider_.value_formatter_ = [](const double value) -> std::string {
            char buffer[32];
            if (value >= 1000.0) {
                snprintf(buffer, sizeof(buffer), "%.2f kHz", value * 0.001);
            } else {
                snprintf(buffer, sizeof(buffer), value >= 100.0 ? "%.0f Hz" : "%.1f Hz", value);
            }
            return buffer;
        };

        setup_plain_slider(gain_slider_);
        gain_slider_.setPrecision(3);
        gain_slider_.permitted_characters_ = "-+0123456789.";
        gain_slider_.value_formatter_ = [](const double value) -> std::string {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%+.2f dB", value);
            return buffer;
        };

        setup_plain_slider(q_slider_);
        q_slider_.setPrecision(3);
        q_slider_.permitted_characters_ = "0123456789.";
        q_slider_.value_formatter_ = [](const double value) -> std::string {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "Q %.2f", value);
            return buffer;
        };

        setup_plain_slider(range_slider_);
        range_slider_.setPrecision(3);
        range_slider_.permitted_characters_ = "-+0123456789.";
        range_slider_.value_formatter_ = [this](const double target_value) -> std::string {
            double base_gain = 0.0;
            if (const auto band = base_.getSelectedBand(); band < zlp::kBandNum) {
                if (const auto* value = p_ref_.parameters_.getRawParameterValue(zlp::PGain::kID + std::to_string(band))) {
                    base_gain = value->load(std::memory_order::relaxed);
                }
            }
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%+.2f dB", target_value - base_gain);
            return buffer;
        };
        range_slider_.string_formatter_ = [this](const std::string& text) -> std::optional<double> {
            double base_gain = 0.0;
            if (const auto band = base_.getSelectedBand(); band < zlp::kBandNum) {
                if (const auto* value = p_ref_.parameters_.getRawParameterValue(zlp::PGain::kID + std::to_string(band))) {
                    base_gain = value->load(std::memory_order::relaxed);
                }
            }
            return base_gain + juce::String(text).getDoubleValue();
        };

        setup_plain_slider(threshold_slider_);
        threshold_slider_.setPrecision(3);
        threshold_slider_.permitted_characters_ = "-0123456789.";
        threshold_slider_.value_formatter_ = [](const double value) -> std::string {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%.1f dB", value);
            return buffer;
        };

        setup_plain_slider(attack_slider_);
        attack_slider_.setPrecision(4);
        attack_slider_.permitted_characters_ = "0123456789.";
        attack_slider_.value_formatter_ = [](const double value) -> std::string {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), value < 100.0 ? "%.1f ms" : "%.0f ms", value);
            return buffer;
        };

        setup_plain_slider(release_slider_);
        range_slider_.setFontScale(.86f);
        threshold_slider_.setFontScale(.86f);
        attack_slider_.setFontScale(.86f);
        release_slider_.setFontScale(.86f);
        release_slider_.setPrecision(4);
        release_slider_.permitted_characters_ = "0123456789.";
        release_slider_.value_formatter_ = [](const double value) -> std::string {
            char buffer[32];
            if (value >= 1000.0) {
                snprintf(buffer, sizeof(buffer), "%.2f s", value * .001);
            } else {
                snprintf(buffer, sizeof(buffer), "%.0f ms", value);
            }
            return buffer;
        };

        for (auto* s : {&knee_slider_, &rms_length_slider_, &rms_mix_slider_, &smooth_slider_,
                        &side_freq_slider_, &side_q_slider_}) {
            setup_plain_slider(*s);
            s->setFontScale(.82f);
        }
        knee_slider_.setPrecision(3);
        knee_slider_.permitted_characters_ = "0123456789.";
        knee_slider_.value_formatter_ = [](const double v) -> std::string {
            char buffer[32]; snprintf(buffer, sizeof(buffer), "%.1f dB", v); return buffer;
        };
        rms_length_slider_.setPrecision(3);
        rms_length_slider_.permitted_characters_ = "0123456789.";
        rms_length_slider_.value_formatter_ = [](const double v) -> std::string {
            char buffer[32]; snprintf(buffer, sizeof(buffer), "%.1f ms", v); return buffer;
        };
        rms_mix_slider_.setPrecision(3);
        rms_mix_slider_.permitted_characters_ = "0123456789.";
        rms_mix_slider_.value_formatter_ = [](const double v) -> std::string {
            char buffer[32]; snprintf(buffer, sizeof(buffer), "%.0f%%", v); return buffer;
        };
        smooth_slider_.setPrecision(3);
        smooth_slider_.permitted_characters_ = "0123456789.";
        smooth_slider_.value_formatter_ = [](const double v) -> std::string {
            char buffer[32]; snprintf(buffer, sizeof(buffer), "%.0f%%", v); return buffer;
        };
        side_freq_slider_.setPrecision(4);
        side_freq_slider_.permitted_characters_ = "0123456789.kK";
        side_freq_slider_.value_formatter_ = [](const double value) -> std::string {
            char buffer[32];
            if (value >= 1000.0) snprintf(buffer, sizeof(buffer), "%.2f kHz", value * .001);
            else snprintf(buffer, sizeof(buffer), value >= 100.0 ? "%.0f Hz" : "%.1f Hz", value);
            return buffer;
        };
        side_q_slider_.setPrecision(3);
        side_q_slider_.permitted_characters_ = "0123456789.";
        side_q_slider_.value_formatter_ = [](const double v) -> std::string {
            char buffer[32]; snprintf(buffer, sizeof(buffer), "Q %.2f", v); return buffer;
        };

        updateDynamicVisibility(false, false);
        base_.getSoloWholeIdxTree().addListener(this);
    }

    FloatPopPanel::~FloatPopPanel() {
        base_.getSoloWholeIdxTree().removeListener(this);
    }

    void FloatPopPanel::paintOverChildren(juce::Graphics& g) {
        static constexpr std::array<const char*, 11> filter_names{
            "Bell", "Low Shelf", "High Cut", "High Shelf", "Low Cut", "Notch",
            "Band Pass", "Tilt", "Flat Tilt", "All Pass", "Gain"
        };
        static constexpr std::array<const char*, 5> mode_names{"Stereo", "Left", "Right", "Mid", "Side"};

        const auto text_colour = juce::Colour(246, 251, 255);
        const auto quiet_colour = text_colour.withAlpha(.52f);

        // Repaint the legacy button hit-targets with a new minimal glyph language.
        // The underlying buttons still own the parameter interactions, but none of ZL's
        // original icon artwork is visible.
        const auto draw_power = [&g, &text_colour](const juce::Rectangle<int>& ib, const float alpha) {
            auto r = ib.toFloat().reduced(ib.getHeight() * .27f);
            g.setColour(text_colour.withAlpha(alpha));
            juce::Path power_arc;
            power_arc.addCentredArc(r.getCentreX(), r.getCentreY(), r.getWidth() * .5f,
                                    r.getHeight() * .5f, 0.f, .72f, 5.56f, true);
            g.strokePath(power_arc, juce::PathStrokeType(1.45f, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
            g.drawLine(r.getCentreX(), r.getY(), r.getCentreX(), r.getCentreY(), 1.45f);
        };
        draw_power(bypass_button_.getBounds(), bypass_button_.getToggleState() ? .42f : .92f);

        // Solo and delete stay available but visually secondary to the filter controls.
        {
            auto r = solo_button_.getBounds().toFloat().reduced(solo_button_.getHeight() * .30f);
            const auto active = solo_button_.getButton().getToggleState();
            g.setColour(text_colour.withAlpha(active ? .92f : .40f));
            juce::Path hp;
            hp.addCentredArc(r.getCentreX(), r.getCentreY() + r.getHeight() * .08f,
                             r.getWidth() * .40f, r.getHeight() * .42f, 0.f, 3.48f, 5.94f, true);
            g.strokePath(hp, juce::PathStrokeType(1.25f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.drawRoundedRectangle(r.getX(), r.getCentreY(), r.getWidth() * .19f, r.getHeight() * .36f, 2.f, 1.1f);
            g.drawRoundedRectangle(r.getRight() - r.getWidth() * .19f, r.getCentreY(), r.getWidth() * .19f, r.getHeight() * .36f, 2.f, 1.1f);
        }
        {
            auto r = close_button_.getBounds().toFloat().reduced(close_button_.getHeight() * .31f);
            g.setColour(text_colour.withAlpha(.34f));
            g.drawRoundedRectangle(r.getX() + r.getWidth() * .19f, r.getY() + r.getHeight() * .25f,
                                   r.getWidth() * .62f, r.getHeight() * .62f, 1.5f, 1.1f);
            g.drawLine(r.getX() + r.getWidth() * .12f, r.getY() + r.getHeight() * .22f,
                       r.getRight() - r.getWidth() * .12f, r.getY() + r.getHeight() * .22f, 1.1f);
            g.drawLine(r.getX() + r.getWidth() * .37f, r.getY() + r.getHeight() * .11f,
                       r.getX() + r.getWidth() * .63f, r.getY() + r.getHeight() * .11f, 1.1f);
        }

        {
            auto r = dynamic_button_.getBounds().toFloat().reduced(dynamic_button_.getHeight() * .29f);
            juce::Path wave;
            wave.startNewSubPath(r.getX(), r.getCentreY());
            wave.cubicTo(r.getX() + r.getWidth() * .22f, r.getY(),
                         r.getX() + r.getWidth() * .28f, r.getBottom(),
                         r.getX() + r.getWidth() * .50f, r.getCentreY());
            wave.cubicTo(r.getX() + r.getWidth() * .72f, r.getY(),
                         r.getX() + r.getWidth() * .78f, r.getBottom(),
                         r.getRight(), r.getCentreY());
            const auto a = dynamic_button_.getToggleState() ? .95f : .48f;
            g.setColour(juce::Colour(188, 216, 255).withAlpha(a));
            g.strokePath(wave, juce::PathStrokeType(1.45f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        }

        // Filter-type glyph: a tiny bell response instead of the old diamond ZL icon.
        {
            auto r = ftype_box_.getBounds().toFloat().reduced(ftype_box_.getHeight() * .24f);
            juce::Path bell;
            bell.startNewSubPath(r.getX(), r.getBottom() - r.getHeight() * .18f);
            bell.cubicTo(r.getX() + r.getWidth() * .26f, r.getBottom() - r.getHeight() * .18f,
                         r.getX() + r.getWidth() * .28f, r.getY() + r.getHeight() * .12f,
                         r.getCentreX(), r.getY() + r.getHeight() * .12f);
            bell.cubicTo(r.getX() + r.getWidth() * .72f, r.getY() + r.getHeight() * .12f,
                         r.getX() + r.getWidth() * .74f, r.getBottom() - r.getHeight() * .18f,
                         r.getRight(), r.getBottom() - r.getHeight() * .18f);
            g.setColour(text_colour.withAlpha(.86f));
            g.strokePath(bell, juce::PathStrokeType(1.25f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        }

        const auto f_idx = ftype_box_.getBox().getSelectedItemIndex();
        const auto lr_idx = lr_box_.getBox().getSelectedItemIndex();

        // A restrained band-colour glint along the upper rim ties the inspector to the
        // selected node without colouring the whole card.
        if (const auto band = base_.getSelectedBand(); band < zlp::kBandNum) {
            const auto accent = base_.getColourMap1(band);
            auto accent_line = getLocalBounds().toFloat().reduced(base_.getFontSize() * .85f);
            accent_line.setHeight(1.15f);
            juce::ColourGradient line_gradient(accent.withAlpha(.0f), accent_line.getX(), accent_line.getY(),
                                                accent.interpolatedWith(juce::Colours::white, .35f).withAlpha(.48f), accent_line.getCentreX(), accent_line.getY(), false);
            line_gradient.addColour(.82, accent.withAlpha(.16f));
            line_gradient.addColour(1.0, accent.withAlpha(.0f));
            g.setGradientFill(line_gradient);
            g.fillRoundedRectangle(accent_line, .6f);
        }

        // Fine glass dividers give the numerical controls hierarchy while keeping the
        // card far quieter than the old knob panel.
        if (!freq_slider_.getBounds().isEmpty() && !gain_slider_.getBounds().isEmpty()) {
            const auto x = (freq_slider_.getRight() + gain_slider_.getX()) * .5f;
            const auto y1 = static_cast<float>(freq_slider_.getY()) + base_.getFontSize() * .18f;
            const auto y2 = static_cast<float>(q_slider_.getBottom()) - base_.getFontSize() * .18f;
            g.setColour(juce::Colour(240, 249, 255).withAlpha(.080f));
            g.drawLine(x, y1, x, y2, 1.f);
        }

        auto filter_outline = ftype_box_.getBounds().getUnion(filter_name_bound_).toFloat().expanded(2.f, 1.f);
        juce::ColourGradient filter_glass(juce::Colour(250, 253, 255).withAlpha(.085f),
                                          filter_outline.getCentreX(), filter_outline.getY(),
                                          juce::Colour(42, 67, 88).withAlpha(.13f),
                                          filter_outline.getCentreX(), filter_outline.getBottom(), false);
        g.setGradientFill(filter_glass);
        g.fillRoundedRectangle(filter_outline, filter_outline.getHeight() * .5f);
        g.setColour(juce::Colour(248, 252, 255).withAlpha(.16f));
        g.drawRoundedRectangle(filter_outline, filter_outline.getHeight() * .5f, .8f);

        g.setColour(text_colour.withAlpha(.94f));
        g.setFont(juce::FontOptions(base_.getFontSize() * .94f));
        if (f_idx >= 0 && f_idx < static_cast<int>(filter_names.size())) {
            g.drawText(filter_names[static_cast<size_t>(f_idx)], filter_name_bound_,
                       juce::Justification::centredLeft, false);
        }

        if (lr_idx >= 0 && lr_idx < static_cast<int>(mode_names.size())) {
            auto mode_pill = mode_name_bound_.toFloat().reduced(.5f);
            g.setColour(juce::Colour(230, 244, 255).withAlpha(.080f));
            g.fillRoundedRectangle(mode_pill, mode_pill.getHeight() * .5f);
            g.setColour(juce::Colour(235, 247, 255).withAlpha(.15f));
            g.drawRoundedRectangle(mode_pill, mode_pill.getHeight() * .5f, 1.f);
            g.setColour(quiet_colour);
            g.setFont(juce::FontOptions(base_.getFontSize() * .68f));
            g.drawText(mode_names[static_cast<size_t>(lr_idx)], mode_name_bound_,
                       juce::Justification::centred, false);
        }

        if (dynamic_on_) {
            const auto labels = detail_page_ == DetailPage::dynamics
                ? std::array<const char*, 4>{"RANGE", "THRESH", "ATTACK", "RELEASE"}
                : detail_page_ == DetailPage::detector
                    ? std::array<const char*, 4>{"KNEE", "LENGTH", "MIX", "SMOOTH"}
                    : std::array<const char*, 4>{"FILTER", "SLOPE", "FREQ", "Q"};

            auto separator = getLocalBounds().toFloat().reduced(static_cast<float>(getPaddingSize(base_.getFontSize()) * 2));
            separator.setY(static_cast<float>(detail_label_bounds_[0].getY()) - base_.getFontSize() * .46f);
            separator.setHeight(1.f);
            g.setColour(juce::Colour(232, 246, 255).withAlpha(.075f));
            g.fillRect(separator);

            g.setColour(quiet_colour.withMultipliedAlpha(.82f));
            g.setFont(juce::FontOptions(base_.getFontSize() * .58f));
            for (size_t i = 0; i < labels.size(); ++i) {
                g.drawText(labels[i], detail_label_bounds_[i], juce::Justification::centredBottom, false);
            }
        }
    }

    void FloatPopPanel::resized() {
        control_background_.setBounds(getLocalBounds());

        const auto padding = getPaddingSize(base_.getFontSize());
        const auto button_size = getButtonSize(base_.getFontSize());
        auto bound = getLocalBounds().reduced(padding + padding / 2, padding + padding / 2);

        // Primary row: filter identity + quiet utility actions.
        auto header = bound.removeFromTop(button_size);
        ftype_box_.setBounds(header.removeFromLeft(button_size));
        filter_name_bound_ = header.removeFromLeft(juce::jmax(button_size * 2, juce::roundToInt(base_.getFontSize() * 4.35f)));

        more_button_.setBounds(header.removeFromRight(button_size));
        header.removeFromRight(padding / 5);
        bypass_button_.setBounds(header.removeFromRight(button_size));

        bound.removeFromTop(padding / 3);
        auto values = bound.removeFromTop(button_size);
        const auto value_gap = juce::jmax(2, padding / 2);
        const auto value_w = (values.getWidth() - 2 * value_gap) / 3;
        freq_slider_.setBounds(values.removeFromLeft(value_w));
        values.removeFromLeft(value_gap);
        gain_slider_.setBounds(values.removeFromLeft(value_w));
        values.removeFromLeft(value_gap);
        q_slider_.setBounds(values);

        const auto show_advanced = advanced_open_;
        dynamic_button_.setVisible(show_advanced);
        solo_button_.setVisible(show_advanced);
        close_button_.setVisible(show_advanced);
        lr_box_.setVisible(show_advanced);
        slope_box_.setVisible(show_advanced && slope_supported_);
        mode_name_bound_ = {};
        if (show_advanced) {
            bound.removeFromTop(padding / 3);
            auto advanced = bound.removeFromTop(button_size);
            close_button_.setBounds(advanced.removeFromRight(button_size));
            advanced.removeFromRight(padding / 6);
            solo_button_.setBounds(advanced.removeFromRight(button_size));
            advanced.removeFromRight(padding / 6);
            dynamic_button_.setBounds(advanced.removeFromRight(button_size));
            advanced.removeFromRight(padding / 2);

            const auto mode_w = juce::jmax(button_size * 2, juce::roundToInt(base_.getFontSize() * 3.6f));
            mode_name_bound_ = advanced.removeFromRight(juce::jmin(mode_w, advanced.getWidth()));
            lr_box_.setBounds(mode_name_bound_);
            advanced.removeFromRight(padding / 2);
            if (slope_supported_) {
                slope_box_.setBounds(advanced);
            } else {
                slope_box_.setBounds({});
            }
        } else {
            dynamic_button_.setBounds({}); solo_button_.setBounds({}); close_button_.setBounds({});
            slope_box_.setBounds({}); lr_box_.setBounds({});
        }

        freq_label_bound_ = {};
        gain_label_bound_ = {};
        q_label_bound_ = {};
        for (auto& r : detail_label_bounds_) r = {};
        range_label_bound_ = threshold_label_bound_ = attack_label_bound_ = release_label_bound_ = {};

        if (dynamic_on_) {
            bound.removeFromTop(padding * 2 / 3);
            auto tabs = bound.removeFromTop(juce::jmax(button_size * 3 / 4, juce::roundToInt(base_.getFontSize() * 1.25f)));
            const auto tab_gap = juce::jmax(2, padding / 4);
            const auto tab_w = (tabs.getWidth() - 2 * tab_gap) / 3;
            dynamics_page_button_.setBounds(tabs.removeFromLeft(tab_w));
            tabs.removeFromLeft(tab_gap);
            detector_page_button_.setBounds(tabs.removeFromLeft(tab_w));
            tabs.removeFromLeft(tab_gap);
            sidechain_page_button_.setBounds(tabs);

            bound.removeFromTop(padding / 2);
            const auto label_h = juce::jmax(8, juce::roundToInt(base_.getFontSize() * .66f));
            auto labels = bound.removeFromTop(label_h);
            auto values_row = bound.removeFromTop(button_size);
            const auto gap = juce::jmax(2, padding / 3);
            const auto cell_w = (values_row.getWidth() - 3 * gap) / 4;
            for (size_t i = 0; i < 4; ++i) {
                detail_label_bounds_[i] = labels.removeFromLeft(cell_w);
                if (i < 3) labels.removeFromLeft(gap);
            }

            if (detail_page_ == DetailPage::dynamics) {
                range_slider_.setBounds(values_row.removeFromLeft(cell_w)); values_row.removeFromLeft(gap);
                threshold_slider_.setBounds(values_row.removeFromLeft(cell_w)); values_row.removeFromLeft(gap);
                attack_slider_.setBounds(values_row.removeFromLeft(cell_w)); values_row.removeFromLeft(gap);
                release_slider_.setBounds(values_row);
                range_label_bound_ = detail_label_bounds_[0]; threshold_label_bound_ = detail_label_bounds_[1];
                attack_label_bound_ = detail_label_bounds_[2]; release_label_bound_ = detail_label_bounds_[3];
            } else if (detail_page_ == DetailPage::detector) {
                knee_slider_.setBounds(values_row.removeFromLeft(cell_w)); values_row.removeFromLeft(gap);
                rms_length_slider_.setBounds(values_row.removeFromLeft(cell_w)); values_row.removeFromLeft(gap);
                rms_mix_slider_.setBounds(values_row.removeFromLeft(cell_w)); values_row.removeFromLeft(gap);
                smooth_slider_.setBounds(values_row);

                bound.removeFromTop(padding / 3);
                auto flags = bound.removeFromTop(juce::jmax(button_size * 3 / 4, juce::roundToInt(base_.getFontSize() * 1.20f)));
                const auto fw = (flags.getWidth() - 2 * gap) / 3;
                learn_button_.setBounds(flags.removeFromLeft(fw)); flags.removeFromLeft(gap);
                relative_button_.setBounds(flags.removeFromLeft(fw)); flags.removeFromLeft(gap);
                dyn_bypass_button_.setBounds(flags);
            } else {
                side_type_box_.setBounds(values_row.removeFromLeft(cell_w)); values_row.removeFromLeft(gap);
                side_order_box_.setBounds(values_row.removeFromLeft(cell_w)); values_row.removeFromLeft(gap);
                side_freq_slider_.setBounds(values_row.removeFromLeft(cell_w)); values_row.removeFromLeft(gap);
                side_q_slider_.setBounds(values_row);

                bound.removeFromTop(padding / 3);
                auto flags = bound.removeFromTop(juce::jmax(button_size * 3 / 4, juce::roundToInt(base_.getFontSize() * 1.20f)));
                const auto fw = juce::jmax(button_size * 2, (flags.getWidth() - gap) / 2);
                side_link_button_.setBounds(flags.removeFromLeft(fw)); flags.removeFromLeft(gap);
                side_swap_button_.setBounds(flags.removeFromLeft(juce::jmin(fw, flags.getWidth())));
            }
        } else {
            dynamics_page_button_.setBounds({}); detector_page_button_.setBounds({}); sidechain_page_button_.setBounds({});
        }

        updateDetailVisibility();
        ideal_height_ = static_cast<float>(getIdealHeight());
        ideal_width_ = static_cast<float>(getIdealWidth());
    }

    void FloatPopPanel::mouseUp(const juce::MouseEvent& event) {
        // The Liquid Glass filter selector is painted by this component, while the
        // compact combobox occupies the icon. Make its visible text part interactive.
        if (filter_name_bound_.contains(event.getPosition())) {
            ftype_box_.getBox().showPopup();
        }
    }

    void FloatPopPanel::updateBand() {
        if (base_.getSelectedBand() < zlp::kBandNum) {
            const auto band_s = std::to_string(base_.getSelectedBand());
            ftype_attachment_ = std::make_unique<zlgui::attachment::ComboBoxAttachment<true>>(
                ftype_box_.getBox(), p_ref_.parameters_, zlp::PFilterType::kID + band_s, updater_);
            lr_attachment_ = std::make_unique<zlgui::attachment::ComboBoxAttachment<true>>(
                lr_box_.getBox(), p_ref_.parameters_, zlp::PLRMode::kID + band_s, updater_);
            slope_attachment_ = std::make_unique<zlgui::attachment::ComboBoxAttachment<true>>(
                slope_box_.getBox(), p_ref_.parameters_, zlp::POrder::kID + band_s, updater_);
            freq_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
                freq_slider_.getSlider(), p_ref_.parameters_, zlp::PFreq::kID + band_s, updater_);
            gain_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
                gain_slider_.getSlider(), p_ref_.parameters_, zlp::PGain::kID + band_s, updater_);
            q_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
                q_slider_.getSlider(), p_ref_.parameters_, zlp::PQ::kID + band_s, updater_);

            dynamic_attachment_ = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(
                dynamic_button_.getButton(), p_ref_.parameters_, zlp::PDynamicON::kID + band_s, updater_,
                juce::dontSendNotification);
            range_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
                range_slider_.getSlider(), p_ref_.parameters_, zlp::PTargetGain::kID + band_s, updater_);
            threshold_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
                threshold_slider_.getSlider(), p_ref_.parameters_, zlp::PThreshold::kID + band_s, updater_);
            attack_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
                attack_slider_.getSlider(), p_ref_.parameters_, zlp::PAttack::kID + band_s, updater_);
            release_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
                release_slider_.getSlider(), p_ref_.parameters_, zlp::PRelease::kID + band_s, updater_);
            knee_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
                knee_slider_.getSlider(), p_ref_.parameters_, zlp::PKneeW::kID + band_s, updater_);
            rms_length_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
                rms_length_slider_.getSlider(), p_ref_.parameters_, zlp::PDynamicRMSLength::kID + band_s, updater_);
            rms_mix_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
                rms_mix_slider_.getSlider(), p_ref_.parameters_, zlp::PDynamicRMSMix::kID + band_s, updater_);
            smooth_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
                smooth_slider_.getSlider(), p_ref_.parameters_, zlp::PDynamicSmooth::kID + band_s, updater_);

            learn_attachment_ = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(
                learn_button_.getButton(), p_ref_.parameters_, zlp::PDynamicLearn::kID + band_s, updater_, juce::dontSendNotification);
            relative_attachment_ = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(
                relative_button_.getButton(), p_ref_.parameters_, zlp::PDynamicRelative::kID + band_s, updater_, juce::dontSendNotification);
            dyn_bypass_attachment_ = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(
                dyn_bypass_button_.getButton(), p_ref_.parameters_, zlp::PDynamicBypass::kID + band_s, updater_, juce::dontSendNotification);

            side_type_attachment_ = std::make_unique<zlgui::attachment::ComboBoxAttachment<true>>(
                side_type_box_.getBox(), p_ref_.parameters_, zlp::PSideFilterType::kID + band_s, updater_);
            side_order_attachment_ = std::make_unique<zlgui::attachment::ComboBoxAttachment<true>>(
                side_order_box_.getBox(), p_ref_.parameters_, zlp::PSideOrder::kID + band_s, updater_);
            side_freq_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
                side_freq_slider_.getSlider(), p_ref_.parameters_, zlp::PSideFreq::kID + band_s, updater_);
            side_q_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
                side_q_slider_.getSlider(), p_ref_.parameters_, zlp::PSideQ::kID + band_s, updater_);
            side_link_attachment_ = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(
                side_link_button_.getButton(), p_ref_.parameters_, zlp::PSideLink::kID + band_s, updater_, juce::dontSendNotification);
            side_swap_attachment_ = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(
                side_swap_button_.getButton(), p_ref_.parameters_, zlp::PSideSwap::kID + band_s, updater_, juce::dontSendNotification);

            freq_attachment_->updateComponent();
            gain_attachment_->updateComponent();
            q_attachment_->updateComponent();
            slope_attachment_->updateComponent();
            range_attachment_->updateComponent();
            threshold_attachment_->updateComponent();
            attack_attachment_->updateComponent();
            release_attachment_->updateComponent();
            knee_attachment_->updateComponent();
            rms_length_attachment_->updateComponent();
            rms_mix_attachment_->updateComponent();
            smooth_attachment_->updateComponent();
            side_freq_attachment_->updateComponent();
            side_q_attachment_->updateComponent();

            filter_status_ptr_ = p_ref_.parameters_.getRawParameterValue(zlp::PFilterStatus::kID + band_s);
            dynamic_on_ptr_ = p_ref_.parameters_.getRawParameterValue(zlp::PDynamicON::kID + band_s);
            filter_type_ptr_ = p_ref_.parameters_.getRawParameterValue(zlp::PFilterType::kID + band_s);
            slope_ptr_ = p_ref_.parameters_.getRawParameterValue(zlp::POrder::kID + band_s);
            current_filter_type_ = -1;
            current_slope_ = -1;
            updateFilterCapabilities();
            const auto next_dynamic = dynamic_on_ptr_ != nullptr
                && dynamic_on_ptr_->load(std::memory_order::relaxed) > .5f;
            updateDynamicVisibility(next_dynamic, true);
            repaintCallBackSlow();
        } else {
            ftype_attachment_.reset();
            lr_attachment_.reset();
            slope_attachment_.reset();
            freq_attachment_.reset();
            gain_attachment_.reset();
            q_attachment_.reset();
            dynamic_attachment_.reset();
            range_attachment_.reset();
            threshold_attachment_.reset();
            attack_attachment_.reset();
            release_attachment_.reset();
            knee_attachment_.reset();
            rms_length_attachment_.reset();
            rms_mix_attachment_.reset();
            smooth_attachment_.reset();
            learn_attachment_.reset();
            relative_attachment_.reset();
            dyn_bypass_attachment_.reset();
            side_type_attachment_.reset();
            side_order_attachment_.reset();
            side_freq_attachment_.reset();
            side_q_attachment_.reset();
            side_link_attachment_.reset();
            side_swap_attachment_.reset();
            filter_status_ptr_ = nullptr;
            dynamic_on_ptr_ = nullptr;
            filter_type_ptr_ = nullptr;
            slope_ptr_ = nullptr;
            current_filter_type_ = -1;
            current_slope_ = -1;
            updateDynamicVisibility(false, true);
        }
        setVisible(base_.getSelectedBand() < zlp::kBandNum);
    }

    void FloatPopPanel::repaintCallBackSlow() {
        if (filter_status_ptr_ != nullptr) {
            updater_.updateComponents();
            freq_slider_.updateDisplayValue();
            gain_slider_.updateDisplayValue();
            q_slider_.updateDisplayValue();
            range_slider_.updateDisplayValue();
            threshold_slider_.updateDisplayValue();
            attack_slider_.updateDisplayValue();
            release_slider_.updateDisplayValue();
            knee_slider_.updateDisplayValue();
            rms_length_slider_.updateDisplayValue();
            rms_mix_slider_.updateDisplayValue();
            smooth_slider_.updateDisplayValue();
            side_freq_slider_.updateDisplayValue();
            side_q_slider_.updateDisplayValue();

            const auto filter_on = filter_status_ptr_->load(std::memory_order::relaxed) > 1.5f;
            if (filter_on != bypass_button_.getToggleState()) {
                bypass_button_.getButton().setToggleState(filter_on, juce::dontSendNotification);
            }

            if (dynamic_on_ptr_ != nullptr) {
                const auto next_dynamic = dynamic_on_ptr_->load(std::memory_order::relaxed) > .5f;
                if (next_dynamic != dynamic_on_) {
                    updateDynamicVisibility(next_dynamic, true);
                }
            }
            updateFilterCapabilities();
            repaint();
        }
    }

    void FloatPopPanel::updateFilterCapabilities() {
        if (filter_type_ptr_ == nullptr || slope_ptr_ == nullptr) return;

        const auto filter_type = static_cast<int>(std::round(
            filter_type_ptr_->load(std::memory_order::relaxed)));
        const auto slope = static_cast<int>(std::round(slope_ptr_->load(std::memory_order::relaxed)));
        if (filter_type == current_filter_type_ && slope == current_slope_) return;

        current_filter_type_ = filter_type;
        current_slope_ = slope;

        const auto slope_enabled = filter_type != static_cast<int>(zldsp::filter::kFlatTilt)
            && filter_type != static_cast<int>(zldsp::filter::kFlatGain);
        const auto slope_6_allowed = filter_type != static_cast<int>(zldsp::filter::kPeak)
            && filter_type != static_cast<int>(zldsp::filter::kBandPass)
            && filter_type != static_cast<int>(zldsp::filter::kNotch);
        if (!slope_6_allowed && slope_box_.getBox().getSelectedId() == 1) {
            slope_box_.getBox().setSelectedId(2, juce::sendNotificationSync);
        }
        slope_box_.getBox().setItemEnabled(1, slope_6_allowed);
        slope_supported_ = slope_enabled;
        slope_box_.setEditable(slope_enabled);

        const auto gain_enabled = filter_type == static_cast<int>(zldsp::filter::kPeak)
            || filter_type == static_cast<int>(zldsp::filter::kLowShelf)
            || filter_type == static_cast<int>(zldsp::filter::kHighShelf)
            || filter_type == static_cast<int>(zldsp::filter::kTiltShelf)
            || filter_type == static_cast<int>(zldsp::filter::kFlatTilt)
            || filter_type == static_cast<int>(zldsp::filter::kFlatGain);
        if (!gain_enabled && dynamic_button_.getToggleState()) {
            dynamic_button_.getButton().setToggleState(false, juce::sendNotificationSync);
        }
        gain_slider_.setEditable(gain_enabled);
        dynamic_button_.setAlpha(gain_enabled ? 1.f : .38f);
        dynamic_button_.setInterceptsMouseClicks(false, gain_enabled);
        q_slider_.setEditable(slope_enabled && slope != 0);
        resized();
        repaint();
    }

    int FloatPopPanel::getIdealWidth() const {
        const auto padding = getPaddingSize(base_.getFontSize());
        const auto button_size = getButtonSize(base_.getFontSize());
        return juce::roundToInt(6.35f * static_cast<float>(button_size) + 4.5f * static_cast<float>(padding));
    }

    int FloatPopPanel::getIdealHeight() const {
        const auto padding = getPaddingSize(base_.getFontSize());
        const auto button_size = getButtonSize(base_.getFontSize());
        const auto base_height = 2 * button_size + 3 * padding
            + (advanced_open_ ? button_size + padding / 2 : 0);
        if (!dynamic_on_) return base_height;

        const auto tabs_h = juce::jmax(button_size * 3 / 4, juce::roundToInt(base_.getFontSize() * 1.25f));
        const auto label_h = juce::jmax(8, juce::roundToInt(base_.getFontSize() * .66f));
        auto detail = tabs_h + label_h + button_size + 3 * padding;
        if (detail_page_ != DetailPage::dynamics) {
            detail += juce::jmax(button_size * 3 / 4, juce::roundToInt(base_.getFontSize() * 1.20f)) + padding / 2;
        }
        return base_height + detail;
    }

    void FloatPopPanel::styleDetailButton(zlgui::button::ClickTextButton& button) {
        button.getLAF().setFontScale(.68f);
        button.setBackgroundPainter([this](juce::Graphics& g, juce::Button& b,
                                           const bool highlighted, const bool down) {
            auto r = b.getLocalBounds().toFloat().reduced(.5f);
            const auto active = b.getToggleState() || down;
            g.setColour(active ? juce::Colour(135, 191, 238).withAlpha(.24f)
                               : juce::Colour(245, 251, 255).withAlpha(highlighted ? .085f : .038f));
            g.fillRoundedRectangle(r, r.getHeight() * .5f);
            g.setColour(juce::Colour(246, 252, 255).withAlpha(active ? .20f : .08f));
            g.drawRoundedRectangle(r, r.getHeight() * .5f, .75f);
        });
    }

    void FloatPopPanel::updateDetailPage(const DetailPage page, const bool request_parent_resize) {
        detail_page_ = page;
        dynamics_page_button_.getButton().setToggleState(page == DetailPage::dynamics, juce::dontSendNotification);
        detector_page_button_.getButton().setToggleState(page == DetailPage::detector, juce::dontSendNotification);
        sidechain_page_button_.getButton().setToggleState(page == DetailPage::sidechain, juce::dontSendNotification);
        updateDetailVisibility();
        if (request_parent_resize) {
            if (auto* parent = getParentComponent()) parent->resized();
        }
        // Detector and Sidechain have the same ideal height, so the parent may keep
        // our bounds unchanged and JUCE will not call resized(). Relayout explicitly
        // so controls made visible on the new page never retain empty bounds.
        resized();
        repaint();
    }

    void FloatPopPanel::updateDetailVisibility() {
        const auto dyn = dynamic_on_;
        dynamics_page_button_.setVisible(dyn);
        detector_page_button_.setVisible(dyn);
        sidechain_page_button_.setVisible(dyn);

        const auto showDynamics = dyn && detail_page_ == DetailPage::dynamics;
        const std::array<juce::Component*, 4> dynamicsComponents{
            &range_slider_, &threshold_slider_, &attack_slider_, &release_slider_};
        for (auto* c : dynamicsComponents) c->setVisible(showDynamics);

        const auto showDetector = dyn && detail_page_ == DetailPage::detector;
        const std::array<juce::Component*, 7> detectorComponents{
            &knee_slider_, &rms_length_slider_, &rms_mix_slider_, &smooth_slider_,
            &learn_button_, &relative_button_, &dyn_bypass_button_};
        for (auto* c : detectorComponents) c->setVisible(showDetector);

        const auto showSide = dyn && detail_page_ == DetailPage::sidechain;
        const std::array<juce::Component*, 6> sideComponents{
            &side_type_box_, &side_order_box_, &side_freq_slider_, &side_q_slider_,
            &side_link_button_, &side_swap_button_};
        for (auto* c : sideComponents) c->setVisible(showSide);
    }

    void FloatPopPanel::updateDynamicVisibility(const bool dynamic_on, const bool request_parent_resize) {
        dynamic_on_ = dynamic_on;
        if (!dynamic_on_) detail_page_ = DetailPage::dynamics;
        updateDetailPage(detail_page_, request_parent_resize);
    }

    void FloatPopPanel::setTargetVisible(const bool is_target_visible) {
        if (is_target_visible != is_target_visible_) {
            is_target_visible_ = is_target_visible;
            updateTransformation();
        }
    }

    void FloatPopPanel::updatePosition(const juce::Point<float> position,
                                       const juce::Point<float> target_position) {
        if (std::abs(position.x - position_.x) > .01f
            || std::abs(position.y - position_.y) > .01f
            || std::abs(target_position.x - target_position_.x) > .01f
            || std::abs(target_position.y - target_position_.y) > .01f) {
            position_ = position;
            target_position_ = target_position;
            updateTransformation();
        }
    }

    void FloatPopPanel::updateFloatingBound(const juce::Rectangle<float> bound) {
        const auto width = ideal_width_;
        const auto height = ideal_height_;
        const auto padding = base_.getFontSize() * kPaddingScale * 1.5f;

        upper_center_ = {width * .5f, -base_.getFontSize()};
        lower_center_ = {width * .5f, height + base_.getFontSize()};
        left_center_ = {-padding, ideal_height_ * .5f};
        right_center_ = {width + padding, ideal_height_ * .5f};

        x_min_ = width * .5f;
        x_mid_ = bound.getWidth() * .5f;
        x_max_ = bound.getWidth() - width * .5f;
        y_min_ = ideal_height_ * .5f;
        y_max_ = bound.getHeight() - height * .5f;
        floating_top_ = bound.getY();
        floating_bottom_ = bound.getBottom();

        y1_ = bound.getHeight() * .25f;
        y2_ = bound.getHeight() * .5f + .5f;
        y3_ = bound.getHeight() * .75f;

        updateTransformation();
    }

    void FloatPopPanel::updateTransformation() {
        const auto place_below = [this]() {
            setTransform(juce::AffineTransform::translation(
                std::clamp(position_.x, x_min_, x_max_) - upper_center_.x,
                position_.y - upper_center_.y));
        };
        const auto place_above = [this]() {
            setTransform(juce::AffineTransform::translation(
                std::clamp(position_.x, x_min_, x_max_) - lower_center_.x,
                position_.y - lower_center_.y));
        };
        const auto place_on_side = [this]() {
            if (position_.x < x_mid_) {
                setTransform(juce::AffineTransform::translation(
                    position_.x - left_center_.x,
                    std::clamp(position_.y, y_min_, y_max_) - left_center_.y));
            } else {
                setTransform(juce::AffineTransform::translation(
                    position_.x - right_center_.x,
                    std::clamp(position_.y, y_min_, y_max_) - right_center_.y));
            }
        };
        const auto place_using_default_rule = [this, &place_below, &place_above]() {
            if (position_.y < y1_ || (position_.y > y2_ && position_.y < y3_)) {
                place_below();
            } else {
                place_above();
            }
        };

        constexpr float target_position_epsilon = 1.f;
        const auto target_y_delta = target_position_.y - position_.y;
        if (!is_target_visible_ || std::abs(target_y_delta) <= target_position_epsilon) {
            place_using_default_rule();
        } else if (target_y_delta < 0.f) {
            const auto popup_bottom = position_.y - upper_center_.y + ideal_height_;
            if (popup_bottom <= floating_bottom_) {
                place_below();
            } else {
                place_on_side();
            }
        } else {
            const auto popup_top = position_.y - lower_center_.y;
            if (popup_top >= floating_top_) {
                place_above();
            } else {
                place_on_side();
            }
        }
    }

    void FloatPopPanel::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) {
        const auto solo_whole_idx = base_.getSoloWholeIdx();
        solo_button_.getButton().setToggleState(solo_whole_idx < zlp::kBandNum, juce::dontSendNotification);
    }
}
