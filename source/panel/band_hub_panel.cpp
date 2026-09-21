// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer

#include "band_hub_panel.hpp"
#include "../gui/glass_tokens.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

namespace zlpanel {
    namespace {
        constexpr std::array<const char*, 11> kFilterNames{
            "Bell", "Low Shelf", "High Cut", "High Shelf", "Low Cut", "Notch",
            "Band Pass", "Tilt", "Flat Tilt", "All Pass", "Gain"
        };

        juce::String formatFrequency(const double value) {
            if (value >= 1000.0) return juce::String(value * .001, 2) + " kHz";
            return juce::String(value, value >= 100.0 ? 0 : 1) + " Hz";
        }
    }

    BandHubPanel::BandHubPanel(PluginProcessor& p, zlgui::UIBase& base) :
        p_ref_(p), base_(base), updater_(),
        channel_box_(juce::StringArray{"Stereo", "Left", "Right", "Mid", "Side"}, base, "Band channel"),
        dynamic_toggle_(base, "Dynamic"),
        expand_button_(base, "Open"),
        dynamics_page_button_(base, "Dynamics"),
        detector_page_button_(base, "Detector"),
        sidechain_page_button_(base, "Sidechain"),
        actions_page_button_(base, "Actions"),
        threshold_slider_("", base), range_slider_("", base), attack_slider_("", base), release_slider_("", base),
        knee_slider_("", base), rms_length_slider_("", base), rms_mix_slider_("", base), smooth_slider_("", base),
        learn_button_(base, "Learn"), relative_button_(base, "Relative"), dynamic_bypass_button_(base, "Dyn Bypass"),
        side_type_box_(zlp::PSideFilterType::kChoices, base, "Sidechain filter"),
        side_order_box_(zlp::PSideOrder::kChoices, base, "Sidechain slope"),
        side_freq_slider_("", base), side_q_slider_("", base),
        side_link_button_(base, "Link"), side_swap_button_(base, "Swap"), external_button_(base, "External"),
        external_attach_(external_button_.getButton(), p.parameters_, zlp::PExtSide::kID, updater_),
        solo_button_(base, "Solo"), invert_button_(base, "Invert Gain"),
        split_lr_button_(base, "Split L/R"), split_ms_button_(base, "Split M/S"),
        copy_button_(base, "Copy"), paste_button_(base, "Paste"), delete_button_(base, "Delete") {
        setOpaque(false);
        setInterceptsMouseClicks(false, true);

        channel_box_.setScrollEnabled(true);
        channel_box_.getLAF().setFontScale(.70f);
        channel_box_.getLAF().setBoxAlpha(.60f);
        channel_box_.getLAF().setLabelJustification(juce::Justification::centred);
        addAndMakeVisible(channel_box_);

        stylePill(dynamic_toggle_);
        dynamic_toggle_.getButton().setToggleable(true);
        dynamic_toggle_.getButton().setClickingTogglesState(true);
        dynamic_toggle_.getButton().onClick = [this]() {
            if (band_ >= zlp::kBandNum) return;
            const auto max_idx = std::round(p_ref_.parameters_NA_.getRawParameterValue(
                zlstate::PEQMaxDB::kID)->load(std::memory_order::relaxed));
            band_helper::turnOnOffDynamic(p_ref_, band_, dynamic_toggle_.getToggleState(),
                                          base_.getCurveDBScale(static_cast<size_t>(max_idx)));
            if (dynamic_toggle_.getToggleState()) {
                setPage(Page::dynamics);
                setExpanded(true);
            }
        };
        addAndMakeVisible(dynamic_toggle_);

        stylePill(expand_button_, true);
        expand_button_.getButton().onClick = [this]() { setExpanded(!expanded_); };
        addAndMakeVisible(expand_button_);

        for (auto* button : {&dynamics_page_button_, &detector_page_button_,
                             &sidechain_page_button_, &actions_page_button_}) {
            stylePill(*button, true);
            button->getButton().setToggleable(true);
            button->getButton().setClickingTogglesState(false);
            addAndMakeVisible(*button);
        }
        dynamics_page_button_.getButton().onClick = [this]() { setPage(Page::dynamics); };
        detector_page_button_.getButton().onClick = [this]() { setPage(Page::detector); };
        sidechain_page_button_.getButton().onClick = [this]() { setPage(Page::sidechain); };
        actions_page_button_.getButton().onClick = [this]() { setPage(Page::actions); };

        const auto setupDial = [this](auto& slider, const juce::String& title) {
            slider.setGlassRotaryMode(title);
            slider.setFontScale(.72f);
            slider.getSlider().setSliderSnapsToMousePosition(false);
            addAndMakeVisible(slider);
        };
        setupDial(threshold_slider_, "Threshold");
        setupDial(range_slider_, "Range");
        setupDial(attack_slider_, "Attack");
        setupDial(release_slider_, "Release");
        setupDial(knee_slider_, "Knee");
        setupDial(rms_length_slider_, "Length");
        setupDial(rms_mix_slider_, "RMS Mix");
        setupDial(smooth_slider_, "Smooth");

        threshold_slider_.setPrecision(3);
        threshold_slider_.permitted_characters_ = "-0123456789.";
        threshold_slider_.value_formatter_ = [](const double v) {
            char b[32]; std::snprintf(b, sizeof(b), "%.1f dB", v); return std::string{b};
        };
        range_slider_.setPrecision(3);
        range_slider_.permitted_characters_ = "-+0123456789.";
        range_slider_.value_formatter_ = [this](const double target) {
            double gain = 0.0;
            if (band_ < zlp::kBandNum) {
                if (const auto* value = p_ref_.parameters_.getRawParameterValue(zlp::PGain::kID + std::to_string(band_)))
                    gain = value->load(std::memory_order::relaxed);
            }
            char b[32]; std::snprintf(b, sizeof(b), "%+.2f dB", target - gain); return std::string{b};
        };
        range_slider_.string_formatter_ = [this](const std::string& text) -> std::optional<double> {
            double gain = 0.0;
            if (band_ < zlp::kBandNum) {
                if (const auto* value = p_ref_.parameters_.getRawParameterValue(zlp::PGain::kID + std::to_string(band_)))
                    gain = value->load(std::memory_order::relaxed);
            }
            return gain + juce::String(text).getDoubleValue();
        };
        attack_slider_.setPrecision(4);
        attack_slider_.permitted_characters_ = "0123456789.";
        attack_slider_.value_formatter_ = [](const double v) {
            char b[32]; std::snprintf(b, sizeof(b), v < 100.0 ? "%.1f ms" : "%.0f ms", v); return std::string{b};
        };
        release_slider_.setPrecision(4);
        release_slider_.permitted_characters_ = "0123456789.";
        release_slider_.value_formatter_ = [](const double v) {
            char b[32];
            if (v >= 1000.0) std::snprintf(b, sizeof(b), "%.2f s", v * .001);
            else std::snprintf(b, sizeof(b), "%.0f ms", v);
            return std::string{b};
        };
        knee_slider_.setPrecision(3);
        knee_slider_.permitted_characters_ = "0123456789.";
        knee_slider_.value_formatter_ = [](const double v) {
            char b[32]; std::snprintf(b, sizeof(b), "%.1f dB", v); return std::string{b};
        };
        rms_length_slider_.setPrecision(3);
        rms_length_slider_.permitted_characters_ = "0123456789.";
        rms_length_slider_.value_formatter_ = [](const double v) {
            char b[32]; std::snprintf(b, sizeof(b), "%.1f ms", v); return std::string{b};
        };
        rms_mix_slider_.setPrecision(3);
        rms_mix_slider_.permitted_characters_ = "0123456789.";
        rms_mix_slider_.value_formatter_ = [](const double v) {
            char b[32]; std::snprintf(b, sizeof(b), "%.0f%%", v); return std::string{b};
        };
        smooth_slider_.setPrecision(3);
        smooth_slider_.permitted_characters_ = "0123456789.";
        smooth_slider_.value_formatter_ = [](const double v) {
            char b[32]; std::snprintf(b, sizeof(b), "%.0f%%", v); return std::string{b};
        };

        for (auto* button : {&learn_button_, &relative_button_, &dynamic_bypass_button_,
                             &side_link_button_, &side_swap_button_, &external_button_}) {
            stylePill(*button, true);
            button->getButton().setToggleable(true);
            button->getButton().setClickingTogglesState(true);
            addAndMakeVisible(*button);
        }

        for (auto* box : {&side_type_box_, &side_order_box_}) {
            box->setScrollEnabled(true);
            box->getLAF().setFontScale(.72f);
            box->getLAF().setBoxAlpha(.62f);
            box->getLAF().setLabelJustification(juce::Justification::centred);
            addAndMakeVisible(*box);
        }

        const auto setupPlain = [this](auto& slider) {
            slider.setFontScale(.90f);
            slider.getSlider().setSliderSnapsToMousePosition(false);
            addAndMakeVisible(slider);
        };
        setupPlain(side_freq_slider_);
        side_freq_slider_.setPrecision(4);
        side_freq_slider_.permitted_characters_ = "0123456789.kK";
        side_freq_slider_.value_formatter_ = [](const double v) { return formatFrequency(v).toStdString(); };
        setupPlain(side_q_slider_);
        side_q_slider_.setPrecision(3);
        side_q_slider_.permitted_characters_ = "0123456789.";
        side_q_slider_.value_formatter_ = [](const double v) {
            char b[32]; std::snprintf(b, sizeof(b), "Q %.2f", v); return std::string{b};
        };

        for (auto* button : {&solo_button_, &invert_button_, &split_lr_button_, &split_ms_button_,
                             &copy_button_, &paste_button_, &delete_button_}) {
            stylePill(*button, true);
            addAndMakeVisible(*button);
        }
        solo_button_.getButton().setToggleable(true);
        solo_button_.getButton().setClickingTogglesState(true);
        solo_button_.getButton().onClick = [this]() {
            if (band_ >= zlp::kBandNum) return;
            base_.setSoloWholeIdx(solo_button_.getToggleState() ? band_ : 2 * zlp::kBandNum);
        };
        invert_button_.getButton().onClick = [this]() { invertGain(); };
        split_lr_button_.getButton().onClick = [this]() { splitBand(zlp::FilterStereo::kLeft, zlp::FilterStereo::kRight); };
        split_ms_button_.getButton().onClick = [this]() { splitBand(zlp::FilterStereo::kMid, zlp::FilterStereo::kSide); };
        copy_button_.getButton().onClick = [this]() { copyBand(); };
        paste_button_.getButton().onClick = [this]() { pasteBand(); };
        delete_button_.getButton().onClick = [this]() { deleteBand(); };

        base_.getSoloWholeIdxTree().addListener(this);
        updatePageVisibility();
        setVisible(false);
    }

