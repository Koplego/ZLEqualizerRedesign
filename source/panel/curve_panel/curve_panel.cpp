// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#include "curve_panel.hpp"
#include "../../gui/glass_tokens.hpp"

namespace zlpanel {
    void GlassOutputMeter::timerCallback() {
        for (size_t channel = 0; channel < level_db_.size(); ++channel) {
            const auto incoming = juce::Decibels::gainToDecibels(p_ref_.getOutputPeak(channel), -60.f);
            level_db_[channel] = incoming > level_db_[channel]
                ? incoming
                : juce::jmax(-60.f, level_db_[channel] - 1.25f);
            peak_db_[channel] = incoming > peak_db_[channel]
                ? incoming
                : juce::jmax(level_db_[channel], peak_db_[channel] - .42f);
        }
        repaint();
    }

    void GlassOutputMeter::paint(juce::Graphics& g) {
        auto bounds = getLocalBounds().toFloat().reduced(.5f);
        const auto radius = juce::jmax(7.f, base_.getFontSize() * .56f);

        // The reference meter sits in its own blue-violet glass channel. Do not use the
        // generic dark control material here; its enclosure is visibly lighter than the
        // previous build and picks up the purple field from the right side of the shell.
        juce::Path meter_clip;
        meter_clip.addRoundedRectangle(bounds, radius);
        {
            juce::Graphics::ScopedSaveState state(g);
            g.reduceClipRegion(meter_clip);
            juce::ColourGradient housing(juce::Colour(64, 87, 136).withAlpha(.88f),
                                         bounds.getX(), bounds.getY(),
                                         juce::Colour(42, 54, 102).withAlpha(.94f),
                                         bounds.getRight(), bounds.getBottom(), false);
            housing.addColour(.46, juce::Colour(53, 73, 119).withAlpha(.92f));
            g.setGradientFill(housing);
            g.fillRect(bounds.expanded(1.f));

            juce::ColourGradient violet(juce::Colour(170, 129, 239).withAlpha(.11f),
                                        bounds.getRight() - bounds.getWidth() * .18f,
                                        bounds.getY() + bounds.getHeight() * .24f,
                                        juce::Colours::transparentBlack,
                                        bounds.getX() + bounds.getWidth() * .30f,
                                        bounds.getY() + bounds.getHeight() * .70f, true);
            g.setGradientFill(violet);
            g.fillRect(bounds);

            juce::ColourGradient sheen(juce::Colour(232, 244, 252).withAlpha(.105f),
                                       bounds.getCentreX(), bounds.getY(),
                                       juce::Colours::transparentBlack,
                                       bounds.getCentreX(), bounds.getBottom() * .55f, false);
            g.setGradientFill(sheen);
            g.fillRect(bounds);
        }
        g.setColour(juce::Colour(239, 248, 253).withAlpha(.18f));
        g.drawRoundedRectangle(bounds, radius, .80f);

        const auto label_w = juce::jmax(18.f, base_.getFontSize() * 1.55f);
        auto meter_area = bounds.reduced(base_.getFontSize() * .42f, base_.getFontSize() * .52f);
        auto labels = meter_area.removeFromRight(label_w);
        meter_area.removeFromRight(base_.getFontSize() * .18f);

        const auto gap = juce::jmax(2.f, base_.getFontSize() * .20f);
        const auto bar_w = (meter_area.getWidth() - gap) * .5f;
        const std::array<float, 6> marks{{0.f, -6.f, -12.f, -24.f, -36.f, -60.f}};
        g.setFont(juce::FontOptions(base_.getFontSize() * .55f));
        g.setColour(juce::Colour(238, 246, 252).withAlpha(.48f));
        for (const auto mark : marks) {
            const auto norm = juce::jlimit(0.f, 1.f, (mark + 60.f) / 60.f);
            const auto y = meter_area.getBottom() - norm * meter_area.getHeight();
            g.drawText(juce::String(static_cast<int>(mark)),
                       labels.withY(y - base_.getFontSize() * .38f)
                             .withHeight(base_.getFontSize() * .76f).toNearestInt(),
                       juce::Justification::centredRight, false);
        }

        for (size_t channel = 0; channel < 2; ++channel) {
            auto track = juce::Rectangle<float>(meter_area.getX() + static_cast<float>(channel) * (bar_w + gap),
                                                 meter_area.getY(), bar_w, meter_area.getHeight());
            g.setColour(juce::Colour(10, 27, 48).withAlpha(.74f));
            g.fillRoundedRectangle(track, bar_w * .42f);
            g.setColour(juce::Colour(226, 241, 250).withAlpha(.075f));
            g.drawRoundedRectangle(track, bar_w * .42f, .55f);

            const auto norm = juce::jlimit(0.f, 1.f, (level_db_[channel] + 60.f) / 60.f);
            auto active = track.withTop(track.getBottom() - norm * track.getHeight());
            juce::ColourGradient meter(juce::Colour(105, 237, 184), active.getCentreX(), active.getBottom(),
                                       juce::Colour(255, 194, 88), active.getCentreX(), active.getY(), false);
            meter.addColour(.68, juce::Colour(128, 229, 161));
            meter.addColour(.88, juce::Colour(226, 217, 111));
            if (active.getHeight() > 1.f) {
                const juce::DropShadow meter_glow{juce::Colour(108, 232, 184).withAlpha(.30f),
                                                   juce::jmax(2, juce::roundToInt(bar_w * .82f)), {0, 0}};
                juce::Path glow_path;
                glow_path.addRoundedRectangle(active, bar_w * .42f);
                meter_glow.drawForPath(g, glow_path);
            }
            g.setGradientFill(meter);
            g.fillRoundedRectangle(active, bar_w * .42f);

            const auto peak_norm = juce::jlimit(0.f, 1.f, (peak_db_[channel] + 60.f) / 60.f);
            const auto peak_y = track.getBottom() - peak_norm * track.getHeight();
            g.setColour(juce::Colour(255, 224, 147).withAlpha(.92f));
            g.fillRect(track.getX(), peak_y, track.getWidth(), juce::jmax(1.f, base_.getFontSize() * .08f));
        }
    }

