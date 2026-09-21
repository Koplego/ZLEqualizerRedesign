// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
#pragma once

#include "../../../../PluginProcessor.hpp"
#include "../../../../gui/gui.hpp"
#include "../../../helper/helper.hpp"
#include "../../../helper/freq_note.hpp"
#include "../../../multilingual/tooltip_helper.hpp"

namespace zlpanel {
    class FloatPopPanel final : public juce::Component,
                                private juce::Timer {
    public:
        explicit FloatPopPanel(PluginProcessor& p, zlgui::UIBase& base,
                               const multilingual::TooltipHelper& tooltip_helper);
        ~FloatPopPanel() override = default;

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseUp(const juce::MouseEvent& event) override;
        void repaintCallBackSlow();
        void updateBand();
        int getIdealHeight() const;
        int getIdealWidth() const;
        void updatePosition(juce::Point<float> position, juce::Point<float> target_position);
        void setTargetVisible(bool is_target_visible);
        void updateFloatingBound(juce::Rectangle<float> bound);

    private:
        enum class Placement { above, below };

        PluginProcessor& p_ref_;
        zlgui::UIBase& base_;
        zlgui::attachment::ComponentUpdater updater_{};

        const std::unique_ptr<juce::Drawable> bypass_drawable_;
        zlgui::button::ClickButton bypass_button_;
        const std::unique_ptr<juce::Drawable> dynamic_drawable_;
        zlgui::button::ClickButton dynamic_button_;
        const std::unique_ptr<juce::Drawable> solo_drawable_;
        zlgui::button::ClickButton solo_button_;
        zlgui::combobox::CompactCombobox ftype_box_;
        zlgui::combobox::CompactCombobox slope_box_;
        zlgui::slider::CompactLinearSlider<false, false, false> freq_slider_;
        zlgui::slider::CompactLinearSlider<false, false, false> gain_slider_;
        zlgui::slider::CompactLinearSlider<false, false, false> q_slider_;

        std::unique_ptr<zlgui::attachment::ComboBoxAttachment<true>> ftype_attachment_;
        std::unique_ptr<zlgui::attachment::ComboBoxAttachment<true>> slope_attachment_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> freq_attachment_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> gain_attachment_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> q_attachment_;

        std::atomic<float>* filter_status_ptr_{nullptr};
        std::atomic<float>* filter_type_ptr_{nullptr};
        std::atomic<float>* slope_ptr_{nullptr};
        std::atomic<float>* dynamic_on_ptr_{nullptr};
        int current_filter_type_{-1};
        int current_slope_{-1};
        bool slope_supported_{true};
        bool target_visible_{false};

        juce::Rectangle<int> filter_name_bound_{};
        juce::Rectangle<int> slope_name_bound_{};
        std::array<juce::Rectangle<int>, 3> value_bounds_{};

        juce::Point<float> position_{};
        juce::Point<float> target_position_{};
        juce::Rectangle<float> floating_bound_{};
        Placement placement_{Placement::above};
        bool transform_initialized_{false};
        float current_x_{0.f}, current_y_{0.f};
        float target_x_{0.f}, target_y_{0.f};

        void updateFilterCapabilities();
        void updateTransformationTarget();
        void applyCurrentTransform();
        void timerCallback() override;
        void drawFilterGlyph(juce::Graphics& g, juce::Rectangle<float> r, int type,
                             juce::Colour colour) const;
        void drawQuickActionGlyphs(juce::Graphics& g) const;
    };
}
