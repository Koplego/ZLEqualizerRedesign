// Copyright (C) 2026 - zsliu98
// Glass EQ personal UI fork
#pragma once

#include "../PluginProcessor.hpp"
#include "../gui/gui.hpp"
#include "helper/helper.hpp"

namespace zlpanel {
    class GlassFooterPanel final : public juce::Component {
    public:
        GlassFooterPanel(PluginProcessor& p, zlgui::UIBase& base);

        void paint(juce::Graphics& g) override;
        void resized() override;
        int getIdealHeight() const;
        void repaintCallbackSlow();

    private:
        zlgui::UIBase& base_;
        zlgui::attachment::ComponentUpdater updater_{};

        zlgui::button::ClickTextButton analyzer_button_;
        zlgui::button::ClickTextButton pre_button_;
        zlgui::button::ClickTextButton post_button_;
        zlgui::attachment::ButtonAttachment<true> pre_attach_;
        zlgui::attachment::ButtonAttachment<true> post_attach_;

        zlgui::combobox::CompactCombobox speed_box_;
        zlgui::attachment::ComboBoxAttachment<true> speed_attach_;

        zlgui::button::ClickTextButton output_button_;

        zlgui::combobox::CompactCombobox phase_box_;
        zlgui::attachment::ComboBoxAttachment<true> phase_attach_;

        juce::Rectangle<int> processing_label_bound_{};
        juce::Rectangle<int> spectrum_label_bound_{};
        juce::Rectangle<int> analyzer_group_bound_{};
        juce::Rectangle<int> tools_group_bound_{};
        juce::Rectangle<int> processing_group_bound_{};

        void stylePill(zlgui::button::ClickTextButton& button);
    };
}
