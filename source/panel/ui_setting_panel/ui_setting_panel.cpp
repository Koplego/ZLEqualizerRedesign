// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#include "ui_setting_panel.hpp"
#include "../../gui/glass_tokens.hpp"
#include "BinaryData.h"

namespace zlpanel {
    UISettingPanel::UISettingPanel(PluginProcessor& p, zlgui::UIBase& base) :
        p_ref_(p), base_(base),
        colour_panel_(p, base),
        control_panel_(p, base),
        other_panel_(p, base),
        credit_panel_(base),
        background_(base),
        version_text_laf_(base),
        version_text_({},
                      juce::String(ZLEQUALIZER_CURRENT_VERSION) + " " + juce::String(ZLEQUALIZER_CURRENT_HASH)),
        tab_bar_(base),
        view_port_(base),
        save_drawable_(juce::Drawable::createFromImageData(BinaryData::save_svg,
                                                           BinaryData::save_svgSize)),
        close_drawable_(juce::Drawable::createFromImageData(BinaryData::close_svg,
                                                            BinaryData::close_svgSize)),
        reset_drawable_(juce::Drawable::createFromImageData(BinaryData::reset_settings_svg,
                                                            BinaryData::reset_settings_svgSize)),
        folder_open_drawable_(juce::Drawable::createFromImageData(BinaryData::folder_open_svg,
                                                                  BinaryData::folder_open_svgSize)),
        save_button_(base_, save_drawable_.get()),
        close_button_(base_, close_drawable_.get()),
        reset_button_(base_, reset_drawable_.get()),
        folder_open_button_(base_, folder_open_drawable_.get()) {
        juce::ignoreUnused(p_ref_);
        setOpaque(false);
        setInterceptsMouseClicks(true, true);
        base_.setPanelProperty(zlgui::PanelSettingIdx::kUISettingPanel, false);

        background_.setBufferedToImage(true);
        addAndMakeVisible(background_);
        version_text_laf_.setFontScale(.68f);
        version_text_.setJustificationType(juce::Justification::centred);
        version_text_.setLookAndFeel(&version_text_laf_);
        version_text_.setAlpha(.5f);
        version_text_.setInterceptsMouseClicks(false, false);
        version_text_.setBufferedToImage(true);
        addAndMakeVisible(version_text_);
        addAndMakeVisible(tab_bar_);
        addAndMakeVisible(view_port_);

        for (auto* button : {&save_button_, &reset_button_, &close_button_, &folder_open_button_}) {
            button->setImageAlpha(0.f, 0.f, 0.f, 0.f);
            button->setBufferedToImage(true);
            addAndMakeVisible(button);
        }
        save_button_.getButton().onClick = [this]() {
            saveCurrentPanel();
        };
        reset_button_.getButton().onClick = [this]() {
            resetCurrentPanel();
        };
        close_button_.getButton().onClick = [this]() {
            base_.setPanelProperty(zlgui::PanelSettingIdx::kUISettingPanel, false);
        };
        folder_open_button_.getButton().onClick = []() {
            const juce::File ui_file =
                juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                .getChildFile("ZL Audio").getChildFile(JucePlugin_Name).getChildFile("ui.xml");
            if (ui_file.existsAsFile()) {
                ui_file.revealToUser();
            }
        };

        tab_bar_.onTabSelected = [this](const int index) {
            view_positions_[static_cast<size_t>(current_panel_idx_)] = view_port_.getViewPosition();
            current_panel_idx_ = static_cast<PanelIdx>(index);
            changeDisplayPanel();
        };

        changeDisplayPanel();
    }

    UISettingPanel::~UISettingPanel() {
        view_port_.setViewedComponent(nullptr, 0);
        version_text_.setLookAndFeel(nullptr);
    }

