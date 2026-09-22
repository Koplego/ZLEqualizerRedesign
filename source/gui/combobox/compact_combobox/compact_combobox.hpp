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

        // A few Glass controls deliberately use CompactCombobox only as an interaction/
        // parameter host and draw their visible face in the parent. Historically those
        // callers signalled that by setting the wrapper to ~1% alpha. Once that happens,
        // remember the intent permanently: later setEditable()/state refreshes are allowed
        // to restore the wrapper's hit target opacity, but must never resurrect JUCE's own
        // ComboBox label/arrow underneath the custom Glass face.
        inline void setAlpha(const float alpha) {
            if (alpha <= .011f)
                face_suppressed_ = true;

            juce::Component::setAlpha(alpha);
            combo_box_.setAlpha(face_suppressed_ ? 0.f : 1.f);
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
        bool face_suppressed_{false};
        float cumulative_y_{0.f};
        float hover_progress_{0.f};
        float hover_target_{0.f};
    };
}
