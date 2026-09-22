// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer

#include "float_pop_panel.hpp"
#include "../../../../gui/glass_tokens.hpp"
#include "BinaryData.h"
#include <array>
#include <cstdio>

namespace zlpanel {
    FloatPopPanel::FloatPopPanel(PluginProcessor& p, zlgui::UIBase& base,
                                 const multilingual::TooltipHelper& tooltip_helper) :
        p_ref_(p), base_(base), updater_(),
        bypass_drawable_(juce::Drawable::createFromImageData(BinaryData::bypass_svg, BinaryData::bypass_svgSize)),
        bypass_button_(base, bypass_drawable_.get(), bypass_drawable_.get(),
                       tooltip_helper.getToolTipText(multilingual::kBandBypass)),
        dynamic_drawable_(juce::Drawable::createFromImageData(BinaryData::dynamic_svg, BinaryData::dynamic_svgSize)),
        dynamic_button_(base, dynamic_drawable_.get(), dynamic_drawable_.get(),
                        tooltip_helper.getToolTipText(multilingual::kBandDynamic)),
        solo_drawable_(juce::Drawable::createFromImageData(BinaryData::solo_svg, BinaryData::solo_svgSize)),
        solo_button_(base, solo_drawable_.get(), solo_drawable_.get(),
                     tooltip_helper.getToolTipText(multilingual::kBandSolo)),
        ftype_box_(juce::StringArray{"Bell", "Low Shelf", "High Cut", "High Shelf", "Low Cut", "Notch",
                                     "Band Pass", "Tilt", "Flat Tilt", "All Pass", "Gain"}, base, ""),
        slope_box_(zlp::POrder::kChoices, base, ""),
        freq_slider_("", base), gain_slider_("", base), q_slider_("", base) {
        setOpaque(false);
        setInterceptsMouseClicks(true, true);

        bypass_button_.setImageAlpha(0.f, 0.f, 0.f, 0.f);
        addAndMakeVisible(bypass_button_);
        bypass_button_.getButton().onClick = [this]() {
            if (const auto band = base_.getSelectedBand(); band < zlp::kBandNum) {
                updateValue(p_ref_.parameters_.getParameter(zlp::PFilterStatus::kID + std::to_string(band)),
                            bypass_button_.getToggleState() ? 1.f : .5f);
            }
        };

        dynamic_button_.setImageAlpha(0.f, 0.f, 0.f, 0.f);
        dynamic_button_.getButton().setToggleable(true);
        dynamic_button_.getButton().setClickingTogglesState(false);
        addAndMakeVisible(dynamic_button_);
        dynamic_button_.getButton().onClick = [this]() {
            if (const auto band = base_.getSelectedBand(); band < zlp::kBandNum) {
                const auto next = !(dynamic_on_ptr_ != nullptr
                    && dynamic_on_ptr_->load(std::memory_order::relaxed) > .5f);
                const auto max_idx = std::round(p_ref_.parameters_NA_.getRawParameterValue(
                    zlstate::PEQMaxDB::kID)->load(std::memory_order::relaxed));
                band_helper::turnOnOffDynamic(p_ref_, band, next,
                                              base_.getCurveDBScale(static_cast<size_t>(max_idx)));
                dynamic_button_.getButton().setToggleState(next, juce::dontSendNotification);
                repaint();
            }
        };

        solo_button_.setImageAlpha(0.f, 0.f, 0.f, 0.f);
        solo_button_.getButton().setToggleable(true);
        solo_button_.getButton().setClickingTogglesState(false);
        addAndMakeVisible(solo_button_);
        solo_button_.getButton().onClick = [this]() {
            if (const auto band = base_.getSelectedBand(); band < zlp::kBandNum) {
                const auto active = base_.getSoloWholeIdx() == band;
                base_.setSoloWholeIdx(active ? 2 * zlp::kBandNum : band);
                solo_button_.getButton().setToggleState(!active, juce::dontSendNotification);
                repaint();
            }
        };

        const auto popup = juce::PopupMenu::Options()
            .withPreferredPopupDirection(juce::PopupMenu::Options::PopupDirection::upwards)
            .withMinimumNumColumns(1);
        for (auto* box : {&ftype_box_, &slope_box_}) {
            box->getLAF().setOption(popup);
            box->setBufferedToImage(true);
            box->setAlpha(.01f);
            addAndMakeVisible(*box);
        }
        slope_box_.setScrollEnabled(true);

        auto setup = [this](auto& slider) {
            slider.setFontScale(.72f);
            slider.getSlider().setSliderSnapsToMousePosition(false);
            slider.setBufferedToImage(true);
            addAndMakeVisible(slider);
        };
        setup(freq_slider_); setup(gain_slider_); setup(q_slider_);

        freq_slider_.setPrecision(4);
        freq_slider_.permitted_characters_ = "0123456789.kK";
        freq_slider_.value_formatter_ = [](double v) -> std::string {
            char b[32];
            if (v >= 1000.0) snprintf(b, sizeof(b), "%.2f kHz", v * .001);
            else snprintf(b, sizeof(b), v >= 100.0 ? "%.0f Hz" : "%.1f Hz", v);
            return b;
        };
        gain_slider_.setPrecision(3);
        gain_slider_.permitted_characters_ = "-+0123456789.";
        gain_slider_.value_formatter_ = [](double v) -> std::string {
            char b[32]; snprintf(b, sizeof(b), "%+.2f dB", v); return b;
        };
        q_slider_.setPrecision(3);
        q_slider_.permitted_characters_ = "0123456789.";
        q_slider_.value_formatter_ = [](double v) -> std::string {
            char b[32]; snprintf(b, sizeof(b), "Q %.2f", v); return b;
        };
    }

