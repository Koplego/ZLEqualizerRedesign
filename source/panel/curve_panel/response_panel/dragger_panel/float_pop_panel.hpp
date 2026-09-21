// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "../../../../PluginProcessor.hpp"
#include "../../../../gui/gui.hpp"
#include "../../../helper/helper.hpp"
#include "../../../helper/freq_note.hpp"
#include "../../../multilingual/tooltip_helper.hpp"
#include "../../../background/panel_background.hpp"

namespace zlpanel {
    class FloatPopPanel final : public juce::Component,
                                private juce::ValueTree::Listener {
    public:
        explicit FloatPopPanel(PluginProcessor& p, zlgui::UIBase& base,
                               const multilingual::TooltipHelper& tooltip_helper);
        ~FloatPopPanel() override;

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

        bool is_target_visible_{false};
        bool dynamic_on_{false};
        bool hub_open_{false};
        bool bubble_below_{false};
        bool bubble_side_initialized_{false};

        enum class DetailPage { dynamics, detector, sidechain };
        DetailPage detail_page_{DetailPage::dynamics};

        juce::Point<float> position_{};
        juce::Point<float> target_position_{};
        juce::Rectangle<float> safe_bound_{};
        float ideal_height_{}, ideal_width_{};

        const std::unique_ptr<juce::Drawable> bypass_drawable_;
        zlgui::button::ClickButton bypass_button_;
        std::atomic<float>* filter_status_ptr_{nullptr};

        const std::unique_ptr<juce::Drawable> solo_drawable_;
        zlgui::button::ClickButton solo_button_;

        const std::unique_ptr<juce::Drawable> close_drawable_;
        zlgui::button::ClickButton close_button_;

        const std::unique_ptr<juce::Drawable> dynamic_drawable_;
        zlgui::button::ClickButton dynamic_button_;
        std::unique_ptr<zlgui::attachment::ButtonAttachment<true>> dynamic_attachment_;
        std::atomic<float>* dynamic_on_ptr_{nullptr};

        zlgui::button::ClickTextButton dynamics_page_button_;
        zlgui::button::ClickTextButton detector_page_button_;
        zlgui::button::ClickTextButton sidechain_page_button_;
        zlgui::button::ClickTextButton more_button_;
        bool slope_supported_{true};

        zlgui::combobox::CompactCombobox ftype_box_;
        std::unique_ptr<zlgui::attachment::ComboBoxAttachment<true>> ftype_attachment_;
        zlgui::combobox::CompactCombobox lr_box_;
        std::unique_ptr<zlgui::attachment::ComboBoxAttachment<true>> lr_attachment_;
        zlgui::combobox::CompactCombobox slope_box_;
        std::unique_ptr<zlgui::attachment::ComboBoxAttachment<true>> slope_attachment_;
        std::atomic<float>* filter_type_ptr_{nullptr};
        std::atomic<float>* slope_ptr_{nullptr};
        int current_filter_type_{-1};
        int current_slope_{-1};

        zlgui::slider::CompactLinearSlider<false, false, false> freq_slider_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> freq_attachment_;
        zlgui::slider::CompactLinearSlider<false, false, false> gain_slider_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> gain_attachment_;
        zlgui::slider::CompactLinearSlider<false, false, false> q_slider_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> q_attachment_;

        zlgui::slider::CompactLinearSlider<false, false, false> range_slider_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> range_attachment_;
        zlgui::slider::CompactLinearSlider<false, false, false> threshold_slider_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> threshold_attachment_;
        zlgui::slider::CompactLinearSlider<false, false, false> attack_slider_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> attack_attachment_;
        zlgui::slider::CompactLinearSlider<false, false, false> release_slider_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> release_attachment_;

        zlgui::slider::CompactLinearSlider<false, false, false> knee_slider_;
        zlgui::slider::CompactLinearSlider<false, false, false> rms_length_slider_;
        zlgui::slider::CompactLinearSlider<false, false, false> rms_mix_slider_;
        zlgui::slider::CompactLinearSlider<false, false, false> smooth_slider_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> knee_attachment_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> rms_length_attachment_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> rms_mix_attachment_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> smooth_attachment_;

        zlgui::button::ClickTextButton learn_button_;
        zlgui::button::ClickTextButton relative_button_;
        zlgui::button::ClickTextButton dyn_bypass_button_;
        std::unique_ptr<zlgui::attachment::ButtonAttachment<true>> learn_attachment_;
        std::unique_ptr<zlgui::attachment::ButtonAttachment<true>> relative_attachment_;
        std::unique_ptr<zlgui::attachment::ButtonAttachment<true>> dyn_bypass_attachment_;

        zlgui::combobox::CompactCombobox side_type_box_;
        zlgui::combobox::CompactCombobox side_order_box_;
        zlgui::slider::CompactLinearSlider<false, false, false> side_freq_slider_;
        zlgui::slider::CompactLinearSlider<false, false, false> side_q_slider_;
        std::unique_ptr<zlgui::attachment::ComboBoxAttachment<true>> side_type_attachment_;
        std::unique_ptr<zlgui::attachment::ComboBoxAttachment<true>> side_order_attachment_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> side_freq_attachment_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> side_q_attachment_;
        zlgui::button::ClickTextButton side_link_button_;
        zlgui::button::ClickTextButton side_swap_button_;
        std::unique_ptr<zlgui::attachment::ButtonAttachment<true>> side_link_attachment_;
        std::unique_ptr<zlgui::attachment::ButtonAttachment<true>> side_swap_attachment_;

        juce::Rectangle<int> filter_name_bound_, mode_name_bound_, hub_title_bound_;
        juce::Rectangle<int> dynamic_title_bound_, advanced_section_title_bound_;
        std::array<juce::Rectangle<int>, 4> primary_dynamic_label_bounds_{};
        std::array<juce::Rectangle<int>, 4> advanced_dynamic_label_bounds_{};

        void setHubOpen(bool open);
        void updateDynamicVisibility(bool dynamic_on, bool request_parent_resize);
        void updateDetailPage(DetailPage page, bool request_parent_resize);
        void updateDetailVisibility();
        void updateFilterCapabilities();
        void styleDetailButton(zlgui::button::ClickTextButton& button);
        void updateTransformation();

        void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override;
    };
}
