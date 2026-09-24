// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "control_panel/control_panel.hpp"
#include "control_panel/extra_dynamic_panel.hpp"
#include "curve_panel/curve_panel.hpp"
#include "glass_footer_panel.hpp"
#include "preset_browser/preset_browser.hpp"
#include "top_panel/top_panel.hpp"
#include "ui_setting_panel/ui_setting_panel.hpp"

namespace zlpanel {
    class MainPanel final : public juce::Component,
                            private juce::ValueTree::Listener,
                            private juce::Timer {
    public:
        explicit MainPanel(PluginProcessor& p, zlgui::UIBase& base, multilingual::TooltipLanguage language);

        ~MainPanel() override;

        void paint(juce::Graphics& g) override;
        void paintOverChildren(juce::Graphics& g) override;
        void resized() override;

        void repaintCallBack(double time_stamp);
        void startThreads();
        void stopThreads();

        ControlPanel& getControlPanel() { return control_panel_; }
        OutputPanel& getOutputPanel() { return curve_panel_.getOutputPanel(); }

    private:
        static constexpr float kHoWMin = 0.33f;

        class GlobalScrim final : public juce::Component {
        public:
            GlobalScrim() {
                setOpaque(false);
                setInterceptsMouseClicks(true, true);
            }

            void paint(juce::Graphics& g) override {
                const auto b = getLocalBounds().toFloat();
                g.setColour(juce::Colour(3, 12, 21).withAlpha(.78f));
                g.fillRoundedRectangle(b, 12.f);
                juce::ColourGradient vignette(juce::Colours::transparentBlack,
                                               b.getCentreX(), b.getCentreY(),
                                               juce::Colour(1, 7, 13).withAlpha(.48f),
                                               b.getX(), b.getY(), true);
                g.setGradientFill(vignette);
                g.fillRoundedRectangle(b, 12.f);
            }
        };

        PluginProcessor& p_ref_;
        zlgui::UIBase& base_;
        juce::Image backdrop_image_;
        juce::Image graph_material_image_;
        void paintGlassBackdrop(juce::Graphics& g);
        multilingual::TooltipHelper tooltip_helper_;

        RefreshHandler refresh_handler_;
        double previous_time_stamp_{-1.0};
        double refresh_rate_{-1.0};

        CurvePanel curve_panel_;
        GlobalScrim overlay_scrim_;
        ControlPanel control_panel_;
        ExtraDynamicPanel extra_dynamic_panel_;
        TopPanel top_panel_;
        GlassFooterPanel footer_panel_;
        PresetBrowser preset_browser_;
        UISettingPanel ui_setting_panel_;

        zlgui::tooltip::TooltipLookAndFeel tooltip_laf_;
        std::unique_ptr<zlgui::tooltip::TooltipWindow> tooltip_window_;

        size_t c_band_{zlp::kBandNum};
        double c_sample_rate_{0.};
        void toggleSettingsSheet();
        void closeGlobalOverlaysExcept(zlgui::PanelSettingIdx keep);
        void closeGlobalSheetsForUtility(zlgui::PanelSettingIdx utility);
        void updateOverlayState();

        void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property) override;
        void timerCallback() override;
        void repaintCallBackSlow();
    };
}