    BandHubPanel::~BandHubPanel() {
        stopTimer();
        base_.getSoloWholeIdxTree().removeListener(this);
    }

    void BandHubPanel::stylePill(zlgui::button::ClickTextButton& button, const bool quiet) {
        button.getLAF().setFontScale(quiet ? .65f : .70f);
        button.setBackgroundPainter([quiet](juce::Graphics& g, juce::Button& b,
                                            const bool highlighted, const bool down) {
            const auto active = b.getToggleState() || down;
            auto r = b.getLocalBounds().toFloat().reduced(.5f);
            const auto fill = active ? .105f : (highlighted ? .052f : (quiet ? .020f : .035f));
            const auto rim = active ? .17f : (highlighted ? .095f : .055f);
            if (fill > .001f)
                zlgui::glass::fillGlassSurface(g, r, r.getHeight() * .5f, fill, rim, rim);
            if (active) {
                g.setColour(juce::Colour(127, 187, 235).withAlpha(.10f));
                g.fillRoundedRectangle(r.reduced(1.f), r.getHeight() * .45f);
            }
        });
    }

    void BandHubPanel::paint(juce::Graphics& g) {
        if (band_ >= zlp::kBandNum || hub_surface_.isEmpty()) return;

        const auto font = base_.getFontSize();
        const auto radius = juce::jmax(12.f, font * 1.05f);
        zlgui::glass::fillGlassSurface(g, hub_surface_, radius, .082f, .145f, .15f);

        const auto accent = base_.getColourMap1(band_);
        auto accent_line = hub_surface_.reduced(font * 1.2f, 0.f);
        accent_line.setHeight(1.1f);
        accent_line.setY(hub_surface_.getY() + .5f);
        juce::ColourGradient glint(accent.withAlpha(.0f), accent_line.getX(), accent_line.getY(),
                                   accent.withAlpha(.0f), accent_line.getRight(), accent_line.getY(), false);
        glint.addColour(.28, accent.withAlpha(.22f));
        glint.addColour(.52, accent.interpolatedWith(juce::Colours::white, .28f).withAlpha(.42f));
        glint.addColour(.76, accent.withAlpha(.14f));
        g.setGradientFill(glint);
        g.fillRoundedRectangle(accent_line, .6f);

        if (!identity_bound_.isEmpty()) {
            auto identity = identity_bound_.toFloat();
            const auto dot = juce::jmax(5.f, font * .48f);
            g.setColour(accent.withAlpha(.92f));
            g.fillEllipse(identity.getX(), identity.getCentreY() - dot * .5f, dot, dot);
            g.setColour(juce::Colours::white.withAlpha(.62f));
            g.drawEllipse(identity.getX(), identity.getCentreY() - dot * .5f, dot, dot, .8f);

            int filter_index = -1;
            if (filter_type_ptr_ != nullptr)
                filter_index = static_cast<int>(std::round(filter_type_ptr_->load(std::memory_order::relaxed)));
            const auto filter = filter_index >= 0 && filter_index < static_cast<int>(kFilterNames.size())
                ? juce::String{kFilterNames[static_cast<size_t>(filter_index)]}
                : juce::String{"Band"};
            const auto text = "Band " + juce::String(static_cast<int>(band_) + 1) + "  ·  " + filter;
            identity.removeFromLeft(dot + font * .50f);
            g.setColour(zlgui::glass::textPrimary().withAlpha(.90f));
            g.setFont(juce::FontOptions(font * .72f));
            g.drawFittedText(text, identity.toNearestInt(), juce::Justification::centredLeft, 1);
        }

        if (reveal_ > .08f && !page_title_bound_.isEmpty()) {
            const char* title = page_ == Page::dynamics ? "DYNAMIC EQ"
                : page_ == Page::detector ? "DETECTOR"
                : page_ == Page::sidechain ? "SIDECHAIN"
                : "BAND ACTIONS";
            g.setColour(zlgui::glass::textTertiary().withAlpha(.62f * reveal_));
            g.setFont(juce::FontOptions(font * .55f));
            g.drawText(title, page_title_bound_, juce::Justification::centredLeft, false);
        }

        if (reveal_ > .08f && page_ == Page::sidechain) {
            static constexpr std::array<const char*, 4> labels{"FILTER", "SLOPE", "FREQUENCY", "Q"};
            g.setColour(zlgui::glass::textTertiary().withAlpha(.58f * reveal_));
            g.setFont(juce::FontOptions(font * .52f));
            for (size_t i = 0; i < labels.size(); ++i)
                g.drawText(labels[i], value_label_bounds_[i], juce::Justification::centredBottom, false);
        }

        if (reveal_ > .08f && page_ == Page::dynamics && dynamic_on_ptr_ != nullptr
            && dynamic_on_ptr_->load(std::memory_order::relaxed) <= .5f) {
            auto notice = hub_surface_.toNearestInt().reduced(juce::roundToInt(font * 2.0f));
            notice.removeFromTop(juce::roundToInt(font * 6.7f));
            g.setColour(zlgui::glass::textTertiary().withAlpha(.48f * reveal_));
            g.setFont(juce::FontOptions(font * .58f));
            g.drawFittedText("Enable Dynamic to process this band. The controls stay visible so the workflow never jumps.",
                             notice, juce::Justification::centred, 1);
        }
    }

