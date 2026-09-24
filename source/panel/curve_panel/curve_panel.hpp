// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "background_panel/background_panel.hpp"
#include "fft_panel/fft_panel.hpp"
#include "fft_panel/match_fft_panel.hpp"
#include "response_panel/response_panel.hpp"
#include "output_panel/output_panel.hpp"
#include "analyzer_panel/analyzer_panel.hpp"

namespace zlpanel {
    class GlassOutputMeter final : public juce::Component,
                                   private juce::Timer {
    public:
        GlassOutputMeter(PluginProcessor& p, zlgui::UIBase& base) : p_ref_(p), base_(base) {
            setInterceptsMouseClicks(false, false);
            startTimerHz(30);
        }

        void paint(juce::Graphics& g) override;

    private:
        PluginProcessor& p_ref_;
        zlgui::UIBase& base_;
        std::array<float, 2> level_db_{{-60.f, -60.f}};
        std::array<float, 2> peak_db_{{-60.f, -60.f}};

        void timerCallback() override;
    };

    class CurvePanel final : public juce::Component,
                             private juce::ValueTree::Listener,
                             private juce::Thread {
    public:
        explicit CurvePanel(PluginProcessor& p, zlgui::UIBase& base,
                            multilingual::TooltipHelper& tooltip_helper);

        ~CurvePanel() override;

        void paintOverChildren(juce::Graphics& g) override;

        void run() override;

        void resized() override;

        void mouseDown(const juce::MouseEvent&) override;

        void repaintCallBack();

        void repaintCallBackSlow();

        void updateBand();

        void updateSampleRate(double sample_rate);

        void startThreads();

        void stopThreads();

        juce::Component& getNodeLens(size_t band) { return response_panel_.getNodeLens(band); }
        juce::Rectangle<int> getGraphGlassBounds() const { return background_panel_.getBounds(); }
        juce::Rectangle<int> getMeterGlassBounds() const { return output_meter_.getBounds(); }

        auto& getFFTPanel() {
            return fft_panel_;
        }

        auto& getOutputPanel() {
            return output_panel_;
        }

        auto& getMatchFFTPanel() {
            return match_fft_panel_;
        }

    private:
        zlgui::UIBase& base_;
        GlassOutputMeter output_meter_;
        BackgroundPanel background_panel_;
        FFTPanel fft_panel_;
        ResponsePanel response_panel_;
        MatchFFTPanel match_fft_panel_;
        ScalePanel scale_panel_;
        OutputPanel output_panel_;
        AnalyzerPanel analyzer_panel_;
        std::atomic<bool> is_match_on_{false};

        void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property) override;
    };
}
