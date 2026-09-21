// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../interface_definitions.hpp"
#include "../../glass_tokens.hpp"

namespace zlgui::combobox {
    class CompactComboboxLookAndFeel : public juce::LookAndFeel_V4 {
    public:
        explicit CompactComboboxLookAndFeel(UIBase& base, bool align_label = true) :
            base_(base), align_label_(align_label) {
            setColour(juce::PopupMenu::backgroundColourId, juce::Colours::transparentBlack);
        }

        void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown, int, int, int, int,
                          juce::ComboBox& box) override {
            const auto box_bound = juce::Rectangle<float>(0, 0,
                                                          static_cast<float>(width),
                                                          static_cast<float>(height));
            const auto corner_size = box_bound.getHeight() * .5f;
            const auto active = isButtonDown || box.isPopupActive();
            glass::fillGlassSurface(g, box_bound.reduced(.5f), corner_size,
                                    active ? .115f : .045f + .035f * box_alpha_,
                                    active ? .175f : .070f + .040f * box_alpha_,
                                    active ? .19f : .075f + .065f * box_alpha_);

            if (!icons_.empty() && box.getSelectedItemIndex() >= 0) {
                const auto fig = icons_[static_cast<size_t>(box.getSelectedItemIndex())]->createCopy();
                fig->replaceColour(juce::Colours::black, glass::textPrimary());
                fig->drawWithin(g, box.getLocalBounds().toFloat().reduced(box_bound.getHeight() * .25f),
                                juce::RectanglePlacement::centred, .86f);
            } else {
                auto arrow = box_bound.withLeft(box_bound.getRight() - box_bound.getHeight() * .70f);
                const auto centre = arrow.getCentre();
                juce::Path chevron;
                chevron.startNewSubPath(centre.x - 3.f, centre.y - 1.f);
                chevron.lineTo(centre.x, centre.y + 2.f);
                chevron.lineTo(centre.x + 3.f, centre.y - 1.f);
                g.setColour(glass::textSecondary().withAlpha(.52f + .18f * box_alpha_));
                g.strokePath(chevron, juce::PathStrokeType(.95f, juce::PathStrokeType::curved,
                                                           juce::PathStrokeType::rounded));
            }
        }

        void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override {
            label.setBounds(box.getLocalBounds());
        }

        void drawLabel(juce::Graphics& g, juce::Label& label) override {
            if (!icons_.empty()) return;
            g.setColour(glass::textPrimary().withAlpha(.88f));
            g.setFont(base_.getFontSize() * font_scale_);
            auto bound = label.getLocalBounds().toFloat();
            const auto inset = align_label_
                ? juce::jmin(juce::jmax(base_.getFontSize() * .70f, padding_), bound.getWidth() * .14f)
                : 0.f;
            bound.removeFromLeft(inset);
            bound.removeFromRight(juce::jmin(juce::jmax(base_.getFontSize() * 1.40f, inset * .55f),
                                             bound.getWidth() * .38f));
            g.drawText(label.getText(), bound, label_justification_);
        }

        void drawPopupMenuBackground(juce::Graphics& g, const int width, const int height) override {
            const auto box_bound = juce::Rectangle<float>(0, 0, static_cast<float>(width),
                                                          static_cast<float>(height));
            const auto corner_size = juce::jmax(base_.getFontSize() * .70f, 8.f);
            const auto body = box_bound.reduced(1.0f);

            // Popup menus live in their own host window, so give the rounded body a nearly
            // opaque base before applying the shared glass material. This prevents graph
            // details from bleeding through without bringing back rectangular black corners.
            g.setColour(glass::canvasBottom().withAlpha(.965f));
            g.fillRoundedRectangle(body, corner_size);
            glass::fillGlassSurface(g, body, corner_size, .105f, .18f, .23f);
        }

        void getIdealPopupMenuItemSize(const juce::String& text, const bool isSeparator, int standardMenuItemHeight,
                                       int& ideal_width, int& ideal_height) override {
            juce::ignoreUnused(isSeparator, standardMenuItemHeight);
            const auto font = juce::Font(base_.getFontSize() * font_scale_);
            const auto text_width = juce::roundToInt(juce::GlyphArrangement::getStringWidth(font, text));
            ideal_width = juce::jmax(item_width_, text_width + juce::roundToInt(base_.getFontSize() * 3.55f));
            ideal_height = juce::jmax(item_height_, juce::roundToInt(base_.getFontSize() * 1.86f));
        }