    void BandHubPanel::resized() { layoutForReveal(); }

    void BandHubPanel::layoutForReveal() {
        if (getWidth() <= 0 || getHeight() <= 0 || band_ >= zlp::kBandNum) {
            hub_surface_ = {};
            return;
        }

        const auto font = base_.getFontSize();
        const auto padding = juce::jmax(5, getPaddingSize(font));
        const auto button = getButtonSize(font);
        const auto collapsed_h = juce::jmax(button + 2 * padding, juce::roundToInt(font * 3.15f));
        const auto expanded_h = juce::jmax(collapsed_h + 4 * button + 4 * padding,
                                            juce::roundToInt(font * 18.2f));
        const auto eased = reveal_ * reveal_ * (3.f - 2.f * reveal_);
        const auto height = juce::roundToInt(static_cast<float>(collapsed_h)
                                             + (expanded_h - collapsed_h) * eased);

        const auto margin = juce::jmax(8, juce::roundToInt(font * .75f));
        const auto desired_w = juce::roundToInt(static_cast<float>(getWidth()) * .72f);
        const auto min_w = juce::roundToInt(font * 37.f);
        const auto width = juce::jlimit(juce::jmin(getWidth() - 2 * margin, min_w),
                                        juce::jmax(1, getWidth() - 2 * margin),
                                        juce::jmax(desired_w, min_w));
        const auto x = (getWidth() - width) / 2;
        const auto y = juce::jmax(margin, getHeight() - height - margin);
        hub_surface_ = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                              static_cast<float>(width), static_cast<float>(height));