    void UISettingPanel::resized() {
        view_positions_[static_cast<size_t>(current_panel_idx_)] = view_port_.getViewPosition();

        const auto font_size = base_.getFontSize();
        const auto padding = juce::roundToInt(font_size * .72f);
        const auto button_size = juce::roundToInt(font_size * 2.f);
        const auto header_height = juce::roundToInt(font_size * 2.35f);
        const auto footer_height = button_size + padding;
        const auto content_top_margin = juce::roundToInt(font_size * .36f);

        auto bounds = getLocalBounds().reduced(padding);
        auto header = bounds.removeFromTop(header_height);
        auto footer = bounds.removeFromBottom(footer_height);
        footer.removeFromTop(padding);

        settings_title_bound_ = header.removeFromLeft(juce::roundToInt(font_size * 7.2f));
        header.removeFromLeft(juce::roundToInt(font_size * .45f));
        tab_bar_.setBounds(header.reduced(0, juce::roundToInt(font_size * .20f)));

        bounds.removeFromTop(content_top_margin);
        view_port_.setBounds(bounds);

        const auto get_footer_cell = [&footer](const int index) {
            const auto left = footer.getX() + footer.getWidth() * index / 5;
            const auto right = footer.getX() + footer.getWidth() * (index + 1) / 5;
            return juce::Rectangle<int>{left, footer.getY(), right - left, footer.getHeight()};
        };
        version_text_.setBounds(get_footer_cell(0));
        folder_open_button_.setBounds(get_footer_cell(1).withSizeKeepingCentre(button_size, button_size));
        folder_open_button_.getButton().setEdgeIndent(static_cast<int>(std::round(font_size * .175f)));
        reset_button_.setBounds(get_footer_cell(2).withSizeKeepingCentre(button_size, button_size));
        save_button_.setBounds(get_footer_cell(3).withSizeKeepingCentre(button_size, button_size));
        close_button_.setBounds(get_footer_cell(4).withSizeKeepingCentre(button_size, button_size));

        background_.setBounds(getLocalBounds());
        background_.setSurfaceBounds({tab_bar_.getBounds(), view_port_.getBounds()});

        const auto parent_width = getParentComponent() != nullptr ? getParentComponent()->getWidth() : getWidth();
        other_panel_.setParentWidth(parent_width);
        changeDisplayPanel();
    }

    void UISettingPanel::paintOverChildren(juce::Graphics& g) {
        const auto font = base_.getFontSize();
        g.setColour(zlgui::glass::textPrimary().withAlpha(.92f));
        g.setFont(juce::FontOptions(font * 1.02f));
        g.drawText("Settings", settings_title_bound_, juce::Justification::centredLeft, false);

        const auto drawActionSurface = [&g, font](const juce::Rectangle<int>& ib) {
            auto r = ib.toFloat().reduced(1.f);
            zlgui::glass::fillGlassSurface(g, r, r.getHeight() * .5f, .055f, .10f, .12f);
            return r.reduced(font * .48f);
        };
        const auto colour = zlgui::glass::textPrimary().withAlpha(.62f);
        const auto stroke = juce::PathStrokeType(1.15f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded);

        // Open settings file.
        {
            const auto r = drawActionSurface(folder_open_button_.getBounds());
            juce::Path p;
            p.startNewSubPath(r.getX(), r.getY() + r.getHeight() * .30f);
            p.lineTo(r.getX() + r.getWidth() * .38f, r.getY() + r.getHeight() * .30f);
            p.lineTo(r.getX() + r.getWidth() * .48f, r.getY() + r.getHeight() * .16f);
            p.lineTo(r.getRight(), r.getY() + r.getHeight() * .16f);
            p.lineTo(r.getRight() - r.getWidth() * .10f, r.getBottom());
            p.lineTo(r.getX(), r.getBottom()); p.closeSubPath();
            g.setColour(colour); g.strokePath(p, stroke);
        }
        // Reset.
        {
            const auto r = drawActionSurface(reset_button_.getBounds());
            juce::Path p;
            p.addCentredArc(r.getCentreX(), r.getCentreY(), r.getWidth() * .42f, r.getHeight() * .42f,
                            0.f, .35f, 5.2f, true);
            p.startNewSubPath(r.getX() + r.getWidth() * .12f, r.getY() + r.getHeight() * .42f);
            p.lineTo(r.getX() + r.getWidth() * .12f, r.getY() + r.getHeight() * .12f);
            p.lineTo(r.getX() + r.getWidth() * .42f, r.getY() + r.getHeight() * .18f);
            g.setColour(colour); g.strokePath(p, stroke);
        }
        // Apply/save.
        {
            const auto r = drawActionSurface(save_button_.getBounds());
            juce::Path p;
            p.startNewSubPath(r.getX() + r.getWidth() * .12f, r.getCentreY());
            p.lineTo(r.getX() + r.getWidth() * .40f, r.getBottom() - r.getHeight() * .16f);
            p.lineTo(r.getRight() - r.getWidth() * .08f, r.getY() + r.getHeight() * .14f);
            g.setColour(colour); g.strokePath(p, juce::PathStrokeType(1.55f, juce::PathStrokeType::curved,
                                                                      juce::PathStrokeType::rounded));
        }
        // Close.
        {
            const auto r = drawActionSurface(close_button_.getBounds());
            g.setColour(colour);
            g.drawLine(juce::Line<float>{r.getTopLeft(), r.getBottomRight()}, 1.25f);
            g.drawLine(juce::Line<float>{r.getTopRight(), r.getBottomLeft()}, 1.25f);
        }
    }