        void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                               const bool isSeparator, const bool isActive,
                               const bool isHighlighted, const bool isTicked, const bool hasSubMenu,
                               const juce::String& text,
                               const juce::String& shortcutKeyText, const juce::Drawable* icon,
                               const juce::Colour* const textColourToUse) override {
            juce::ignoreUnused(hasSubMenu, shortcutKeyText, textColourToUse);
            if (isSeparator) {
                auto line = area.toFloat().withHeight(1.f).withCentre(area.toFloat().getCentre());
                line.reduce(base_.getFontSize() * 1.0f, 0.f);
                g.setColour(glass::rim().withMultipliedAlpha(.36f));
                g.fillRect(line);
                return;
            }

            const auto alpha = !isActive ? .28f : (isHighlighted || isTicked ? .98f : .78f);
            auto card = area.toFloat().reduced(base_.getFontSize() * .20f, base_.getFontSize() * .10f);
            if ((isHighlighted || isTicked) && isActive) {
                g.setColour(juce::Colour(132, 187, 229).withAlpha(isTicked ? .16f : .095f));
                g.fillRoundedRectangle(card, juce::jmax(5.f, base_.getFontSize() * .44f));
                if (isTicked) {
                    g.setColour(glass::rim().withMultipliedAlpha(.60f));
                    g.drawRoundedRectangle(card, juce::jmax(5.f, base_.getFontSize() * .44f), .65f);
                }
            }

            auto content = area.reduced(juce::roundToInt(base_.getFontSize() * .78f), 0);
            if (icon != nullptr) {
                const auto icon_size = juce::jmin(content.getHeight(), juce::roundToInt(base_.getFontSize() * 1.82f));
                auto icon_area = content.removeFromLeft(icon_size).toFloat().reduced(base_.getFontSize() * .24f);
                const auto fig = icon->createCopy();
                fig->replaceColour(juce::Colours::black, glass::textPrimary());
                fig->drawWithin(g, icon_area, juce::RectanglePlacement::centred, alpha);
                if (text.isNotEmpty()) content.removeFromLeft(juce::roundToInt(base_.getFontSize() * .48f));
            }

            if (text.isNotEmpty()) {
                g.setColour(glass::textPrimary().withAlpha(alpha));
                g.setFont(base_.getFontSize() * font_scale_);
                g.drawText(text, content, item_justification_, true);
            }
        }

        int getMenuWindowFlags() override { return 0; }
        int getPopupMenuBorderSize() override { return 0; }

        inline void setFontScale(const float x) { font_scale_ = x; }
        float getFontScale() const { return font_scale_; }

        void setOption(const juce::PopupMenu::Options& x) { option_ = x; }

        juce::PopupMenu::Options getOptionsForComboBoxPopupMenu(juce::ComboBox& box, juce::Label& label) override {
            popup_target_ = &box;
            popup_placement_known_ = false;
            popup_attached_ = false;
            popup_uses_opaque_fallback_ = false;
            auto option = option_;
            if (option.getParentComponent() == nullptr) {
                if (juce::JUCEApplicationBase::isStandaloneApp()) {
                    option = option.withParentComponent(box.getTopLevelComponent());
                } else if (auto* top = box.getTopLevelComponent()) {
                    option = option.withParentComponent(top);
                }
            }
            if (option.getMinimumWidth() == 0) {
                option = option.withMinimumWidth(juce::jmax(box.getWidth(), juce::roundToInt(base_.getFontSize() * 8.4f)));
            }
            option = option.withMinimumNumColumns(1).withMaximumNumColumns(1);
            return option.withTargetComponent(&box)
                         .withInitiallySelectedItem(box.getSelectedId())
                         .withStandardItemHeight(juce::jmax(label.getHeight(), juce::roundToInt(base_.getFontSize() * 1.86f)));
        }

        void preparePopupMenuWindow(juce::Component& new_window) override {
            new_window.setOpaque(false);
            popup_uses_opaque_fallback_ = false;

            if (popup_target_ == nullptr) return;

            const auto target_bounds = popup_target_->getScreenBounds();
            const auto popup_bounds = new_window.getScreenBounds();
            const auto distance_below = std::abs(popup_bounds.getY() - target_bounds.getBottom());
            const auto distance_above = std::abs(target_bounds.getY() - popup_bounds.getBottom());
            popup_below_box_ = distance_below <= distance_above;
            popup_attached_ = std::min(distance_below, distance_above) <= 2;
            popup_placement_known_ = true;
            popup_target_->repaint();
        }

        void drawPopupMenuColumnSeparatorWithOptions(juce::Graphics& g, const juce::Rectangle<int>& bounds,
            const juce::PopupMenu::Options&) override {
            auto bound = bounds.toFloat();
            bound = bound.withSizeKeepingCentre(bound.getWidth() * .4f, bound.getHeight());
            g.setColour(glass::rim().withMultipliedAlpha(.38f));
            g.fillRect(bound);
        }

        int getPopupMenuColumnSeparatorWidthWithOptions(const juce::PopupMenu::Options&) override {
            return static_cast<int>(base_.getFontSize() * .35f);
        }

        void setBoxAlpha(const float x) { box_alpha_ = x; }
        void setLabelJustification(const juce::Justification j) { label_justification_ = j; }
        void setAlignLabel(const bool should_align) { align_label_ = should_align; }
        void setItemJustification(const juce::Justification j) { item_justification_ = j; }
        void setPadding(const float padding) { padding_ = padding; }

        void setIcons(const std::vector<std::unique_ptr<juce::Drawable>>& icons) {
            icons_.clear();
            for (size_t i = 0; i < icons.size(); ++i) icons_.emplace_back(icons[i]->createCopy());
        }

        void setItemSize(const int width, const int height) {
            item_width_ = width;
            item_height_ = height;
        }

    private:
        int item_width_{0}, item_height_{0};
        float font_scale_{1.0f}, box_alpha_{0.f};
        float padding_{0.f};
        juce::Justification label_justification_{juce::Justification::centred};
        juce::Justification item_justification_{juce::Justification::centredLeft};
        juce::PopupMenu::Options option_{};
        juce::Component::SafePointer<juce::ComboBox> popup_target_;
        bool popup_placement_known_{false};
        bool popup_attached_{false};
        bool popup_below_box_{true};
        bool popup_uses_opaque_fallback_{false};

        UIBase& base_;
        bool align_label_{true};
        std::vector<std::unique_ptr<juce::Drawable>> icons_;
    };
}