        auto content = hub_surface_.toNearestInt().reduced(padding + padding / 2, padding);
        auto header = content.removeFromTop(button);

        const auto expand_w = juce::jmax(button * 2, juce::roundToInt(font * 3.4f));
        expand_button_.setBounds(header.removeFromRight(expand_w));
        header.removeFromRight(padding / 2);
        const auto dynamic_w = juce::jmax(button * 2, juce::roundToInt(font * 4.6f));
        dynamic_toggle_.setBounds(header.removeFromRight(dynamic_w));
        header.removeFromRight(padding / 2);
        const auto channel_w = juce::jmax(button * 2, juce::roundToInt(font * 4.8f));
        channel_box_.setBounds(header.removeFromRight(channel_w));
        header.removeFromRight(padding);
        identity_bound_ = header;

        const auto details_visible = reveal_ > .035f;
        const auto detail_alpha = juce::jlimit(0.f, 1.f, (reveal_ - .04f) / .32f);

        for (auto* c : {static_cast<juce::Component*>(&dynamics_page_button_), &detector_page_button_,
                        &sidechain_page_button_, &actions_page_button_,
                        &threshold_slider_, &range_slider_, &attack_slider_, &release_slider_,
                        &knee_slider_, &rms_length_slider_, &rms_mix_slider_, &smooth_slider_,
                        &learn_button_, &relative_button_, &dynamic_bypass_button_,
                        &side_type_box_, &side_order_box_, &side_freq_slider_, &side_q_slider_,
                        &side_link_button_, &side_swap_button_, &external_button_,
                        &solo_button_, &invert_button_, &split_lr_button_, &split_ms_button_,
                        &copy_button_, &paste_button_, &delete_button_}) {
            c->setAlpha(detail_alpha);
        }