    void UISettingPanel::loadSetting() {
        colour_panel_.loadSetting();
        control_panel_.loadSetting();
        other_panel_.loadSetting();
    }

    void UISettingPanel::flushPendingScroll() {
        view_port_.flushPendingScroll();
    }

    int UISettingPanel::getIdealWidth() const {
        return juce::roundToInt(base_.getFontSize() * 42.f);
    }

    int UISettingPanel::getIdealHeight() const {
        return juce::roundToInt(base_.getFontSize() * 27.f);
    }

    void UISettingPanel::changeDisplayPanel() {
        juce::Component* panel = nullptr;
        auto ideal_height = 0;
        switch (current_panel_idx_) {
        case kColourP: {
            panel = &colour_panel_;
            ideal_height = colour_panel_.getIdealHeight();
            break;
        }
        case kControlP: {
            panel = &control_panel_;
            ideal_height = control_panel_.getIdealHeight();
            break;
        }
        case kOtherP: {
            panel = &other_panel_;
            ideal_height = other_panel_.getIdealHeight();
            break;
        }
        case kCreditP: {
            panel = &credit_panel_;
            ideal_height = credit_panel_.getIdealHeight();
            break;
        }
        }
        tab_bar_.setSelectedIndex(static_cast<int>(current_panel_idx_));
        view_port_.setViewedComponent(panel, ideal_height);
        view_port_.setViewPosition(view_positions_[static_cast<size_t>(current_panel_idx_)]);
        updateActionButtonStates();
    }

    void UISettingPanel::saveCurrentPanel() {
        switch (current_panel_idx_) {
        case kColourP:
            colour_panel_.saveSetting();
            break;
        case kControlP:
            control_panel_.saveSetting();
            break;
        case kOtherP:
            other_panel_.saveSetting();
            break;
        case kCreditP:
            return;
        }
        base_.setPanelProperty(zlgui::kUISettingChanged,
                               !static_cast<bool>(base_.getPanelProperty(zlgui::kUISettingChanged)));
    }

    void UISettingPanel::resetCurrentPanel() {
        switch (current_panel_idx_) {
        case kColourP:
            colour_panel_.resetSetting();
            break;
        case kControlP:
            control_panel_.resetSetting();
            break;
        case kOtherP:
            other_panel_.resetSetting();
            break;
        case kCreditP:
            return;
        }
    }

    void UISettingPanel::updateActionButtonStates() {
        const auto enabled = current_panel_idx_ != kCreditP;
        for (auto* button : {&save_button_, &reset_button_}) {
            button->setAlpha(enabled ? 1.f : .25f);
            button->setInterceptsMouseClicks(enabled, enabled);
        }
    }

    void UISettingPanel::lookAndFeelChanged() {
        background_.repaint();
        version_text_.repaint();
        tab_bar_.repaint();
        view_port_.repaint();
        repaint();
    }

    void UISettingPanel::visibilityChanged() {
        if (isVisible()) {
            loadSetting();
        }
    }
}
