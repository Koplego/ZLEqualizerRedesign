// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#include "scale_label_panel.hpp"
#include "scale_panel_layout.hpp"

namespace zlpanel {
    ScaleLabelPanel::ScaleLabelPanel(PluginProcessor& p,
                                     zlgui::UIBase& base,
                                     const multilingual::TooltipHelper& tooltip_helper) :
        base_(base) {
        juce::ignoreUnused(p, tooltip_helper);
        setInterceptsMouseClicks(false, false);
    }

    void ScaleLabelPanel::paint(juce::Graphics& g) {
        if (c_eq_max_idx_ < 0) {
            return;
        }

        const auto bound = getLocalBounds().toFloat();
        const auto unit_height = getUnitHeight();
        const float label_height = base_.getFontSize() * 1.1f;
        const float y0 = base_.getFontSize() * kDraggerScale - label_height * .5f;
        const auto eq_unit = base_.getCurveDBScale(static_cast<size_t>(c_eq_max_idx_)) / 3.f;

        g.setFont(base_.getFontSize() * 1.08f);
        g.setColour(base_.getTextColour().withAlpha(.66f));

        // Glass EQ deliberately shows only the EQ scale on the graph. The analyzer's
        // own top/floor controls remain available in the Analyzer panel, but removing
        // the second number column prevents the cramped "12 0 / 8 -10" look.
        for (int i = 1; i < 7; ++i) {
            const auto eq_value = (3.f - static_cast<float>(i)) * eq_unit;
            const auto label_bound = juce::Rectangle<float>(
                0.f, y0 + static_cast<float>(i) * unit_height, bound.getWidth() - base_.getFontSize() * .35f, label_height);
            if (std::abs(std::round(eq_value) - eq_value) < 0.01f) {
                const auto rounded = static_cast<int>(std::round(eq_value));
                const auto label = rounded > 0 ? juce::String("+") + juce::String(rounded) : juce::String(rounded);
                g.drawText(label, label_bound, juce::Justification::centredRight, false);
            }
        }
    }

    void ScaleLabelPanel::setScaleIdx(const int eq_max_idx, const int fft_top_idx, const int fft_min_idx) {
        if (eq_max_idx != c_eq_max_idx_ || fft_top_idx != c_fft_top_idx_ || fft_min_idx != c_fft_min_idx_) {
            c_eq_max_idx_ = eq_max_idx;
            c_fft_top_idx_ = fft_top_idx;
            c_fft_min_idx_ = fft_min_idx;
            repaint();
        }
    }

    float ScaleLabelPanel::getUnitHeight() const {
        const auto bound = getLocalBounds().toFloat();
        return (bound.getHeight() - 2.f * base_.getFontSize() * kDraggerScale
            - static_cast<float>(getBottomAreaHeight(base_.getFontSize()))) / 6.f;
    }
}
