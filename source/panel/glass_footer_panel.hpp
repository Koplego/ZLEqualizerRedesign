// Copyright (C) 2026 - zsliu98
// Glass EQ personal UI fork
#pragma once

#include <functional>

#include "../PluginProcessor.hpp"
#include "../gui/gui.hpp"
#include "helper/helper.hpp"

namespace zlpanel {
    class GlassFooterPanel final : public juce::Component {
    public:
        GlassFooterPanel(PluginProcessor& p, zlgui::UIBase& base,
                         std::function<void()> controls_callback,
                         std::function<void()> settings_callback);

        void paint(juce::Graphics& g) override;
        void resized() override;
        int getIdealHeight() const;
        void repaintCallbackSlow();

        void setControlsActive(bool active);

    private:
        zlgui::UIBase& base_;
        zlgui::attachment::ComponentUpdater updater_{};

        std::function<void()> controls_callback_;
        std::function<void()> settings_callback_;

        zlgui::button::ClickTextButton analyzer_button_;
        zlgui::button::ClickTextButton pre_button_;
        zlgui::button::ClickTextButton post_button_;
        zlgui::attachment::ButtonAttachment<true> pre_attach_;
        zlgui::attachment::ButtonAttachment<true> post_attach_;

        zlgui::combobox::CompactCombobox speed_box_;
        zlgui::attachment::ComboBoxAttachment<true> speed_attach_;

        zlgui::button::ClickTextButton controls_button_;
        zlgui::button::ClickTextButton settings_button_;

        zlgui::combobox::CompactCombobox phase_box_;
        zlgui::attachment::ComboBoxAttachment<true> phase_attach_;

        juce::Rectangle<int> processing_label_bound_{};
        juce::Rectangle<int> analyzer_group_bound_{};
        juce::Rectangle<int> tools_group_bound_{};
        juce::Rectangle<int> processing_group_bound_{};

        void stylePill(zlgui::button::ClickTextButton& button);
    };
}