        if (!details_visible) {
            page_title_bound_ = {};
            for (auto& r : value_label_bounds_) r = {};
            updatePageVisibility();
            repaint();
            return;
        }

        content.removeFromTop(padding / 2);
        auto page_row = content.removeFromTop(juce::jmax(button * 3 / 4, juce::roundToInt(font * 1.35f)));
        const auto page_gap = juce::jmax(2, padding / 3);
        const auto page_w = (page_row.getWidth() - 3 * page_gap) / 4;
        dynamics_page_button_.setBounds(page_row.removeFromLeft(page_w)); page_row.removeFromLeft(page_gap);
        detector_page_button_.setBounds(page_row.removeFromLeft(page_w)); page_row.removeFromLeft(page_gap);
        sidechain_page_button_.setBounds(page_row.removeFromLeft(page_w)); page_row.removeFromLeft(page_gap);
        actions_page_button_.setBounds(page_row);

        content.removeFromTop(padding / 2);
        page_title_bound_ = content.removeFromTop(juce::jmax(9, juce::roundToInt(font * .78f)));
        content.removeFromTop(padding / 3);

        for (auto& r : value_label_bounds_) r = {};

        if (page_ == Page::dynamics || page_ == Page::detector) {
            auto dial_row = content.removeFromTop(juce::jmax(button * 2 + padding,
                                                             juce::roundToInt(font * 7.0f)));
            const auto gap = juce::jmax(3, padding / 2);
            const auto dial_w = (dial_row.getWidth() - 3 * gap) / 4;
            auto place = [&dial_row, dial_w, gap](juce::Component& c, const bool last = false) {
                c.setBounds(last ? dial_row : dial_row.removeFromLeft(dial_w));
                if (!last) dial_row.removeFromLeft(gap);
            };
            if (page_ == Page::dynamics) {
                place(threshold_slider_); place(range_slider_); place(attack_slider_); place(release_slider_, true);
            } else {
                place(knee_slider_); place(rms_length_slider_); place(rms_mix_slider_); place(smooth_slider_, true);
                content.removeFromTop(padding / 3);
                auto flags = content.removeFromTop(juce::jmax(button * 3 / 4, juce::roundToInt(font * 1.3f)));
                const auto fw = (flags.getWidth() - 2 * gap) / 3;
                learn_button_.setBounds(flags.removeFromLeft(fw)); flags.removeFromLeft(gap);
                relative_button_.setBounds(flags.removeFromLeft(fw)); flags.removeFromLeft(gap);
                dynamic_bypass_button_.setBounds(flags);
            }
        } else if (page_ == Page::sidechain) {
            const auto label_h = juce::jmax(8, juce::roundToInt(font * .66f));
            auto labels = content.removeFromTop(label_h);
            auto values = content.removeFromTop(button);
            const auto gap = juce::jmax(3, padding / 2);
            const auto cell = (values.getWidth() - 3 * gap) / 4;
            for (size_t i = 0; i < 4; ++i) {
                value_label_bounds_[i] = labels.removeFromLeft(cell);
                if (i < 3) labels.removeFromLeft(gap);
            }
            side_type_box_.setBounds(values.removeFromLeft(cell)); values.removeFromLeft(gap);
            side_order_box_.setBounds(values.removeFromLeft(cell)); values.removeFromLeft(gap);
            side_freq_slider_.setBounds(values.removeFromLeft(cell)); values.removeFromLeft(gap);
            side_q_slider_.setBounds(values);
            content.removeFromTop(padding / 2);
            auto flags = content.removeFromTop(juce::jmax(button * 3 / 4, juce::roundToInt(font * 1.30f)));
            const auto fw = (flags.getWidth() - 2 * gap) / 3;
            external_button_.setBounds(flags.removeFromLeft(fw)); flags.removeFromLeft(gap);
            side_link_button_.setBounds(flags.removeFromLeft(fw)); flags.removeFromLeft(gap);
            side_swap_button_.setBounds(flags);
        } else {
            const auto gap = juce::jmax(3, padding / 2);
            auto row1 = content.removeFromTop(juce::jmax(button, juce::roundToInt(font * 2.15f)));
            const auto w1 = (row1.getWidth() - 3 * gap) / 4;
            solo_button_.setBounds(row1.removeFromLeft(w1)); row1.removeFromLeft(gap);
            invert_button_.setBounds(row1.removeFromLeft(w1)); row1.removeFromLeft(gap);
            split_lr_button_.setBounds(row1.removeFromLeft(w1)); row1.removeFromLeft(gap);
            split_ms_button_.setBounds(row1);
            content.removeFromTop(gap);
            auto row2 = content.removeFromTop(juce::jmax(button, juce::roundToInt(font * 2.15f)));
            const auto w2 = (row2.getWidth() - 2 * gap) / 3;
            copy_button_.setBounds(row2.removeFromLeft(w2)); row2.removeFromLeft(gap);
            paste_button_.setBounds(row2.removeFromLeft(w2)); row2.removeFromLeft(gap);
            delete_button_.setBounds(row2);
        }