    CurvePanel::CurvePanel(PluginProcessor& p,
                           zlgui::UIBase& base,
                           multilingual::TooltipHelper& tooltip_helper) :
        Thread("curve_panel"),
        base_(base),
        output_meter_(p, base),
        background_panel_(p, base, tooltip_helper),
        fft_panel_(p, base),
        response_panel_(p, base, tooltip_helper),
        match_fft_panel_(p, base),
        scale_panel_(p, base, tooltip_helper),
        output_panel_(p, base, tooltip_helper),
        analyzer_panel_(p, base, tooltip_helper) {
        background_panel_.setBufferedToImage(true);
        addAndMakeVisible(background_panel_);
        addAndMakeVisible(output_meter_);
        addAndMakeVisible(fft_panel_);
        addChildComponent(match_fft_panel_);
        addAndMakeVisible(response_panel_);
        response_panel_.addMouseListener(this, true);
        scale_panel_.setBufferedToImage(false);
        addChildComponent(scale_panel_);
        addChildComponent(output_panel_);
        addChildComponent(analyzer_panel_);
        setInterceptsMouseClicks(false, true);
        base_.getPanelValueTree().addListener(this);
    }

    CurvePanel::~CurvePanel() {
        base_.getPanelValueTree().removeListener(this);
        stopThreads();
    }

    void CurvePanel::paintOverChildren(juce::Graphics&) {
        notify();
        response_panel_.notify();
    }

    void CurvePanel::run() {
        while (!threadShouldExit()) {
            const auto flag = wait(-1);
            juce::ignoreUnused(flag);
            if (is_match_on_.load(std::memory_order::relaxed)) {
                match_fft_panel_.run(*this);
            } else {
                fft_panel_.run(*this);
            }
        }
    }

    void CurvePanel::resized() {
        auto bound = getLocalBounds();
        const auto font_size = base_.getFontSize();
        const auto padding = getPaddingSize(font_size);
        const auto meter_width = juce::jmax(50, juce::roundToInt(font_size * 4.35f));
        auto meter_bound = bound.removeFromRight(meter_width);
        bound.removeFromRight(juce::jmax(3, padding / 2));
        output_meter_.setBounds(meter_bound.reduced(0, juce::jmax(2, padding / 3)));

        background_panel_.setBounds(bound);
        fft_panel_.setBounds(bound);
        response_panel_.setBounds(bound);
        match_fft_panel_.setBounds(bound);

        const auto output_width = output_panel_.getIdealWidth();
        const auto output_height = output_panel_.getIdealHeight();
        output_panel_.setBounds(bound.getWidth() - output_width - 2 * padding,
                                juce::jmax(0, bound.getHeight() - output_height - padding),
                                output_width, output_height);

        const auto analyzer_width = analyzer_panel_.getIdealWidth();
        const auto analyzer_height = analyzer_panel_.getIdealHeight();
        analyzer_panel_.setBounds(getButtonSize(font_size) + 2 * padding,
                                  juce::jmax(0, bound.getHeight() - analyzer_height - padding),
                                  analyzer_width, analyzer_height);

        scale_panel_.setBounds(bound.withLeft(bound.getWidth() - scale_panel_.getIdealWidth()));
    }

    void CurvePanel::mouseDown(const juce::MouseEvent&) {
        base_.setPanelProperty(zlgui::PanelSettingIdx::kOutputPanel, 0.f);
        base_.setPanelProperty(zlgui::PanelSettingIdx::kAnalyzerPanel, 0.f);
    }

    void CurvePanel::repaintCallBack() {
        repaint();
        response_panel_.repaintCallBack();
    }

    void CurvePanel::repaintCallBackSlow() {
        response_panel_.repaintCallBackSlow();
        output_panel_.repaintCallBackSlow();
        analyzer_panel_.repaintCallBackSlow();
        scale_panel_.repaintCallBackSlow();
    }

    void CurvePanel::updateBand() {
        response_panel_.updateBand();
    }

    void CurvePanel::updateSampleRate(const double sample_rate) {
        background_panel_.updateSampleRate(sample_rate);
        response_panel_.updateSampleRate(sample_rate);
    }

    void CurvePanel::startThreads() {
        startThread(juce::Thread::Priority::low);
        response_panel_.startThread(juce::Thread::Priority::low);
    }

    void CurvePanel::stopThreads() {
        if (isThreadRunning()) stopThread(-1);
        if (response_panel_.isThreadRunning()) response_panel_.stopThread(-1);
    }

    void CurvePanel::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property) {
        if (base_.isPanelIdentifier(zlgui::PanelSettingIdx::kMatchPanel, property)) {
            const auto f = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kMatchPanel));
            const auto idx = static_cast<int>(std::round(f));
            match_fft_panel_.setVisible(idx > 0);
            is_match_on_.store(idx > 0, std::memory_order::relaxed);
            scale_panel_.setVisible(idx > 0);
            fft_panel_.setVisible(idx == 0);
            response_panel_.setVisible(idx != 1 && idx != 2);
            response_panel_.turnMatchON(idx > 0);
        }
    }
}
