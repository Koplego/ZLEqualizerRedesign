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
#include <algorithm>
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
        ftype_box_(juce::StringArray{"Bell", "Low Shelf", "High Cut", "High Shelf", "Low Cut",
                                     "Notch", "Band Pass", "Tilt", "Flat Tilt", "All Pass", "Gain"},
                   base, ""),
        lr_box_(juce::StringArray{"Stereo", "Left", "Right", "Mid", "Side"}, base, ""),
        slope_box_(zlp::POrder::kChoices, base, ""),
        freq_slider_("", base), gain_slider_("", base), q_slider_("", base),
        range_slider_("", base), threshold_slider_("", base), attack_slider_("", base), release_slider_("", base),
        knee_slider_("", base), rms_length_slider_("", base), rms_mix_slider_("", base), smooth_slider_("", base),
        learn_button_(base, "Learn"), relative_button_(base, "Relative"), dyn_bypass_button_(base, "Dyn Bypass"),
        side_type_box_(zlp::PSideFilterType::kChoices, base, ""),
        side_order_box_(zlp::PSideOrder::kChoices, base, ""),
        side_freq_slider_("", base), side_q_slider_("", base),
        side_link_button_(base, "Link"), side_swap_button_(base, "Swap") {
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
                if (band1 < zlp::kBandNum) base_.setSelectedBand(band1);
                else if (band2 < zlp::kBandNum) base_.setSelectedBand(band2);
                else base_.setSelectedBand(zlp::kBandNum);
            }
        };

        solo_button_.setImageAlpha(0.f, 0.f, 0.f, 0.f);
        solo_button_.setBufferedToImage(true);
        addAndMakeVisible(solo_button_);
        solo_button_.getButton().onClick = [this]() {
            if (solo_button_.getButton().getToggleState()) {
                if (const auto c_band = base_.getSelectedBand(); c_band < zlp::kBandNum)
                    base_.setSoloWholeIdx(c_band);
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
            if (!dynamic_button_.getToggleState() && base_.getSoloWholeIdx() < 2 * zlp::kBandNum)
                base_.setSoloWholeIdx(2 * zlp::kBandNum);
        };

        dynamics_page_button_.setVisible(false);
        for (auto* b : {&detector_page_button_, &sidechain_page_button_}) {
            styleDetailButton(*b);
            b->getButton().setToggleable(true);
            b->getButton().setClickingTogglesState(false);
            addAndMakeVisible(*b);
        }
        detector_page_button_.getButton().onClick = [this]() {
            updateDetailPage(detail_page_ == DetailPage::detector ? DetailPage::dynamics : DetailPage::detector, true);
        };
        sidechain_page_button_.getButton().onClick = [this]() {
            updateDetailPage(detail_page_ == DetailPage::sidechain ? DetailPage::dynamics : DetailPage::sidechain, true);
        };

        styleDetailButton(more_button_);
        more_button_.getLAF().setFontScale(.72f);
        more_button_.getButton().onClick = [this]() { setHubOpen(!hub_open_); };
        addAndMakeVisible(more_button_);

        for (auto* b : {&learn_button_, &relative_button_, &dyn_bypass_button_, &side_link_button_, &side_swap_button_}) {
            styleDetailButton(*b);
            b->getButton().setToggleable(true);
            b->getButton().setClickingTogglesState(true);
            addAndMakeVisible(*b);
        }

        side_type_box_.setBufferedToImage(true);
        side_type_box_.getLAF().setFontScale(.72f);
        side_type_box_.getLAF().setBoxAlpha(.34f);
        side_type_box_.getLAF().setLabelJustification(juce::Justification::centred);
        addAndMakeVisible(side_type_box_);
        side_order_box_.setBufferedToImage(true);
        side_order_box_.getLAF().setFontScale(.72f);
        side_order_box_.getLAF().setBoxAlpha(.34f);
        side_order_box_.getLAF().setLabelJustification(juce::Justification::centred);
        addAndMakeVisible(side_order_box_);

        const auto popup_up = juce::PopupMenu::Options().withPreferredPopupDirection(
            juce::PopupMenu::Options::PopupDirection::upwards).withMinimumNumColumns(1);
        ftype_box_.getLAF().setOption(popup_up);
        ftype_box_.setBufferedToImage(true);
        ftype_box_.setAlpha(.01f);
        addAndMakeVisible(ftype_box_);

        lr_box_.setScrollEnabled(true);
        lr_box_.getLAF().setOption(popup_up);
        lr_box_.setBufferedToImage(true);
        lr_box_.setAlpha(.01f);
        addAndMakeVisible(lr_box_);

        slope_box_.setScrollEnabled(true);
        slope_box_.getLAF().setOption(popup_up);
        slope_box_.getLAF().setFontScale(.64f);
        slope_box_.getLAF().setBoxAlpha(.36f);
        slope_box_.getLAF().setLabelJustification(juce::Justification::centred);
        slope_box_.setBufferedToImage(true);
        addAndMakeVisible(slope_box_);

        auto setup_plain_slider = [this](auto& slider) {
            slider.setFontScale(.90f);
            slider.getSlider().setSliderSnapsToMousePosition(false);
            slider.setBufferedToImage(true);
            addAndMakeVisible(slider);
        };

        setup_plain_slider(freq_slider_);
        freq_slider_.setPrecision(4);
        freq_slider_.permitted_characters_ = "0123456789.kK";
        freq_slider_.value_formatter_ = [](const double value) -> std::string {
            char buffer[32];
            if (value >= 1000.0) snprintf(buffer, sizeof(buffer), "%.2f kHz", value * .001);
            else snprintf(buffer, sizeof(buffer), value >= 100.0 ? "%.0f Hz" : "%.1f Hz", value);
            return buffer;
        };

        setup_plain_slider(gain_slider_);
        gain_slider_.setPrecision(3);
        gain_slider_.permitted_characters_ = "-+0123456789.";
        gain_slider_.value_formatter_ = [](const double value) -> std::string {
            char buffer[32]; snprintf(buffer, sizeof(buffer), "%+.2f dB", value); return buffer;
        };

        setup_plain_slider(q_slider_);
        q_slider_.setPrecision(3);
        q_slider_.permitted_characters_ = "0123456789.";
        q_slider_.value_formatter_ = [](const double value) -> std::string {
            char buffer[32]; snprintf(buffer, sizeof(buffer), "Q %.2f", value); return buffer;
        };

        setup_plain_slider(range_slider_);
        range_slider_.setPrecision(3);
        range_slider_.permitted_characters_ = "-+0123456789.";
        range_slider_.value_formatter_ = [this](const double target_value) -> std::string {
            double base_gain = 0.0;
            if (const auto band = base_.getSelectedBand(); band < zlp::kBandNum) {
                if (const auto* value = p_ref_.parameters_.getRawParameterValue(zlp::PGain::kID + std::to_string(band)))
                    base_gain = value->load(std::memory_order::relaxed);
            }
            char buffer[32]; snprintf(buffer, sizeof(buffer), "%+.2f dB", target_value - base_gain); return buffer;
        };
        range_slider_.string_formatter_ = [this](const std::string& text) -> std::optional<double> {
            double base_gain = 0.0;
            if (const auto band = base_.getSelectedBand(); band < zlp::kBandNum) {
                if (const auto* value = p_ref_.parameters_.getRawParameterValue(zlp::PGain::kID + std::to_string(band)))
                    base_gain = value->load(std::memory_order::relaxed);
            }
            return base_gain + juce::String(text).getDoubleValue();
        };

        setup_plain_slider(threshold_slider_);
        threshold_slider_.setPrecision(3);
        threshold_slider_.permitted_characters_ = "-0123456789.";
        threshold_slider_.value_formatter_ = [](const double value) -> std::string {
            char buffer[32]; snprintf(buffer, sizeof(buffer), "%.1f dB", value); return buffer;
        };

        setup_plain_slider(attack_slider_);
        attack_slider_.setPrecision(4);
        attack_slider_.permitted_characters_ = "0123456789.";
        attack_slider_.value_formatter_ = [](const double value) -> std::string {
            char buffer[32]; snprintf(buffer, sizeof(buffer), value < 100.0 ? "%.1f ms" : "%.0f ms", value); return buffer;
        };

        setup_plain_slider(release_slider_);
        release_slider_.setPrecision(4);
        release_slider_.permitted_characters_ = "0123456789.";
        release_slider_.value_formatter_ = [](const double value) -> std::string {
            char buffer[32];
            if (value >= 1000.0) snprintf(buffer, sizeof(buffer), "%.2f s", value * .001);
            else snprintf(buffer, sizeof(buffer), "%.0f ms", value);
            return buffer;
        };

        for (auto* s : {&range_slider_, &threshold_slider_, &attack_slider_, &release_slider_,
                        &knee_slider_, &rms_length_slider_, &rms_mix_slider_, &smooth_slider_,
                        &side_freq_slider_, &side_q_slider_}) {
            if (s != &range_slider_ && s != &threshold_slider_ && s != &attack_slider_ && s != &release_slider_)
                setup_plain_slider(*s);
            s->setFontScale(.78f);
        }

        knee_slider_.setPrecision(3);
        knee_slider_.value_formatter_ = [](const double v) -> std::string { char b[32]; snprintf(b, sizeof(b), "%.1f dB", v); return b; };
        rms_length_slider_.setPrecision(3);
        rms_length_slider_.value_formatter_ = [](const double v) -> std::string { char b[32]; snprintf(b, sizeof(b), "%.1f ms", v); return b; };
        rms_mix_slider_.setPrecision(3);
        rms_mix_slider_.value_formatter_ = [](const double v) -> std::string { char b[32]; snprintf(b, sizeof(b), "%.0f%%", v); return b; };
        smooth_slider_.setPrecision(3);
        smooth_slider_.value_formatter_ = [](const double v) -> std::string { char b[32]; snprintf(b, sizeof(b), "%.0f%%", v); return b; };
        side_freq_slider_.setPrecision(4);
        side_freq_slider_.value_formatter_ = [](const double value) -> std::string {
            char b[32];
            if (value >= 1000.0) snprintf(b, sizeof(b), "%.2f kHz", value * .001);
            else snprintf(b, sizeof(b), value >= 100.0 ? "%.0f Hz" : "%.1f Hz", value);
            return b;
        };
        side_q_slider_.setPrecision(3);
        side_q_slider_.value_formatter_ = [](const double v) -> std::string { char b[32]; snprintf(b, sizeof(b), "Q %.2f", v); return b; };

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
        const auto text = zlgui::glass::textPrimary();
        const auto quiet = zlgui::glass::textSecondary();
        const auto band = base_.getSelectedBand();
        const auto accent = band < zlp::kBandNum ? base_.getColourMap1(band) : juce::Colour(126, 185, 232);
        const auto f_idx = ftype_box_.getBox().getSelectedItemIndex();
        const auto lr_idx = lr_box_.getBox().getSelectedItemIndex();

        auto drawPower = [&g, &text](const juce::Rectangle<int>& ib, float alpha) {
            if (ib.isEmpty()) return;
            auto r = ib.toFloat().reduced(ib.getHeight() * .28f);
            g.setColour(text.withAlpha(alpha));
            juce::Path p;
            p.addCentredArc(r.getCentreX(), r.getCentreY(), r.getWidth() * .5f, r.getHeight() * .5f,
                            0.f, .72f, 5.56f, true);
            g.strokePath(p, juce::PathStrokeType(1.35f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.drawLine(r.getCentreX(), r.getY(), r.getCentreX(), r.getCentreY(), 1.35f);
        };
        drawPower(bypass_button_.getBounds(), bypass_button_.getToggleState() ? .40f : .92f);

        if (!ftype_box_.getBounds().isEmpty()) {
            auto r = ftype_box_.getBounds().toFloat().reduced(ftype_box_.getHeight() * .24f);
            const auto y = r.getCentreY();
            juce::Path glyph;
            switch (f_idx) {
            case 0:
                glyph.startNewSubPath(r.getX(), y + r.getHeight() * .20f);
                glyph.cubicTo(r.getX() + r.getWidth() * .28f, y + r.getHeight() * .20f,
                              r.getX() + r.getWidth() * .32f, r.getY(), r.getCentreX(), r.getY());
                glyph.cubicTo(r.getX() + r.getWidth() * .68f, r.getY(),
                              r.getX() + r.getWidth() * .72f, y + r.getHeight() * .20f,
                              r.getRight(), y + r.getHeight() * .20f); break;
            case 1:
                glyph.startNewSubPath(r.getX(), r.getY() + r.getHeight() * .20f);
                glyph.lineTo(r.getX() + r.getWidth() * .30f, r.getY() + r.getHeight() * .20f);
                glyph.cubicTo(r.getCentreX(), r.getY() + r.getHeight() * .20f,
                              r.getCentreX(), r.getBottom() - r.getHeight() * .20f,
                              r.getRight(), r.getBottom() - r.getHeight() * .20f); break;
            case 2:
                glyph.startNewSubPath(r.getX(), r.getY() + r.getHeight() * .20f);
                glyph.lineTo(r.getX() + r.getWidth() * .40f, r.getY() + r.getHeight() * .20f);
                glyph.cubicTo(r.getCentreX(), r.getY() + r.getHeight() * .20f,
                              r.getCentreX(), r.getBottom() - r.getHeight() * .12f,
                              r.getRight(), r.getBottom() - r.getHeight() * .08f); break;
            case 3:
                glyph.startNewSubPath(r.getX(), r.getBottom() - r.getHeight() * .20f);
                glyph.lineTo(r.getX() + r.getWidth() * .30f, r.getBottom() - r.getHeight() * .20f);
                glyph.cubicTo(r.getCentreX(), r.getBottom() - r.getHeight() * .20f,
                              r.getCentreX(), r.getY() + r.getHeight() * .20f,
                              r.getRight(), r.getY() + r.getHeight() * .20f); break;
            case 4:
                glyph.startNewSubPath(r.getX(), r.getBottom() - r.getHeight() * .08f);
                glyph.cubicTo(r.getCentreX(), r.getBottom() - r.getHeight() * .12f,
                              r.getCentreX(), r.getY() + r.getHeight() * .20f,
                              r.getX() + r.getWidth() * .60f, r.getY() + r.getHeight() * .20f);
                glyph.lineTo(r.getRight(), r.getY() + r.getHeight() * .20f); break;
            case 5:
                glyph.startNewSubPath(r.getX(), y - r.getHeight() * .14f);
                glyph.cubicTo(r.getX() + r.getWidth() * .38f, y - r.getHeight() * .14f,
                              r.getX() + r.getWidth() * .42f, r.getBottom(), r.getCentreX(), r.getBottom());
                glyph.cubicTo(r.getX() + r.getWidth() * .58f, r.getBottom(),
                              r.getX() + r.getWidth() * .62f, y - r.getHeight() * .14f, r.getRight(), y - r.getHeight() * .14f); break;
            case 6:
                glyph.startNewSubPath(r.getX(), r.getBottom());
                glyph.cubicTo(r.getX() + r.getWidth() * .30f, r.getBottom(),
                              r.getX() + r.getWidth() * .34f, r.getY(), r.getCentreX(), r.getY());
                glyph.cubicTo(r.getX() + r.getWidth() * .66f, r.getY(),
                              r.getX() + r.getWidth() * .70f, r.getBottom(), r.getRight(), r.getBottom()); break;
            case 7: glyph.startNewSubPath(r.getX(), r.getBottom()); glyph.lineTo(r.getRight(), r.getY()); break;
            case 8: glyph.startNewSubPath(r.getX(), r.getBottom() - r.getHeight() * .16f); glyph.lineTo(r.getRight(), r.getY() + r.getHeight() * .16f); break;
            case 9:
                glyph.startNewSubPath(r.getX(), y); glyph.lineTo(r.getX() + r.getWidth() * .30f, y);
                glyph.lineTo(r.getX() + r.getWidth() * .43f, r.getY()); glyph.lineTo(r.getX() + r.getWidth() * .57f, r.getBottom());
                glyph.lineTo(r.getX() + r.getWidth() * .70f, y); glyph.lineTo(r.getRight(), y); break;
            default: glyph.startNewSubPath(r.getX(), y); glyph.lineTo(r.getRight(), y); break;
            }
            g.setColour(text.withAlpha(.88f));
            g.strokePath(glyph, juce::PathStrokeType(1.22f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        if (!filter_name_bound_.isEmpty()) {
            auto pill = ftype_box_.getBounds().getUnion(filter_name_bound_).toFloat().expanded(1.f, .5f);
            zlgui::glass::fillGlassSurface(g, pill, pill.getHeight() * .5f, .07f, .11f, .12f);
            g.setColour(text.withAlpha(.92f));
            g.setFont(juce::FontOptions(base_.getFontSize() * .82f));
            if (f_idx >= 0 && f_idx < static_cast<int>(filter_names.size()))
                g.drawText(filter_names[static_cast<size_t>(f_idx)], filter_name_bound_, juce::Justification::centredLeft, false);
        }

        if (hub_open_) {
            auto line = getLocalBounds().toFloat().reduced(base_.getFontSize() * .80f);
            line.setHeight(1.2f);
            juce::ColourGradient accentLine(accent.withAlpha(.0f), line.getX(), line.getY(),
                                            accent.interpolatedWith(juce::Colours::white, .30f).withAlpha(.52f),
                                            line.getCentreX(), line.getY(), false);
            accentLine.addColour(.86, accent.withAlpha(.12f));
            g.setGradientFill(accentLine);
            g.fillRoundedRectangle(line, .6f);

            g.setColour(text.withAlpha(.94f));
            g.setFont(juce::FontOptions(base_.getFontSize() * .92f));
            juce::String title = "Band " + juce::String(static_cast<int>(band + 1));
            if (f_idx >= 0 && f_idx < static_cast<int>(filter_names.size())) title += "  ·  " + juce::String(filter_names[static_cast<size_t>(f_idx)]);
            g.drawText(title, hub_title_bound_, juce::Justification::centredLeft, false);

            if (lr_idx >= 0 && lr_idx < static_cast<int>(mode_names.size()) && !mode_name_bound_.isEmpty()) {
                auto pill = mode_name_bound_.toFloat().reduced(.5f);
                zlgui::glass::fillGlassSurface(g, pill, pill.getHeight() * .5f, .055f, .10f, .11f);
                g.setColour(quiet.withAlpha(.90f));
                g.setFont(juce::FontOptions(base_.getFontSize() * .66f));
                g.drawText(mode_names[static_cast<size_t>(lr_idx)], mode_name_bound_, juce::Justification::centred, false);
            }

            if (!dynamic_button_.getBounds().isEmpty()) {
                auto r = dynamic_button_.getBounds().toFloat().reduced(dynamic_button_.getHeight() * .27f);
                juce::Path wave;
                wave.startNewSubPath(r.getX(), r.getCentreY());
                wave.cubicTo(r.getX() + r.getWidth() * .22f, r.getY(), r.getX() + r.getWidth() * .28f, r.getBottom(), r.getCentreX(), r.getCentreY());
                wave.cubicTo(r.getX() + r.getWidth() * .72f, r.getY(), r.getX() + r.getWidth() * .78f, r.getBottom(), r.getRight(), r.getCentreY());
                g.setColour((dynamic_on_ ? accent.interpolatedWith(juce::Colours::white, .25f) : text).withAlpha(dynamic_on_ ? .96f : .46f));
                g.strokePath(wave, juce::PathStrokeType(1.45f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }

            if (!solo_button_.getBounds().isEmpty()) {
                auto r = solo_button_.getBounds().toFloat().reduced(solo_button_.getHeight() * .30f);
                g.setColour(text.withAlpha(solo_button_.getButton().getToggleState() ? .92f : .42f));
                juce::Path hp;
                hp.addCentredArc(r.getCentreX(), r.getCentreY() + r.getHeight() * .08f,
                                 r.getWidth() * .40f, r.getHeight() * .42f, 0.f, 3.48f, 5.94f, true);
                g.strokePath(hp, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }

            if (!close_button_.getBounds().isEmpty()) {
                auto r = close_button_.getBounds().toFloat().reduced(close_button_.getHeight() * .32f);
                g.setColour(text.withAlpha(.36f));
                g.drawRoundedRectangle(r.getX() + r.getWidth() * .20f, r.getY() + r.getHeight() * .27f,
                                       r.getWidth() * .60f, r.getHeight() * .60f, 1.5f, 1.f);
                g.drawLine(r.getX() + r.getWidth() * .14f, r.getY() + r.getHeight() * .22f,
                           r.getRight() - r.getWidth() * .14f, r.getY() + r.getHeight() * .22f, 1.f);
            }

            if (dynamic_on_) {
                g.setColour(accent.interpolatedWith(juce::Colours::white, .20f).withAlpha(.88f));
                g.setFont(juce::FontOptions(base_.getFontSize() * .68f));
                g.drawText(detail_page_ == DetailPage::dynamics ? "Dynamic EQ" :
                           detail_page_ == DetailPage::detector ? "Dynamic EQ  ·  Detector" : "Dynamic EQ  ·  Sidechain",
                           dynamic_title_bound_, juce::Justification::centredLeft, false);

                static constexpr std::array<const char*, 4> primary{"THRESH", "RANGE", "ATTACK", "RELEASE"};
                g.setColour(quiet.withAlpha(.76f));
                g.setFont(juce::FontOptions(base_.getFontSize() * .54f));
                for (size_t i = 0; i < primary.size(); ++i)
                    g.drawText(primary[i], primary_dynamic_label_bounds_[i], juce::Justification::centredBottom, false);

                if (detail_page_ != DetailPage::dynamics) {
                    static constexpr std::array<const char*, 4> detector{"KNEE", "LENGTH", "MIX", "SMOOTH"};
                    static constexpr std::array<const char*, 4> side{"FILTER", "SLOPE", "FREQ", "Q"};
                    const auto& labels = detail_page_ == DetailPage::detector ? detector : side;
                    g.setColour(quiet.withAlpha(.70f));
                    for (size_t i = 0; i < labels.size(); ++i)
                        g.drawText(labels[i], advanced_dynamic_label_bounds_[i], juce::Justification::centredBottom, false);
                }
            } else {
                g.setColour(quiet.withAlpha(.72f));
                g.setFont(juce::FontOptions(base_.getFontSize() * .62f));
                g.drawText("Enable Dynamic EQ for range, threshold, detector and sidechain controls.",
                           dynamic_title_bound_, juce::Justification::centredLeft, false);
            }
        }
    }

    void FloatPopPanel::resized() {
        control_background_.setBounds(getLocalBounds());
        const auto padding = getPaddingSize(base_.getFontSize());
        const auto button = getButtonSize(base_.getFontSize());
        auto bound = getLocalBounds().reduced(padding + padding / 2, padding + padding / 2);

        filter_name_bound_ = mode_name_bound_ = hub_title_bound_ = dynamic_title_bound_ = advanced_section_title_bound_ = {};
        for (auto& r : primary_dynamic_label_bounds_) r = {};
        for (auto& r : advanced_dynamic_label_bounds_) r = {};

        if (!hub_open_) {
            auto header = bound.removeFromTop(button);
            ftype_box_.setBounds(header.removeFromLeft(button));
            filter_name_bound_ = header.removeFromLeft(juce::jmax(button * 2, juce::roundToInt(base_.getFontSize() * 4.0f)));
            more_button_.setBounds(header.removeFromRight(button));
            header.removeFromRight(juce::jmax(1, padding / 6));
            bypass_button_.setBounds(header.removeFromRight(button));

            bound.removeFromTop(juce::jmax(2, padding / 3));
            auto values = bound.removeFromTop(button);
            const auto gap = juce::jmax(2, padding / 3);
            const auto cell = (values.getWidth() - 3 * gap) / 4;
            freq_slider_.setBounds(values.removeFromLeft(cell)); values.removeFromLeft(gap);
            gain_slider_.setBounds(values.removeFromLeft(cell)); values.removeFromLeft(gap);
            q_slider_.setBounds(values.removeFromLeft(cell)); values.removeFromLeft(gap);
            slope_box_.setBounds(values);
            slope_box_.setVisible(slope_supported_);

            lr_box_.setBounds({});
            dynamic_button_.setBounds({});
            solo_button_.setBounds({});
            close_button_.setBounds({});
            detector_page_button_.setBounds({});
            sidechain_page_button_.setBounds({});
            more_button_.getButton().setButtonText("...");
        } else {
            auto header = bound.removeFromTop(button);
            ftype_box_.setBounds(header.removeFromLeft(button));
            filter_name_bound_ = header.removeFromLeft(juce::jmax(button * 2, juce::roundToInt(base_.getFontSize() * 4.0f)));
            hub_title_bound_ = header;
            more_button_.setBounds(header.removeFromRight(juce::jmax(button * 2, juce::roundToInt(base_.getFontSize() * 3.3f))));
            more_button_.getButton().setButtonText("Done");
            header.removeFromRight(juce::jmax(2, padding / 3));
            bypass_button_.setBounds(header.removeFromRight(button));

            bound.removeFromTop(juce::jmax(2, padding / 3));
            auto values = bound.removeFromTop(button);
            const auto gap = juce::jmax(2, padding / 3);
            const auto cell = (values.getWidth() - 4 * gap) / 5;
            freq_slider_.setBounds(values.removeFromLeft(cell)); values.removeFromLeft(gap);
            gain_slider_.setBounds(values.removeFromLeft(cell)); values.removeFromLeft(gap);
            q_slider_.setBounds(values.removeFromLeft(cell)); values.removeFromLeft(gap);
            slope_box_.setBounds(values.removeFromLeft(cell)); values.removeFromLeft(gap);
            mode_name_bound_ = values;
            lr_box_.setBounds(mode_name_bound_);
            slope_box_.setVisible(slope_supported_);
            lr_box_.setVisible(true);

            bound.removeFromTop(juce::jmax(2, padding / 3));
            auto actions = bound.removeFromTop(button);
            dynamic_button_.setBounds(actions.removeFromLeft(button));
            dynamic_button_.setVisible(true);
            dynamic_title_bound_ = actions.removeFromLeft(juce::jmax(button * 3, juce::roundToInt(base_.getFontSize() * 8.f)));
            close_button_.setBounds(actions.removeFromRight(button));
            actions.removeFromRight(juce::jmax(1, padding / 5));
            solo_button_.setBounds(actions.removeFromRight(button));

            if (dynamic_on_) {
                detector_page_button_.setBounds(actions.removeFromRight(juce::jmax(button * 2, juce::roundToInt(base_.getFontSize() * 3.6f))));
                actions.removeFromRight(gap);
                sidechain_page_button_.setBounds(actions.removeFromRight(juce::jmax(button * 2, juce::roundToInt(base_.getFontSize() * 4.4f))));

                bound.removeFromTop(juce::jmax(2, padding / 2));
                const auto label_h = juce::jmax(8, juce::roundToInt(base_.getFontSize() * .62f));
                auto labels = bound.removeFromTop(label_h);
                auto row = bound.removeFromTop(button);
                const auto dynCell = (row.getWidth() - 3 * gap) / 4;
                for (size_t i = 0; i < 4; ++i) {
                    primary_dynamic_label_bounds_[i] = labels.removeFromLeft(dynCell);
                    if (i < 3) labels.removeFromLeft(gap);
                }
                threshold_slider_.setBounds(row.removeFromLeft(dynCell)); row.removeFromLeft(gap);
                range_slider_.setBounds(row.removeFromLeft(dynCell)); row.removeFromLeft(gap);
                attack_slider_.setBounds(row.removeFromLeft(dynCell)); row.removeFromLeft(gap);
                release_slider_.setBounds(row);

                if (detail_page_ != DetailPage::dynamics) {
                    bound.removeFromTop(juce::jmax(2, padding / 2));
                    auto detailLabels = bound.removeFromTop(label_h);
                    auto detailRow = bound.removeFromTop(button);
                    const auto detailCell = (detailRow.getWidth() - 3 * gap) / 4;
                    for (size_t i = 0; i < 4; ++i) {
                        advanced_dynamic_label_bounds_[i] = detailLabels.removeFromLeft(detailCell);
                        if (i < 3) detailLabels.removeFromLeft(gap);
                    }
                    if (detail_page_ == DetailPage::detector) {
                        knee_slider_.setBounds(detailRow.removeFromLeft(detailCell)); detailRow.removeFromLeft(gap);
                        rms_length_slider_.setBounds(detailRow.removeFromLeft(detailCell)); detailRow.removeFromLeft(gap);
                        rms_mix_slider_.setBounds(detailRow.removeFromLeft(detailCell)); detailRow.removeFromLeft(gap);
                        smooth_slider_.setBounds(detailRow);
                        bound.removeFromTop(juce::jmax(2, padding / 3));
                        auto flags = bound.removeFromTop(juce::jmax(button * 3 / 4, juce::roundToInt(base_.getFontSize() * 1.15f)));
                        const auto fw = (flags.getWidth() - 2 * gap) / 3;
                        learn_button_.setBounds(flags.removeFromLeft(fw)); flags.removeFromLeft(gap);
                        relative_button_.setBounds(flags.removeFromLeft(fw)); flags.removeFromLeft(gap);
                        dyn_bypass_button_.setBounds(flags);
                    } else {
                        side_type_box_.setBounds(detailRow.removeFromLeft(detailCell)); detailRow.removeFromLeft(gap);
                        side_order_box_.setBounds(detailRow.removeFromLeft(detailCell)); detailRow.removeFromLeft(gap);
                        side_freq_slider_.setBounds(detailRow.removeFromLeft(detailCell)); detailRow.removeFromLeft(gap);
                        side_q_slider_.setBounds(detailRow);
                        bound.removeFromTop(juce::jmax(2, padding / 3));
                        auto flags = bound.removeFromTop(juce::jmax(button * 3 / 4, juce::roundToInt(base_.getFontSize() * 1.15f)));
                        const auto fw = (flags.getWidth() - gap) / 2;
                        side_link_button_.setBounds(flags.removeFromLeft(fw)); flags.removeFromLeft(gap);
                        side_swap_button_.setBounds(flags);
                    }
                }
            } else {
                detector_page_button_.setBounds({});
                sidechain_page_button_.setBounds({});
            }
        }

        updateDetailVisibility();
        ideal_height_ = static_cast<float>(getIdealHeight());
        ideal_width_ = static_cast<float>(getIdealWidth());
    }

    void FloatPopPanel::mouseUp(const juce::MouseEvent& event) {
        if (filter_name_bound_.contains(event.getPosition())) ftype_box_.getBox().showPopup();
    }

    void FloatPopPanel::setHubOpen(const bool open) {
        if (hub_open_ == open) return;
        hub_open_ = open;
        detail_page_ = DetailPage::dynamics;
        updateDetailVisibility();
        if (auto* parent = getParentComponent()) parent->resized();
        resized();
        updateTransformation();
        repaint();
    }

    void FloatPopPanel::updateBand() {
        if (base_.getSelectedBand() < zlp::kBandNum) {
            const auto band_s = std::to_string(base_.getSelectedBand());
            ftype_attachment_ = std::make_unique<zlgui::attachment::ComboBoxAttachment<true>>(ftype_box_.getBox(), p_ref_.parameters_, zlp::PFilterType::kID + band_s, updater_);
            lr_attachment_ = std::make_unique<zlgui::attachment::ComboBoxAttachment<true>>(lr_box_.getBox(), p_ref_.parameters_, zlp::PLRMode::kID + band_s, updater_);
            slope_attachment_ = std::make_unique<zlgui::attachment::ComboBoxAttachment<true>>(slope_box_.getBox(), p_ref_.parameters_, zlp::POrder::kID + band_s, updater_);
            freq_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(freq_slider_.getSlider(), p_ref_.parameters_, zlp::PFreq::kID + band_s, updater_);
            gain_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(gain_slider_.getSlider(), p_ref_.parameters_, zlp::PGain::kID + band_s, updater_);
            q_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(q_slider_.getSlider(), p_ref_.parameters_, zlp::PQ::kID + band_s, updater_);
            dynamic_attachment_ = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(dynamic_button_.getButton(), p_ref_.parameters_, zlp::PDynamicON::kID + band_s, updater_, juce::dontSendNotification);
            range_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(range_slider_.getSlider(), p_ref_.parameters_, zlp::PTargetGain::kID + band_s, updater_);
            threshold_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(threshold_slider_.getSlider(), p_ref_.parameters_, zlp::PThreshold::kID + band_s, updater_);
            attack_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(attack_slider_.getSlider(), p_ref_.parameters_, zlp::PAttack::kID + band_s, updater_);
            release_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(release_slider_.getSlider(), p_ref_.parameters_, zlp::PRelease::kID + band_s, updater_);
            knee_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(knee_slider_.getSlider(), p_ref_.parameters_, zlp::PKneeW::kID + band_s, updater_);
            rms_length_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(rms_length_slider_.getSlider(), p_ref_.parameters_, zlp::PDynamicRMSLength::kID + band_s, updater_);
            rms_mix_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(rms_mix_slider_.getSlider(), p_ref_.parameters_, zlp::PDynamicRMSMix::kID + band_s, updater_);
            smooth_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(smooth_slider_.getSlider(), p_ref_.parameters_, zlp::PDynamicSmooth::kID + band_s, updater_);
            learn_attachment_ = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(learn_button_.getButton(), p_ref_.parameters_, zlp::PDynamicLearn::kID + band_s, updater_, juce::dontSendNotification);
            relative_attachment_ = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(relative_button_.getButton(), p_ref_.parameters_, zlp::PDynamicRelative::kID + band_s, updater_, juce::dontSendNotification);
            dyn_bypass_attachment_ = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(dyn_bypass_button_.getButton(), p_ref_.parameters_, zlp::PDynamicBypass::kID + band_s, updater_, juce::dontSendNotification);
            side_type_attachment_ = std::make_unique<zlgui::attachment::ComboBoxAttachment<true>>(side_type_box_.getBox(), p_ref_.parameters_, zlp::PSideFilterType::kID + band_s, updater_);
            side_order_attachment_ = std::make_unique<zlgui::attachment::ComboBoxAttachment<true>>(side_order_box_.getBox(), p_ref_.parameters_, zlp::PSideOrder::kID + band_s, updater_);
            side_freq_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(side_freq_slider_.getSlider(), p_ref_.parameters_, zlp::PSideFreq::kID + band_s, updater_);
            side_q_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(side_q_slider_.getSlider(), p_ref_.parameters_, zlp::PSideQ::kID + band_s, updater_);
            side_link_attachment_ = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(side_link_button_.getButton(), p_ref_.parameters_, zlp::PSideLink::kID + band_s, updater_, juce::dontSendNotification);
            side_swap_attachment_ = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(side_swap_button_.getButton(), p_ref_.parameters_, zlp::PSideSwap::kID + band_s, updater_, juce::dontSendNotification);

            updater_.updateComponents();
            filter_status_ptr_ = p_ref_.parameters_.getRawParameterValue(zlp::PFilterStatus::kID + band_s);
            dynamic_on_ptr_ = p_ref_.parameters_.getRawParameterValue(zlp::PDynamicON::kID + band_s);
            filter_type_ptr_ = p_ref_.parameters_.getRawParameterValue(zlp::PFilterType::kID + band_s);
            slope_ptr_ = p_ref_.parameters_.getRawParameterValue(zlp::POrder::kID + band_s);
            current_filter_type_ = current_slope_ = -1;
            updateFilterCapabilities();
            updateDynamicVisibility(dynamic_on_ptr_ != nullptr && dynamic_on_ptr_->load(std::memory_order::relaxed) > .5f, true);
            repaintCallBackSlow();
        } else {
            ftype_attachment_.reset(); lr_attachment_.reset(); slope_attachment_.reset();
            freq_attachment_.reset(); gain_attachment_.reset(); q_attachment_.reset(); dynamic_attachment_.reset();
            range_attachment_.reset(); threshold_attachment_.reset(); attack_attachment_.reset(); release_attachment_.reset();
            knee_attachment_.reset(); rms_length_attachment_.reset(); rms_mix_attachment_.reset(); smooth_attachment_.reset();
            learn_attachment_.reset(); relative_attachment_.reset(); dyn_bypass_attachment_.reset();
            side_type_attachment_.reset(); side_order_attachment_.reset(); side_freq_attachment_.reset(); side_q_attachment_.reset();
            side_link_attachment_.reset(); side_swap_attachment_.reset();
            filter_status_ptr_ = dynamic_on_ptr_ = filter_type_ptr_ = slope_ptr_ = nullptr;
            current_filter_type_ = current_slope_ = -1;
            hub_open_ = false;
            updateDynamicVisibility(false, true);
        }
        setVisible(base_.getSelectedBand() < zlp::kBandNum);
    }

    void FloatPopPanel::repaintCallBackSlow() {
        if (filter_status_ptr_ == nullptr) return;
        updater_.updateComponents();
        for (auto* s : {&freq_slider_, &gain_slider_, &q_slider_, &range_slider_, &threshold_slider_, &attack_slider_,
                        &release_slider_, &knee_slider_, &rms_length_slider_, &rms_mix_slider_, &smooth_slider_,
                        &side_freq_slider_, &side_q_slider_}) s->updateDisplayValue();
        const auto filter_on = filter_status_ptr_->load(std::memory_order::relaxed) > 1.5f;
        if (filter_on != bypass_button_.getToggleState())
            bypass_button_.getButton().setToggleState(filter_on, juce::dontSendNotification);
        if (dynamic_on_ptr_ != nullptr) {
            const auto next = dynamic_on_ptr_->load(std::memory_order::relaxed) > .5f;
            if (next != dynamic_on_) updateDynamicVisibility(next, true);
        }
        updateFilterCapabilities();
        repaint();
    }

    void FloatPopPanel::updateFilterCapabilities() {
        if (filter_type_ptr_ == nullptr || slope_ptr_ == nullptr) return;
        const auto filter_type = static_cast<int>(std::round(filter_type_ptr_->load(std::memory_order::relaxed)));
        const auto slope = static_cast<int>(std::round(slope_ptr_->load(std::memory_order::relaxed)));
        if (filter_type == current_filter_type_ && slope == current_slope_) return;
        current_filter_type_ = filter_type;
        current_slope_ = slope;

        slope_supported_ = filter_type != static_cast<int>(zldsp::filter::kFlatTilt)
            && filter_type != static_cast<int>(zldsp::filter::kFlatGain);
        const auto slope6 = filter_type != static_cast<int>(zldsp::filter::kPeak)
            && filter_type != static_cast<int>(zldsp::filter::kBandPass)
            && filter_type != static_cast<int>(zldsp::filter::kNotch);
        if (!slope6 && slope_box_.getBox().getSelectedId() == 1)
            slope_box_.getBox().setSelectedId(2, juce::sendNotificationSync);
        slope_box_.getBox().setItemEnabled(1, slope6);
        slope_box_.setEditable(slope_supported_);

        const auto gain_enabled = filter_type == static_cast<int>(zldsp::filter::kPeak)
            || filter_type == static_cast<int>(zldsp::filter::kLowShelf)
            || filter_type == static_cast<int>(zldsp::filter::kHighShelf)
            || filter_type == static_cast<int>(zldsp::filter::kTiltShelf)
            || filter_type == static_cast<int>(zldsp::filter::kFlatTilt)
            || filter_type == static_cast<int>(zldsp::filter::kFlatGain);
        if (!gain_enabled && dynamic_button_.getToggleState())
            dynamic_button_.getButton().setToggleState(false, juce::sendNotificationSync);
        gain_slider_.setEditable(gain_enabled);
        dynamic_button_.setAlpha(gain_enabled ? 1.f : .38f);
        dynamic_button_.setInterceptsMouseClicks(false, gain_enabled);
        q_slider_.setEditable(slope_supported_ && slope != 0);
        resized();
        repaint();
    }

    int FloatPopPanel::getIdealWidth() const {
        const auto p = getPaddingSize(base_.getFontSize());
        const auto b = getButtonSize(base_.getFontSize());
        return hub_open_ ? juce::roundToInt(13.0f * static_cast<float>(b) + 6.0f * static_cast<float>(p))
                         : juce::roundToInt(8.0f * static_cast<float>(b) + 4.0f * static_cast<float>(p));
    }

    int FloatPopPanel::getIdealHeight() const {
        const auto p = getPaddingSize(base_.getFontSize());
        const auto b = getButtonSize(base_.getFontSize());
        if (!hub_open_) return 2 * b + 3 * p;
        auto h = 3 * b + 4 * p;
        if (dynamic_on_) {
            const auto label = juce::jmax(8, juce::roundToInt(base_.getFontSize() * .62f));
            h += label + b + 2 * p;
            if (detail_page_ != DetailPage::dynamics)
                h += label + b + juce::jmax(b * 3 / 4, juce::roundToInt(base_.getFontSize() * 1.15f)) + 2 * p;
        }
        return h;
    }

    void FloatPopPanel::styleDetailButton(zlgui::button::ClickTextButton& button) {
        button.getLAF().setFontScale(.66f);
        button.setBackgroundPainter([this](juce::Graphics& g, juce::Button& b, bool highlighted, bool down) {
            auto r = b.getLocalBounds().toFloat().reduced(.5f);
            const auto active = b.getToggleState() || down;
            if (active || highlighted)
                zlgui::glass::fillGlassSurface(g, r, r.getHeight() * .5f, active ? .10f : .04f,
                                               active ? .15f : .07f, active ? .16f : .08f);
        });
    }

    void FloatPopPanel::updateDetailPage(const DetailPage page, const bool request_parent_resize) {
        detail_page_ = page;
        detector_page_button_.getButton().setToggleState(page == DetailPage::detector, juce::dontSendNotification);
        sidechain_page_button_.getButton().setToggleState(page == DetailPage::sidechain, juce::dontSendNotification);
        updateDetailVisibility();
        if (request_parent_resize) if (auto* parent = getParentComponent()) parent->resized();
        resized();
        updateTransformation();
        repaint();
    }

    void FloatPopPanel::updateDetailVisibility() {
        const auto in_hub = hub_open_;
        ftype_box_.setVisible(true);
        bypass_button_.setVisible(true);
        more_button_.setVisible(true);
        freq_slider_.setVisible(true);
        gain_slider_.setVisible(true);
        q_slider_.setVisible(true);
        slope_box_.setVisible(slope_supported_);
        lr_box_.setVisible(in_hub);
        dynamic_button_.setVisible(in_hub);
        solo_button_.setVisible(in_hub);
        close_button_.setVisible(in_hub);

        const auto showDyn = in_hub && dynamic_on_;
        detector_page_button_.setVisible(showDyn);
        sidechain_page_button_.setVisible(showDyn);
        const std::array<juce::Component*, 4> primary{&range_slider_, &threshold_slider_, &attack_slider_, &release_slider_};
        for (auto* c : primary) c->setVisible(showDyn);

        const auto detector = showDyn && detail_page_ == DetailPage::detector;
        for (auto* c : std::array<juce::Component*, 7>{&knee_slider_, &rms_length_slider_, &rms_mix_slider_, &smooth_slider_,
                                                        &learn_button_, &relative_button_, &dyn_bypass_button_}) c->setVisible(detector);
        const auto side = showDyn && detail_page_ == DetailPage::sidechain;
        for (auto* c : std::array<juce::Component*, 6>{&side_type_box_, &side_order_box_, &side_freq_slider_, &side_q_slider_,
                                                        &side_link_button_, &side_swap_button_}) c->setVisible(side);
    }

    void FloatPopPanel::updateDynamicVisibility(const bool dynamic_on, const bool request_parent_resize) {
        dynamic_on_ = dynamic_on;
        if (!dynamic_on_) detail_page_ = DetailPage::dynamics;
        updateDetailVisibility();
        if (request_parent_resize) if (auto* parent = getParentComponent()) parent->resized();
        resized();
        updateTransformation();
    }

    void FloatPopPanel::setTargetVisible(const bool is_target_visible) {
        is_target_visible_ = is_target_visible;
        updateTransformation();
    }

    void FloatPopPanel::updatePosition(const juce::Point<float> position,
                                       const juce::Point<float> target_position) {
        if (std::abs(position.x - position_.x) > .01f || std::abs(position.y - position_.y) > .01f
            || std::abs(target_position.x - target_position_.x) > .01f || std::abs(target_position.y - target_position_.y) > .01f) {
            position_ = position;
            target_position_ = target_position;
            updateTransformation();
        }
    }

    void FloatPopPanel::updateFloatingBound(const juce::Rectangle<float> bound) {
        safe_bound_ = bound.reduced(juce::jmax(6.f, base_.getFontSize() * .55f));
        updateTransformation();
    }

    void FloatPopPanel::updateTransformation() {
        if (safe_bound_.isEmpty() || ideal_width_ <= 0.f || ideal_height_ <= 0.f) return;
        const auto gap = juce::jmax(8.f, base_.getFontSize() * .80f);
        const auto minX = safe_bound_.getX();
        const auto maxX = juce::jmax(minX, safe_bound_.getRight() - ideal_width_);

        if (hub_open_) {
            const auto x = std::clamp(safe_bound_.getCentreX() - ideal_width_ * .5f, minX, maxX);
            const auto y = juce::jmax(safe_bound_.getY(), safe_bound_.getBottom() - ideal_height_);
            setTransform(juce::AffineTransform::translation(x, y));
            return;
        }

        const auto aboveY = position_.y - gap - ideal_height_;
        const auto belowY = position_.y + gap;
        const auto fitsAbove = aboveY >= safe_bound_.getY();
        const auto fitsBelow = belowY + ideal_height_ <= safe_bound_.getBottom();
        const auto centre = safe_bound_.getCentreY();
        const auto hysteresis = juce::jmax(18.f, base_.getFontSize() * 2.0f);

        if (!bubble_side_initialized_) {
            bubble_below_ = !fitsAbove && fitsBelow ? true : fitsAbove && !fitsBelow ? false : position_.y < centre;
            bubble_side_initialized_ = true;
        } else if (bubble_below_) {
            if ((!fitsBelow && fitsAbove) || (fitsAbove && position_.y > centre + hysteresis)) bubble_below_ = false;
        } else {
            if ((!fitsAbove && fitsBelow) || (fitsBelow && position_.y < centre - hysteresis)) bubble_below_ = true;
        }

        auto y = bubble_below_ ? belowY : aboveY;
        y = std::clamp(y, safe_bound_.getY(), juce::jmax(safe_bound_.getY(), safe_bound_.getBottom() - ideal_height_));
        const auto x = std::clamp(position_.x - ideal_width_ * .5f, minX, maxX);
        setTransform(juce::AffineTransform::translation(x, y));
    }

    void FloatPopPanel::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) {
        const auto solo = base_.getSoloWholeIdx();
        solo_button_.getButton().setToggleState(solo < zlp::kBandNum, juce::dontSendNotification);
    }
}
