// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.

#pragma once

#include "../../../../PluginProcessor.hpp"
#include "../../../../gui/gui.hpp"
#include "../../../helper/helper.hpp"
#include "../../../multilingual/tooltip_helper.hpp"
#include "../../../background/panel_background.hpp"

namespace zlpanel {
    // The node-attached surface is intentionally small.  It owns only the controls
    // needed while shaping a curve; deeper per-band editing belongs to BandHubPanel.
    class FloatPopPanel final : public juce::Component {
    public:
        explicit FloatPopPanel(PluginProcessor& p, zlgui::UIBase& base,
                               const multilingual::TooltipHelper& tooltip_helper);
        ~FloatPopPanel() override = default;

        void paintOverChildren(juce::Graphics& g) override;
        void resized() override;
        void mouseUp(const juce::MouseEvent& event) override;
        void repaintCallBackSlow();
        void updateBand();

        int getIdealHeight() const;
        int getIdealWidth() const;

        void updatePosition(juce::Point<float> position,
                            juce::Point<float> target_position);
        void setTargetVisible(bool is_target_visible);
        void updateFloatingBound(juce::Rectangle<float> bound);

    private:
        PluginProcessor& p_ref_;
        zlgui::UIBase& base_;
        zlgui::attachment::ComponentUpdater updater_{};
        PanelBackground control_background_;

        const std::unique_ptr<juce::Drawable> bypass_drawable_;
        zlgui::button::ClickButton bypass_button_;
        std::atomic<float>* filter_status_ptr_{nullptr};

        zlgui::combobox::CompactCombobox ftype_box_;
        std::unique_ptr<zlgui::attachment::ComboBoxAttachment<true>> ftype_attachment_;
        zlgui::combobox::CompactCombobox slope_box_;
        std::unique_ptr<zlgui::attachment::ComboBoxAttachment<true>> slope_attachment_;

        zlgui::slider::CompactLinearSlider<false, false, false> freq_slider_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> freq_attachment_;
        zlgui::slider::CompactLinearSlider<false, false, false> gain_slider_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> gain_attachment_;
        zlgui::slider::CompactLinearSlider<false, false, false> q_slider_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> q_attachment_;

        std::atomic<float>* filter_type_ptr_{nullptr};
        std::atomic<float>* slope_ptr_{nullptr};
        bool slope_supported_{true};
        int current_filter_type_{-1};
        int current_slope_{-1};

        juce::Rectangle<int> filter_name_bound_{};
        juce::Rectangle<int> freq_label_bound_{};
        juce::Rectangle<int> gain_label_bound_{};
        juce::Rectangle<int> q_label_bound_{};
        juce::Rectangle<int> slope_label_bound_{};

        juce::Point<float> position_{};
        juce::Point<float> target_position_{};
        juce::Rectangle<float> safe_bound_{};
        bool target_visible_{false};
        bool place_below_{false};
        bool placement_initialized_{false};
        float ideal_width_{0.f};
        float ideal_height_{0.f};

        void updateFilterCapabilities();
        void updateTransformation();
    };
}