    void FloatPopPanel::drawFilterGlyph(juce::Graphics& g, juce::Rectangle<float> r,
                                        const int type, const juce::Colour colour) const {
        r = r.reduced(r.getHeight() * .22f);
        const auto y = r.getCentreY();
        juce::Path p;
        switch (type) {
            case 0: case 6:
                p.startNewSubPath(r.getX(), y + r.getHeight() * .18f);
                p.cubicTo(r.getX()+r.getWidth()*.28f, y+r.getHeight()*.18f,
                          r.getX()+r.getWidth()*.34f, r.getY(), r.getCentreX(), r.getY());
                p.cubicTo(r.getX()+r.getWidth()*.66f, r.getY(),
                          r.getX()+r.getWidth()*.72f, y+r.getHeight()*.18f, r.getRight(), y+r.getHeight()*.18f);
                break;
            case 1: case 2:
                p.startNewSubPath(r.getX(), r.getY()+r.getHeight()*.18f);
                p.lineTo(r.getX()+r.getWidth()*.32f, r.getY()+r.getHeight()*.18f);
                p.cubicTo(r.getCentreX(), r.getY()+r.getHeight()*.18f,
                          r.getCentreX(), r.getBottom()-r.getHeight()*.18f,
                          r.getRight(), r.getBottom()-r.getHeight()*.18f);
                break;
            case 3: case 4:
                p.startNewSubPath(r.getX(), r.getBottom()-r.getHeight()*.18f);
                p.lineTo(r.getX()+r.getWidth()*.32f, r.getBottom()-r.getHeight()*.18f);
                p.cubicTo(r.getCentreX(), r.getBottom()-r.getHeight()*.18f,
                          r.getCentreX(), r.getY()+r.getHeight()*.18f,
                          r.getRight(), r.getY()+r.getHeight()*.18f);
                break;
            case 5:
                p.startNewSubPath(r.getX(), y-r.getHeight()*.12f);
                p.cubicTo(r.getX()+r.getWidth()*.38f, y-r.getHeight()*.12f,
                          r.getX()+r.getWidth()*.42f, r.getBottom(), r.getCentreX(), r.getBottom());
                p.cubicTo(r.getX()+r.getWidth()*.58f, r.getBottom(),
                          r.getX()+r.getWidth()*.62f, y-r.getHeight()*.12f, r.getRight(), y-r.getHeight()*.12f);
                break;
            case 7: case 8:
                p.startNewSubPath(r.getX(), r.getBottom()); p.lineTo(r.getRight(), r.getY()); break;
            case 9:
                p.startNewSubPath(r.getX(), y); p.lineTo(r.getX()+r.getWidth()*.30f, y);
                p.lineTo(r.getCentreX(), r.getY()); p.lineTo(r.getX()+r.getWidth()*.70f, y); p.lineTo(r.getRight(), y);
                break;
            default:
                p.startNewSubPath(r.getX(), y); p.lineTo(r.getRight(), y); break;
        }
        g.setColour(colour);
        g.strokePath(p, juce::PathStrokeType(1.15f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void FloatPopPanel::drawQuickActionGlyphs(juce::Graphics& g) const {
        const auto primary = zlgui::glass::textPrimary();

        auto dyn = dynamic_button_.getBounds().toFloat().reduced(static_cast<float>(dynamic_button_.getHeight()) * .25f);
        const auto dynActive = dynamic_button_.getToggleState();
        g.setColour(primary.withAlpha(dynActive ? .95f : .46f));
        juce::Path dp;
        dp.startNewSubPath(dyn.getX(), dyn.getCentreY() + dyn.getHeight() * .18f);
        dp.cubicTo(dyn.getX() + dyn.getWidth() * .25f, dyn.getBottom(),
                   dyn.getX() + dyn.getWidth() * .32f, dyn.getY(), dyn.getCentreX(), dyn.getCentreY());
        dp.cubicTo(dyn.getX() + dyn.getWidth() * .68f, dyn.getBottom(),
                   dyn.getX() + dyn.getWidth() * .75f, dyn.getY(), dyn.getRight(), dyn.getCentreY() - dyn.getHeight() * .18f);
        g.strokePath(dp, juce::PathStrokeType(1.25f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        if (dynActive) g.drawLine(dyn.getX(), dyn.getBottom() - .5f, dyn.getRight(), dyn.getBottom() - .5f, .9f);

        auto solo = solo_button_.getBounds().toFloat().reduced(static_cast<float>(solo_button_.getHeight()) * .26f);
        const auto soloActive = solo_button_.getToggleState();
        g.setColour(primary.withAlpha(soloActive ? .95f : .46f));
        juce::Path hp;
        hp.addCentredArc(solo.getCentreX(), solo.getCentreY() + solo.getHeight() * .08f,
                         solo.getWidth() * .42f, solo.getHeight() * .42f, 0.f, 4.02f, 5.40f, true);
        g.strokePath(hp, juce::PathStrokeType(1.25f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        const auto padW = solo.getWidth() * .16f;
        const auto padH = solo.getHeight() * .38f;
        g.drawRoundedRectangle({solo.getX(), solo.getCentreY(), padW, padH}, padW * .4f, 1.1f);
        g.drawRoundedRectangle({solo.getRight() - padW, solo.getCentreY(), padW, padH}, padW * .4f, 1.1f);
    }

    void FloatPopPanel::paint(juce::Graphics& g) {
        static constexpr std::array<const char*, 11> names{
            "Bell", "Low Shelf", "High Cut", "High Shelf", "Low Cut", "Notch",
            "Band Pass", "Tilt", "Flat Tilt", "All Pass", "Gain"};

        auto card = getLocalBounds().toFloat().reduced(.5f);
        zlgui::glass::fillGlassSurface(g, card, juce::jmax(9.f, card.getHeight() * .20f), .085f, .135f, .15f);

        if (const auto band = base_.getSelectedBand(); band < zlp::kBandNum) {
            const auto accent = base_.getColourMap1(band);
            auto glint = card.reduced(base_.getFontSize() * .58f, 0.f);
            glint.setHeight(1.f);
            juce::ColourGradient grad(accent.withAlpha(.0f), glint.getX(), glint.getY(),
                                      accent.interpolatedWith(juce::Colours::white, .30f).withAlpha(.36f),
                                      glint.getCentreX(), glint.getY(), false);
            grad.addColour(.86, accent.withAlpha(.08f));
            g.setGradientFill(grad);
            g.fillRoundedRectangle(glint, .5f);
        }

        const auto type = ftype_box_.getBox().getSelectedItemIndex();
        const auto typePill = ftype_box_.getBounds().getUnion(filter_name_bound_).toFloat().expanded(.5f, .2f);
        zlgui::glass::fillGlassSurface(g, typePill, typePill.getHeight() * .5f, .045f, .075f, .085f);
        drawFilterGlyph(g, ftype_box_.getBounds().toFloat(), type, zlgui::glass::textPrimary().withAlpha(.86f));
        if (type >= 0 && type < static_cast<int>(names.size())) {
            g.setColour(zlgui::glass::textPrimary().withAlpha(.91f));
            g.setFont(juce::FontOptions(base_.getFontSize() * .68f));
            g.drawFittedText(names[static_cast<size_t>(type)], filter_name_bound_, juce::Justification::centredLeft, 1);
        }

        if (slope_supported_ && !slope_name_bound_.isEmpty()) {
            auto slopePill = slope_name_bound_.toFloat().reduced(.35f);
            zlgui::glass::fillGlassSurface(g, slopePill, slopePill.getHeight() * .5f, .035f, .065f, .075f);
            const auto idx = slope_box_.getBox().getSelectedItemIndex();
            const auto label = idx >= 0 && idx < static_cast<int>(zlp::POrder::kChoices.size())
                ? zlp::POrder::kChoices[static_cast<size_t>(idx)] : juce::String{};
            g.setColour(zlgui::glass::textSecondary().withAlpha(.83f));
            g.setFont(juce::FontOptions(base_.getFontSize() * .53f));
            g.drawFittedText(label, slope_name_bound_, juce::Justification::centred, 1);
        }

        for (const auto& r : value_bounds_) {
            if (r.isEmpty()) continue;
            auto cell = r.toFloat().reduced(.35f);
            g.setColour(juce::Colour(228, 243, 255).withAlpha(.028f));
            g.fillRoundedRectangle(cell, cell.getHeight() * .34f);
            g.setColour(zlgui::glass::rim().withMultipliedAlpha(.13f));
            g.drawRoundedRectangle(cell, cell.getHeight() * .34f, .55f);
        }

        drawQuickActionGlyphs(g);

        auto power = bypass_button_.getBounds().toFloat().reduced(static_cast<float>(bypass_button_.getHeight()) * .26f);
        g.setColour(zlgui::glass::textPrimary().withAlpha(bypass_button_.getToggleState() ? .92f : .40f));
        juce::Path pp;
        pp.addCentredArc(power.getCentreX(), power.getCentreY(), power.getWidth()*.5f, power.getHeight()*.5f,
                         0.f, .72f, 5.56f, true);
        g.strokePath(pp, juce::PathStrokeType(1.25f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.drawLine(power.getCentreX(), power.getY(), power.getCentreX(), power.getCentreY(), 1.25f);
    }

    void FloatPopPanel::resized() {
        const auto font = base_.getFontSize();
        const auto padding = juce::jmax(4, juce::roundToInt(font * .30f));
        const auto rowH = juce::jmax(20, juce::roundToInt(font * 1.42f));
        const auto gap = juce::jmax(2, juce::roundToInt(font * .20f));

        auto b = getLocalBounds().reduced(padding);
        auto header = b.removeFromTop(rowH);

        ftype_box_.setBounds(header.removeFromLeft(rowH));
        filter_name_bound_ = header.removeFromLeft(juce::roundToInt(font * 3.45f));

        bypass_button_.setBounds(header.removeFromRight(rowH));
        solo_button_.setBounds(header.removeFromRight(rowH));
        dynamic_button_.setBounds(header.removeFromRight(rowH));
        header.removeFromRight(gap);

        const auto slopeW = juce::jmin(juce::roundToInt(font * 3.35f), header.getWidth());
        slope_name_bound_ = header.removeFromRight(slopeW);
        slope_box_.setBounds(slope_name_bound_);
        slope_box_.setVisible(slope_supported_);

        b.removeFromTop(gap);
        auto values = b.removeFromTop(rowH);
        const auto cell = juce::jmax(1, (values.getWidth() - 2 * gap) / 3);
        value_bounds_[0] = values.removeFromLeft(cell); values.removeFromLeft(gap);
        value_bounds_[1] = values.removeFromLeft(cell); values.removeFromLeft(gap);
        value_bounds_[2] = values;
        freq_slider_.setBounds(value_bounds_[0]);
        gain_slider_.setBounds(value_bounds_[1]);
        q_slider_.setBounds(value_bounds_[2]);
    }

    void FloatPopPanel::mouseUp(const juce::MouseEvent& event) {
        const auto typeArea = ftype_box_.getBounds().getUnion(filter_name_bound_);
        if (typeArea.contains(event.getPosition())) ftype_box_.getBox().showPopup();
    }

    void FloatPopPanel::updateBand() {
        if (base_.getSelectedBand() < zlp::kBandNum) {
            const auto s = std::to_string(base_.getSelectedBand());
            ftype_attachment_ = std::make_unique<zlgui::attachment::ComboBoxAttachment<true>>(ftype_box_.getBox(), p_ref_.parameters_, zlp::PFilterType::kID+s, updater_);
            slope_attachment_ = std::make_unique<zlgui::attachment::ComboBoxAttachment<true>>(slope_box_.getBox(), p_ref_.parameters_, zlp::POrder::kID+s, updater_);
            freq_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(freq_slider_.getSlider(), p_ref_.parameters_, zlp::PFreq::kID+s, updater_);
            gain_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(gain_slider_.getSlider(), p_ref_.parameters_, zlp::PGain::kID+s, updater_);
            q_attachment_ = std::make_unique<zlgui::attachment::SliderAttachment<true>>(q_slider_.getSlider(), p_ref_.parameters_, zlp::PQ::kID+s, updater_);
            updater_.updateComponents();
            filter_status_ptr_ = p_ref_.parameters_.getRawParameterValue(zlp::PFilterStatus::kID+s);
            filter_type_ptr_ = p_ref_.parameters_.getRawParameterValue(zlp::PFilterType::kID+s);
            slope_ptr_ = p_ref_.parameters_.getRawParameterValue(zlp::POrder::kID+s);
            dynamic_on_ptr_ = p_ref_.parameters_.getRawParameterValue(zlp::PDynamicON::kID+s);
            current_filter_type_ = current_slope_ = -1;
            updateFilterCapabilities();
            transform_initialized_ = false;
            setVisible(true);
            repaintCallBackSlow();
        } else {
            ftype_attachment_.reset(); slope_attachment_.reset(); freq_attachment_.reset(); gain_attachment_.reset(); q_attachment_.reset();
            filter_status_ptr_ = filter_type_ptr_ = slope_ptr_ = dynamic_on_ptr_ = nullptr;
            current_filter_type_ = current_slope_ = -1;
            dynamic_button_.getButton().setToggleState(false, juce::dontSendNotification);
            solo_button_.getButton().setToggleState(false, juce::dontSendNotification);
            stopTimer();
            setVisible(false);
        }
    }

    void FloatPopPanel::repaintCallBackSlow() {
        if (filter_status_ptr_ == nullptr) return;
        updater_.updateComponents();
        freq_slider_.updateDisplayValue(); gain_slider_.updateDisplayValue(); q_slider_.updateDisplayValue();
        bypass_button_.getButton().setToggleState(filter_status_ptr_->load(std::memory_order::relaxed) > 1.5f,
                                                   juce::dontSendNotification);
        dynamic_button_.getButton().setToggleState(dynamic_on_ptr_ != nullptr
            && dynamic_on_ptr_->load(std::memory_order::relaxed) > .5f, juce::dontSendNotification);
        solo_button_.getButton().setToggleState(base_.getSelectedBand() < zlp::kBandNum
            && base_.getSoloWholeIdx() == base_.getSelectedBand(), juce::dontSendNotification);
        updateFilterCapabilities();
        repaint();
    }

    void FloatPopPanel::updateFilterCapabilities() {
        if (filter_type_ptr_ == nullptr || slope_ptr_ == nullptr) return;
        const auto type = static_cast<int>(std::round(filter_type_ptr_->load(std::memory_order::relaxed)));
        const auto slope = static_cast<int>(std::round(slope_ptr_->load(std::memory_order::relaxed)));
        if (type == current_filter_type_ && slope == current_slope_) return;
        current_filter_type_ = type;
        current_slope_ = slope;
        slope_supported_ = type != static_cast<int>(zldsp::filter::kFlatTilt)
            && type != static_cast<int>(zldsp::filter::kFlatGain);
        const auto slope6 = type != static_cast<int>(zldsp::filter::kPeak)
            && type != static_cast<int>(zldsp::filter::kBandPass)
            && type != static_cast<int>(zldsp::filter::kNotch);
        if (!slope6 && slope_box_.getBox().getSelectedId() == 1)
            slope_box_.getBox().setSelectedId(2, juce::sendNotificationSync);
        slope_box_.getBox().setItemEnabled(1, slope6);
        slope_box_.setEditable(slope_supported_);
        slope_box_.setVisible(slope_supported_);

        const auto gainEnabled = type == static_cast<int>(zldsp::filter::kPeak)
            || type == static_cast<int>(zldsp::filter::kLowShelf)
            || type == static_cast<int>(zldsp::filter::kHighShelf)
            || type == static_cast<int>(zldsp::filter::kTiltShelf)
            || type == static_cast<int>(zldsp::filter::kFlatTilt)
            || type == static_cast<int>(zldsp::filter::kFlatGain);
        gain_slider_.setEditable(gainEnabled);
        q_slider_.setEditable(slope_supported_ && slope != 0);
        dynamic_button_.getButton().setEnabled(gainEnabled);
        dynamic_button_.setAlpha(gainEnabled ? 1.f : .34f);
        resized();
        repaint();
    }

    int FloatPopPanel::getIdealWidth() const {
        return juce::jmax(210, juce::roundToInt(base_.getFontSize() * 15.2f));
    }

    int FloatPopPanel::getIdealHeight() const {
        return juce::jmax(58, juce::roundToInt(base_.getFontSize() * 4.05f));
    }

    void FloatPopPanel::updatePosition(const juce::Point<float> position, const juce::Point<float> target_position) {
        position_ = position;
        target_position_ = target_position;
        updateTransformationTarget();
    }

    void FloatPopPanel::setTargetVisible(const bool visible) { target_visible_ = visible; }

    void FloatPopPanel::updateFloatingBound(const juce::Rectangle<float> bound) {
        floating_bound_ = bound;
        updateTransformationTarget();
    }

    void FloatPopPanel::updateTransformationTarget() {
        if (floating_bound_.isEmpty() || !isVisible()) return;
        const auto width = static_cast<float>(getIdealWidth());
        const auto height = static_cast<float>(getIdealHeight());
        const auto safeMargin = juce::jmax(6.f, base_.getFontSize() * .55f);
        const auto gap = juce::jmax(8.f, base_.getFontSize() * .58f);
        const auto hysteresis = juce::jmax(9.f, base_.getFontSize() * .70f);
        const auto safe = floating_bound_.reduced(safeMargin);
        const auto aboveY = position_.y - gap - height;
        const auto belowY = position_.y + gap;
        const auto aboveFits = aboveY >= safe.getY();
        const auto belowFits = belowY + height <= safe.getBottom();

        if (placement_ == Placement::above) {
            if (!aboveFits && belowFits) placement_ = Placement::below;
        } else if ((!belowFits && aboveFits) || (aboveFits && aboveY >= safe.getY() + hysteresis)) {
            placement_ = Placement::above;
        }

        const auto maxX = juce::jmax(safe.getX(), safe.getRight() - width);
        auto x = juce::jlimit(safe.getX(), maxX, position_.x - width * .5f);
        auto y = placement_ == Placement::above ? aboveY : belowY;
        y = juce::jlimit(safe.getY(), juce::jmax(safe.getY(), safe.getBottom() - height), y);
        target_x_ = x;
        target_y_ = y;

        if (!transform_initialized_) {
            current_x_ = target_x_;
            current_y_ = target_y_;
            transform_initialized_ = true;
            applyCurrentTransform();
            return;
        }
        if (std::abs(current_x_ - target_x_) > .25f || std::abs(current_y_ - target_y_) > .25f)
            startTimerHz(60);
    }

    void FloatPopPanel::timerCallback() {
        constexpr float response = .34f;
        current_x_ += (target_x_ - current_x_) * response;
        current_y_ += (target_y_ - current_y_) * response;
        if (std::abs(current_x_ - target_x_) < .20f && std::abs(current_y_ - target_y_) < .20f) {
            current_x_ = target_x_;
            current_y_ = target_y_;
            stopTimer();
        }
        applyCurrentTransform();
    }

    void FloatPopPanel::applyCurrentTransform() {
        setTransform(juce::AffineTransform::translation(current_x_, current_y_));
    }
}
