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
        PluginProcessor& p_ref_;
        zlgui::UIBase& base_;
        zlgui::attachment::ComponentUpdater updater_{};

        // Analyzer is deliberately split into two interactions:
        // clicking the word opens its detail sheet, while the switch controls whether
        // any spectrum trace is rendered at all.
        zlgui::button::ClickTextButton analyzer_menu_button_;
        zlgui::button::ClickTextButton analyzer_toggle_button_;
        zlgui::button::ClickTextButton pre_button_;
        zlgui::button::ClickTextButton post_button_;
        zlgui::button::ClickTextButton side_button_;
        zlgui::attachment::ButtonAttachment<true> pre_attach_;
        zlgui::attachment::ButtonAttachment<true> post_attach_;
        zlgui::attachment::ButtonAttachment<true> side_attach_;

        zlgui::combobox::CompactCombobox speed_box_;
        zlgui::attachment::ComboBoxAttachment<true> speed_attach_;

        zlgui::combobox::CompactCombobox phase_box_;
        zlgui::attachment::ComboBoxAttachment<true> phase_attach_;

        juce::Rectangle<int> processing_label_bound_{};
        juce::Rectangle<int> spectrum_label_bound_{};
        juce::Rectangle<int> analyzer_group_bound_{};
        juce::Rectangle<int> processing_group_bound_{};

        unsigned int analyzer_restore_mask_{0b011u};

        void stylePill(zlgui::button::ClickTextButton& button);
        bool isAnalyzerEnabled() const;
        unsigned int getAnalyzerMask() const;
        void setAnalyzerMask(unsigned int mask);
        void toggleAnalyzerEnabled();
    };
}
