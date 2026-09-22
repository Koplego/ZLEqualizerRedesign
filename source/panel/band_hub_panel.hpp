// Copyright (C) 2026 - zsliu98
// Glass EQ contextual band workspace
#pragma once

#include <array>
#include <utility>
#include <vector>

#include "../PluginProcessor.hpp"
#include "../gui/gui.hpp"
#include "../gui/ambient_light.hpp"
#include "helper/helper.hpp"
#include "multilingual/tooltip_helper.hpp"

namespace zlpanel {
    class BandHubPanel final : public juce::Component,
                               private juce::Timer {
    public:
        explicit BandHubPanel(PluginProcessor& p, zlgui::UIBase& base,
                              const multilingual::TooltipHelper& tooltip_helper);
        ~BandHubPanel() override = default;

        void paint(juce::Graphics& g) override;
        void resized() override;
        bool hitTest(int x, int y) override;
        void mouseUp(const juce::MouseEvent& event) override;

        int getIdealWidth() const;
        int getIdealHeight() const;
        void updateBand();
        void repaintCallbackSlow();
        void setSuppressed(bool suppressed);

        void setAmbientSources(std::vector<zlgui::glass::AmbientLightSource> sources) {
            ambient_sources_ = std::move(sources);
            repaint();
        }

    private:
        enum class Page { dynamics, detector, sidechain };

        PluginProcessor& p_ref_;
        zlgui::UIBase& base_;
        zlgui::attachment::ComponentUpdater updater_{};
        std::vector<zlgui::glass::AmbientLightSource> ambient_sources_{};

        Page page_{Page::dynamics};
        size_t attached_band_{zlp::kBandNum};
        bool dynamic_on_{false};
        bool expanded_{false};
        bool suppressed_{false};
        float reveal_{0.f};
        float reveal_target_{0.f};

        zlgui::button::ClickTextButton collapse_button_;
        zlgui::button::ClickTextButton dynamic_button_;
        zlgui::button::ClickTextButton dynamics_page_button_;
        zlgui::button::ClickTextButton detector_page_button_;
        zlgui::button::ClickTextButton sidechain_page_button_;
        zlgui::combobox::CompactCombobox channel_box_;

        zlgui::slider::CompactLinearSlider<false, false, false> threshold_slider_;
        zlgui::slider::CompactLinearSlider<false, false, false> range_slider_;
        zlgui::slider::CompactLinearSlider<false, false, false> attack_slider_;
        zlgui::slider::CompactLinearSlider<false, false, false> release_slider_;

        zlgui::slider::CompactLinearSlider<false, false, false> knee_slider_;
        zlgui::slider::CompactLinearSlider<false, false, false> rms_length_slider_;
        zlgui::slider::CompactLinearSlider<false, false, false> rms_mix_slider_;
        zlgui::slider::CompactLinearSlider<false, false, false> smooth_slider_;
        zlgui::button::ClickTextButton learn_button_;
        zlgui::button::ClickTextButton relative_button_;
        zlgui::button::ClickTextButton dyn_bypass_button_;

        zlgui::combobox::CompactCombobox side_type_box_;
        zlgui::combobox::CompactCombobox side_order_box_;
        zlgui::slider::CompactLinearSlider<false, false, false> side_freq_slider_;
        zlgui::slider::CompactLinearSlider<false, false, false> side_q_slider_;
        zlgui::button::ClickTextButton side_link_button_;
        zlgui::button::ClickTextButton side_swap_button_;

        std::unique_ptr<zlgui::attachment::ComboBoxAttachment<true>> channel_attachment_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> threshold_attachment_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> range_attachment_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> attack_attachment_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> release_attachment_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> knee_attachment_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> rms_length_attachment_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> rms_mix_attachment_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> smooth_attachment_;
        std::unique_ptr<zlgui::attachment::ButtonAttachment<true>> learn_attachment_;
        std::unique_ptr<zlgui::attachment::ButtonAttachment<true>> relative_attachment_;
        std::unique_ptr<zlgui::attachment::ButtonAttachment<true>> dyn_bypass_attachment_;
        std::unique_ptr<zlgui::attachment::ComboBoxAttachment<true>> side_type_attachment_;
        std::unique_ptr<zlgui::attachment::ComboBoxAttachment<true>> side_order_attachment_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> side_freq_attachment_;
        std::unique_ptr<zlgui::attachment::SliderAttachment<true>> side_q_attachment_;
        std::unique_ptr<zlgui::attachment::ButtonAttachment<true>> side_link_attachment_;
        std::unique_ptr<zlgui::attachment::ButtonAttachment<true>> side_swap_attachment_;

        std::atomic<float>* dynamic_on_ptr_{nullptr};
        std::array<juce::Rectangle<int>, 4> primary_label_bounds_{};
        std::array<juce::Rectangle<int>, 4> detail_label_bounds_{};
        juce::Rectangle<int> title_bound_{};
        juce::Rectangle<int> page_title_bound_{};

        void setExpanded(bool expanded);
        void setPage(Page page);
        void syncDynamicState();
        void updateVisibility();
        void clearAttachments();
        void stylePill(zlgui::button::ClickTextButton& button, float font_scale = .68f);
        juce::Rectangle<float> currentSurface() const;
        juce::Rectangle<float> expandedSurface() const;
        juce::Rectangle<float> collapsedSurface() const;
        void timerCallback() override;
    };
}