        updatePageVisibility();
        repaint();
    }

    void BandHubPanel::setExpanded(const bool should_expand) {
        if (band_ >= zlp::kBandNum) return;
        expanded_ = should_expand;
        reveal_target_ = expanded_ ? 1.f : 0.f;
        expand_button_.getButton().setButtonText(expanded_ ? "Close" : "Open");
        if (std::abs(reveal_target_ - reveal_) > .002f) startTimerHz(60);
        updatePageVisibility();
        repaint();
    }

    void BandHubPanel::timerCallback() {
        const auto delta = reveal_target_ - reveal_;
        reveal_ += delta * .22f;
        if (std::abs(delta) < .004f) {
            reveal_ = reveal_target_;
            stopTimer();
        }
        layoutForReveal();
        repaint();
    }

    void BandHubPanel::setPage(const Page page) {
        page_ = page;
        dynamics_page_button_.getButton().setToggleState(page == Page::dynamics, juce::dontSendNotification);
        detector_page_button_.getButton().setToggleState(page == Page::detector, juce::dontSendNotification);
        sidechain_page_button_.getButton().setToggleState(page == Page::sidechain, juce::dontSendNotification);
        actions_page_button_.getButton().setToggleState(page == Page::actions, juce::dontSendNotification);
        if (!expanded_) setExpanded(true);
        updatePageVisibility();
        layoutForReveal();
        repaint();
    }

    void BandHubPanel::updatePageVisibility() {
        const auto detail = band_ < zlp::kBandNum && reveal_ > .035f;
        for (auto* c : {static_cast<juce::Component*>(&dynamics_page_button_), &detector_page_button_,
                        &sidechain_page_button_, &actions_page_button_}) c->setVisible(detail);

        const auto dyn = detail && page_ == Page::dynamics;
        for (auto* c : {static_cast<juce::Component*>(&threshold_slider_), &range_slider_, &attack_slider_, &release_slider_})
            c->setVisible(dyn);

        const auto detector = detail && page_ == Page::detector;
        for (auto* c : {static_cast<juce::Component*>(&knee_slider_), &rms_length_slider_, &rms_mix_slider_, &smooth_slider_,
                        &learn_button_, &relative_button_, &dynamic_bypass_button_}) c->setVisible(detector);

        const auto side = detail && page_ == Page::sidechain;
        for (auto* c : {static_cast<juce::Component*>(&side_type_box_), &side_order_box_, &side_freq_slider_, &side_q_slider_,
                        &side_link_button_, &side_swap_button_, &external_button_}) c->setVisible(side);

        const auto actions = detail && page_ == Page::actions;
        for (auto* c : {static_cast<juce::Component*>(&solo_button_), &invert_button_, &split_lr_button_, &split_ms_button_,
                        &copy_button_, &paste_button_, &delete_button_}) c->setVisible(actions);
    }

    void BandHubPanel::clearBandAttachments() {
        channel_attach_.reset();
        dynamic_attach_.reset();
        threshold_attach_.reset(); range_attach_.reset(); attack_attach_.reset(); release_attach_.reset();
        knee_attach_.reset(); rms_length_attach_.reset(); rms_mix_attach_.reset(); smooth_attach_.reset();
        learn_attach_.reset(); relative_attach_.reset(); dynamic_bypass_attach_.reset();
        side_type_attach_.reset(); side_order_attach_.reset(); side_freq_attach_.reset(); side_q_attach_.reset();
        side_link_attach_.reset(); side_swap_attach_.reset();
        dynamic_on_ptr_ = nullptr;
        filter_type_ptr_ = nullptr;
    }

    void BandHubPanel::updateBand() {
        const auto next_band = base_.getSelectedBand();
        if (next_band == band_ && next_band < zlp::kBandNum) return;

        clearBandAttachments();
        band_ = next_band;
        if (band_ >= zlp::kBandNum) {
            expanded_ = false; reveal_ = reveal_target_ = 0.f;
            stopTimer();
            setVisible(false);
            return;
        }

        const auto s = std::to_string(band_);
        channel_attach_ = std::make_unique<zlgui::attachment::ComboBoxAttachment<true>>(
            channel_box_.getBox(), p_ref_.parameters_, zlp::PLRMode::kID + s, updater_);
        dynamic_attach_ = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(
            dynamic_toggle_.getButton(), p_ref_.parameters_, zlp::PDynamicON::kID + s, updater_, juce::dontSendNotification);

        threshold_attach_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
            threshold_slider_.getSlider(), p_ref_.parameters_, zlp::PThreshold::kID + s, updater_);
        range_attach_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
            range_slider_.getSlider(), p_ref_.parameters_, zlp::PTargetGain::kID + s, updater_);
        attack_attach_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
            attack_slider_.getSlider(), p_ref_.parameters_, zlp::PAttack::kID + s, updater_);
        release_attach_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
            release_slider_.getSlider(), p_ref_.parameters_, zlp::PRelease::kID + s, updater_);

        knee_attach_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
            knee_slider_.getSlider(), p_ref_.parameters_, zlp::PKneeW::kID + s, updater_);
        rms_length_attach_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
            rms_length_slider_.getSlider(), p_ref_.parameters_, zlp::PDynamicRMSLength::kID + s, updater_);
        rms_mix_attach_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
            rms_mix_slider_.getSlider(), p_ref_.parameters_, zlp::PDynamicRMSMix::kID + s, updater_);
        smooth_attach_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
            smooth_slider_.getSlider(), p_ref_.parameters_, zlp::PDynamicSmooth::kID + s, updater_);

        learn_attach_ = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(
            learn_button_.getButton(), p_ref_.parameters_, zlp::PDynamicLearn::kID + s, updater_, juce::dontSendNotification);
        relative_attach_ = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(
            relative_button_.getButton(), p_ref_.parameters_, zlp::PDynamicRelative::kID + s, updater_, juce::dontSendNotification);
        dynamic_bypass_attach_ = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(
            dynamic_bypass_button_.getButton(), p_ref_.parameters_, zlp::PDynamicBypass::kID + s, updater_, juce::dontSendNotification);

        side_type_attach_ = std::make_unique<zlgui::attachment::ComboBoxAttachment<true>>(
            side_type_box_.getBox(), p_ref_.parameters_, zlp::PSideFilterType::kID + s, updater_);
        side_order_attach_ = std::make_unique<zlgui::attachment::ComboBoxAttachment<true>>(
            side_order_box_.getBox(), p_ref_.parameters_, zlp::PSideOrder::kID + s, updater_);
        side_freq_attach_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
            side_freq_slider_.getSlider(), p_ref_.parameters_, zlp::PSideFreq::kID + s, updater_);
        side_q_attach_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(
            side_q_slider_.getSlider(), p_ref_.parameters_, zlp::PSideQ::kID + s, updater_);
        side_link_attach_ = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(
            side_link_button_.getButton(), p_ref_.parameters_, zlp::PSideLink::kID + s, updater_, juce::dontSendNotification);
        side_swap_attach_ = std::make_unique<zlgui::attachment::ButtonAttachment<true>>(
            side_swap_button_.getButton(), p_ref_.parameters_, zlp::PSideSwap::kID + s, updater_, juce::dontSendNotification);

        dynamic_on_ptr_ = p_ref_.parameters_.getRawParameterValue(zlp::PDynamicON::kID + s);
        filter_type_ptr_ = p_ref_.parameters_.getRawParameterValue(zlp::PFilterType::kID + s);

        updater_.updateComponents();
        const auto accent = base_.getColourMap1(band_);
        for (auto* slider : {&threshold_slider_, &range_slider_, &attack_slider_, &release_slider_,
                             &knee_slider_, &rms_length_slider_, &rms_mix_slider_, &smooth_slider_})
            slider->setGlassRotaryAccent(accent);

        // Keep an already-open hub stable when moving between nodes.  This is the core
        // difference from the old travelling inspector: the workspace does not jump.
        setVisible(true);
        layoutForReveal();
        repaintCallbackSlow();
    }

    void BandHubPanel::repaintCallbackSlow() {
        if (band_ >= zlp::kBandNum) return;
        updater_.updateComponents();
        threshold_slider_.updateDisplayValue(); range_slider_.updateDisplayValue();
        attack_slider_.updateDisplayValue(); release_slider_.updateDisplayValue();
        knee_slider_.updateDisplayValue(); rms_length_slider_.updateDisplayValue();
        rms_mix_slider_.updateDisplayValue(); smooth_slider_.updateDisplayValue();
        side_freq_slider_.updateDisplayValue(); side_q_slider_.updateDisplayValue();

        const auto dynamic_on = dynamic_on_ptr_ != nullptr
            && dynamic_on_ptr_->load(std::memory_order::relaxed) > .5f;
        for (auto* slider : {&threshold_slider_, &range_slider_, &attack_slider_, &release_slider_,
                             &knee_slider_, &rms_length_slider_, &rms_mix_slider_, &smooth_slider_})
            slider->setEditable(dynamic_on);
        detector_page_button_.setAlpha(dynamic_on ? 1.f : .42f);
        sidechain_page_button_.setAlpha(dynamic_on ? 1.f : .42f);
        solo_button_.getButton().setToggleState(base_.getSoloWholeIdx() == band_, juce::dontSendNotification);
        repaint();
    }

    void BandHubPanel::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) {
        if (band_ < zlp::kBandNum)
            solo_button_.getButton().setToggleState(base_.getSoloWholeIdx() == band_, juce::dontSendNotification);
    }

    void BandHubPanel::invertGain() {
        if (band_ >= zlp::kBandNum) return;
        auto* gain = p_ref_.parameters_.getParameter(zlp::PGain::kID + std::to_string(band_));
        auto* target = p_ref_.parameters_.getParameter(zlp::PTargetGain::kID + std::to_string(band_));
        updateValue(gain, 1.f - gain->getValue());
        updateValue(target, 1.f - target->getValue());
    }

    void BandHubPanel::splitBand(const zlp::FilterStereo stereo1, const zlp::FilterStereo stereo2) {
        if (band_ >= zlp::kBandNum) return;
        const auto band2 = band_helper::findOffBand(p_ref_);
        if (band2 >= zlp::kBandNum) return;
        const auto s1 = std::to_string(band_);
        const auto s2 = std::to_string(band2);
        for (const auto& id : kCopyIDs) {
            if (id == zlp::PLRMode::kID) continue;
            auto* p1 = p_ref_.parameters_.getParameter(id + s1);
            auto* p2 = p_ref_.parameters_.getParameter(id + s2);
            updateValue(p2, p1->getValue());
        }
        auto* p1 = p_ref_.parameters_.getParameter(zlp::PLRMode::kID + s1);
        auto* p2 = p_ref_.parameters_.getParameter(zlp::PLRMode::kID + s2);
        const auto mode = p1->convertFrom0to1(p1->getValue());
        if (std::abs(mode - static_cast<float>(stereo1)) < .1f)
            updateValue(p2, p2->convertTo0to1(static_cast<float>(stereo2)));
        else if (std::abs(mode - static_cast<float>(stereo2)) < .1f)
            updateValue(p2, p2->convertTo0to1(static_cast<float>(stereo1)));
        else {
            updateValue(p1, p1->convertTo0to1(static_cast<float>(stereo1)));
            updateValue(p2, p2->convertTo0to1(static_cast<float>(stereo2)));
        }
        base_.setSelectedBand(band2);
    }

    void BandHubPanel::copyBand() {
        if (band_ >= zlp::kBandNum) return;
        juce::ValueTree tree{"filter_info"};
        auto selected = base_.getSelectedBandSet().getItemArray();
        if (selected.isEmpty()) selected.add(band_);
        int index = 0;
        for (const size_t b : selected) {
            juce::ValueTree filter{juce::Identifier{"filter" + std::to_string(index++)}};
            tree.addChild(filter, -1, nullptr);
            const auto s = std::to_string(b);
            for (const auto& id : kCopyIDs) {
                const auto* para = p_ref_.parameters_.getParameter(id + s);
                filter.setProperty(id, para->getCurrentValueAsText(), nullptr);
            }
        }
        juce::SystemClipboard::copyTextToClipboard(tree.toXmlString());
    }

    void BandHubPanel::pasteBand() {
        const auto tree = juce::ValueTree::fromXml(juce::SystemClipboard::getTextFromClipboard());
        if (!tree.hasType("filter_info")) return;
        base_.getSelectedBandSet().deselectAll();
        for (size_t i = 0; i < zlp::kBandNum; ++i) {
            const auto filter = tree.getChildWithName(juce::Identifier{"filter" + std::to_string(i)});
            if (!filter.isValid()) return;
            const auto b = band_helper::findOffBand(p_ref_);
            if (b >= zlp::kBandNum) return;
            const auto s = std::to_string(b);
            for (const auto& id : kCopyIDs) {
                if (!filter.hasProperty(id)) continue;
                auto* para = p_ref_.parameters_.getParameter(id + s);
                updateValue(para, para->getValueForText(filter.getProperty(id)));
            }
            base_.getSelectedBandSet().addToSelection(b);
            base_.setSelectedBand(b);
        }
    }

    void BandHubPanel::deleteBand() {
        if (band_ >= zlp::kBandNum) return;
        const auto old_band = band_;
        band_helper::turnOffBand(p_ref_, old_band, base_.getSelectedBandSet());
        const auto before = band_helper::findClosestBand<true>(p_ref_, old_band);
        const auto after = band_helper::findClosestBand<false>(p_ref_, old_band);
        if (before < zlp::kBandNum) base_.setSelectedBand(before);
        else if (after < zlp::kBandNum) base_.setSelectedBand(after);
        else base_.setSelectedBand(zlp::kBandNum);
    }
}
