// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "compact_combobox_look_and_feel.hpp"

namespace zlgui::combobox {
    class CompactCombobox final : public juce::Component,
                                  public juce::SettableTooltipClient,
                                  private juce::Timer {
    public:
        CompactCombobox(const juce::StringArray& choices, UIBase& base,
                        const juce::String& tooltip_text = "",
                        const std::vector<juce::String>& item_labels = {});

        CompactCombobox(const std::vector<std::unique_ptr<juce::Drawable>>& icons, UIBase& base,
                        const juce::String& tooltip_text = "",
                        const std::vector<juce::String>& item_labels = {});

        ~CompactCombobox() override;

        void paint(juce::Graphics& g) override;

        void resized() override;

        void mouseUp(const juce::MouseEvent& event) override;

        void mouseDown(const juce::MouseEvent& event) override;

        void mouseDrag(const juce::MouseEvent& event) override;

        void mouseEnter(const juce::MouseEvent& event) override;

        void mouseExit(const juce::MouseEvent& event) override;

        void mouseMove(const juce::MouseEvent& event) override;

        void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

        inline void setEditable(const bool x) {
            setAlpha(x ? 1.f : .25f);
            setInterceptsMouseClicks(x, false);
        }

        // Some Glass controls use CompactCombobox only as an invisible interaction/
        // parameter host while drawing their own face in the parent component. Setting
        // those wrappers to ~1% alpha used to leave the JUCE ComboBox label visible as
        // a faint truncated string (for example "1...") above the custom slope chip.
        // Keep the wrapper's requested alpha for hit-testing/layout, but completely
        // suppress the inner JUCE face when the wrapper is intentionally near-invisible.
        inline void setAlpha(const float alpha) {
            juce::Component::setAlpha(alpha);
            combo_box_.setAlpha(alpha <= .011f ? 0.f : 1.f);
        }

        inline juce::ComboBox& getBox() {
            return combo_box_;
        }

        inline CompactComboboxLookAndFeel& getLAF() {
            return box_laf_;
        }

        void setScrollEnabled(const bool is_scroll_enabled) {
            is_scroll_enabled_ = is_scroll_enabled;
        }

    private:
        void timerCallback() override;

        zlgui::UIBase& base_;
        CompactComboboxLookAndFeel box_laf_;
        juce::ComboBox combo_box_;

        bool is_scroll_enabled_{false};
        float cumulative_y_{0.f};
        float hover_progress_{0.f};
        float hover_target_{0.f};
    };
}
