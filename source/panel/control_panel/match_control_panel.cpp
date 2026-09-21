// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#include "match_control_panel.hpp"
#include "../../gui/glass_tokens.hpp"
#include "BinaryData.h"

namespace zlpanel {
    namespace {
        juce::File getLegacyPresetDirectory() {
            return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                .getChildFile("Audio")
                .getChildFile("Presets")
                .getChildFile(JucePlugin_Manufacturer)
                .getChildFile(JucePlugin_Name)
                .getChildFile("Match Presets");
        }

        juce::Result ensureDirectoryExists(const juce::File& directory) {
            if (directory.isDirectory()) {
                return juce::Result::ok();
            }

            const auto parent = directory.getParentDirectory();
            if (parent == directory) {
                return juce::Result::fail("Could not find a parent directory");
            }
            if (const auto result = ensureDirectoryExists(parent); result.failed()) {
                return result;
            }

            const auto result = directory.createDirectory();
            return result.wasOk() || directory.isDirectory() ? juce::Result::ok() : result;
        }

        juce::Result migrateLegacyPresets(const juce::File& destination) {
            const auto legacy = getLegacyPresetDirectory();
            if (!legacy.isDirectory() || destination.exists()) {
                return juce::Result::ok();
            }

            const auto parent = destination.getParentDirectory();
            if (const auto result = ensureDirectoryExists(parent); result.failed()) {
                return result;
            }

            const auto temporary = parent.getNonexistentChildFile(".Match Preset Migration", {}, false);
            if (!legacy.copyDirectoryTo(temporary)) {
                [[maybe_unused]] const auto flag = temporary.deleteRecursively();
                return juce::Result::fail("Could not copy the existing match presets");
            }
            if (!temporary.moveFileTo(destination)) {
                [[maybe_unused]] const auto flag = temporary.deleteRecursively();
                return destination.isDirectory()
                    ? juce::Result::ok()
                    : juce::Result::fail("Could not finish moving the existing match presets");
            }
            return juce::Result::ok();
        }
    }

    MatchControlPanel::MatchControlPanel(PluginProcessor& p, zlgui::UIBase& base,
                                         MatchFFTPanel& match_fft_panel,
                                         const multilingual::TooltipHelper& tooltip_helper) :
        p_ref_(p), base_(base),
        match_fft_panel_(match_fft_panel),
        control_background_(base),
        save_drawable_(juce::Drawable::createFromImageData(BinaryData::save_svg,
                                                           BinaryData::save_svgSize)),
        save_button_(base, save_drawable_.get(), nullptr,
                     tooltip_helper.getToolTipText(multilingual::kEQMatchSave)),
        draw_drawable_(juce::Drawable::createFromImageData(BinaryData::draw_svg,
                                                           BinaryData::draw_svgSize)),
        draw_button_(base, draw_drawable_.get(), draw_drawable_.get(),
                     tooltip_helper.getToolTipText(multilingual::kEQMatchDiffDraw)),
        target_box_({"Side", "Preset", "Flat", "Balanced", "Natural"}, base,
                    tooltip_helper.getToolTipText(multilingual::kEQMatchTarget),
                    {tooltip_helper.getToolTipText(multilingual::kEQMatchTargetSide),
                     tooltip_helper.getToolTipText(multilingual::kEQMatchTargetPreset),
                     tooltip_helper.getToolTipText(multilingual::kEQMatchTargetFlat),
                     tooltip_helper.getToolTipText(multilingual::kEQMatchTargetBalanced),
                     tooltip_helper.getToolTipText(multilingual::kEQMatchTargetNatural)}),
        label_laf_(base),
        shift_label_("", "Shift"),
        shift_slider_("", base,
                      tooltip_helper.getToolTipText(multilingual::kEQMatchDiffShift)),
        scale_label_("", "Scale"),
        scale_slider_("", base,
                      tooltip_helper.getToolTipText(multilingual::kEQMatchDiffSmooth)),
        slope_label_("", "Slope"),
        slope_slider_("", base,
                      tooltip_helper.getToolTipText(multilingual::kEQMatchDiffSlope)),
        limit_combobox_({"6 dB", "12 dB", "30 dB", "Inf"}, base, ""),
        start_drawable_(juce::Drawable::createFromImageData(BinaryData::start_svg,
                                                            BinaryData::start_svgSize)),
        fit_start_button_(base, start_drawable_.get(), nullptr,
                          tooltip_helper.getToolTipText(multilingual::kEQMatchFit)),
        num_band_slider_("", base,
                         tooltip_helper.getToolTipText(multilingual::kEQMatchNumBand)) {
        juce::ignoreUnused(p_ref_);
        setOpaque(false);

        control_background_.setBufferedToImage(true);
        addAndMakeVisible(control_background_);

        // Keep the existing buttons as hit targets but replace the legacy artwork with
        // the same restrained glyph language used throughout Glass EQ.
        for (auto* button : {&save_button_, &draw_button_, &fit_start_button_}) {
            button->setImageAlpha(0.f, 0.f, 0.f, 0.f);
            button->setBufferedToImage(true);
            addAndMakeVisible(button);
        }

        save_button_.getButton().onClick = [this]() { saveToPreset(); };
        draw_button_.getButton().onClick = [this]() {
            match_fft_panel_.setDiffDrawOn(draw_button_.getToggleState());
        };

        target_box_.getBox().onChange = [this]() {
            const auto mode = static_cast<MatchFFTPanel::SideMode>(target_box_.getBox().getSelectedItemIndex());
            if (mode == MatchFFTPanel::SideMode::kPreset) {
                loadFromPreset();
            }
            match_fft_panel_.setSideMode(mode);
        };
        target_box_.getLAF().setFontScale(.76f);
        target_box_.getLAF().setBoxAlpha(.56f);
        target_box_.getLAF().setLabelJustification(juce::Justification::centred);
        target_box_.setBufferedToImage(true);
        addAndMakeVisible(target_box_);

        shift_slider_.getSlider().setNormalisableRange(juce::NormalisableRange<double>(-30.0, 30.0, 0.1));
        shift_slider_.getSlider().setDoubleClickReturnValue(true, 0.);
        shift_slider_.getSlider().onValueChange = [this]() {
            match_fft_panel_.setDiffShift(static_cast<float>(shift_slider_.getSlider().getValue()));
        };

        scale_slider_.getSlider().setNormalisableRange(juce::NormalisableRange<double>(0.0, 1.0, 0.01));
        scale_slider_.getSlider().setDoubleClickReturnValue(true, 1.0);
        scale_slider_.getSlider().onValueChange = [this]() {
            match_fft_panel_.setDiffScale(static_cast<float>(scale_slider_.getSlider().getValue()));
        };

        slope_slider_.getSlider().setNormalisableRange(juce::NormalisableRange<double>(-4.5, 4.5, 0.1));
        slope_slider_.getSlider().setDoubleClickReturnValue(true, 0.);
        slope_slider_.getSlider().onValueChange = [this]() {
            match_fft_panel_.setDiffSlope(static_cast<float>(slope_slider_.getSlider().getValue()));
        };

        label_laf_.setFontScale(.78f);
        for (auto& l : {&shift_label_, &scale_label_, &slope_label_}) {
            l->setLookAndFeel(&label_laf_);
            l->setJustificationType(juce::Justification::centredLeft);
            l->setAlpha(.72f);
            l->setBufferedToImage(true);
            addAndMakeVisible(l);
        }

        for (auto& s : {&shift_slider_, &scale_slider_, &slope_slider_}) {
            s->setFontScale(.80f);
            s->getSlider().setSliderSnapsToMousePosition(false);
            s->setBufferedToImage(true);
            addAndMakeVisible(s);
        }

        limit_combobox_.getBox().onChange = [this]() {
            constexpr std::array<float, 4> kLimits{6.f, 12.f, 30.f, 300.f};
            const auto selected_index = static_cast<size_t>(
                std::max(limit_combobox_.getBox().getSelectedItemIndex(), 0));
            match_fft_panel_.setMatchLimit(kLimits[selected_index]);
        };
        limit_combobox_.getLAF().setFontScale(.76f);
        limit_combobox_.getLAF().setBoxAlpha(.56f);
        limit_combobox_.getLAF().setLabelJustification(juce::Justification::centred);
        limit_combobox_.setBufferedToImage(true);
        addAndMakeVisible(limit_combobox_);

        fit_start_button_.getButton().onClick = [this]() {
            if (match_fft_panel_.getMatchPhase() == MatchFFTPanel::MatchPhase::kMatch) {
                return;
            }
            for (size_t band = 0; band < zlp::kBandNum; ++band) {
                const auto band_s = std::to_string(band);
                auto* status_para = p_ref_.parameters_.getParameter(zlp::PFilterStatus::kID + band_s);
                updateValue(status_para, 0.f);
                auto* stereo_para = p_ref_.parameters_.getParameter(zlp::PLRMode::kID + band_s);
                updateValue(stereo_para, 0.f);
                auto* dynamic_para = p_ref_.parameters_.getParameter(zlp::PDynamicON::kID + band_s);
                updateValue(dynamic_para, 0.f);
                const auto max_idx = std::round(p_ref_.parameters_NA_.getRawParameterValue(
                    zlstate::PEQMaxDB::kID)->load(std::memory_order::relaxed));
                band_helper::turnOnOffDynamic(p_ref_, band, false,
                                              base_.getCurveDBScale(static_cast<size_t>(max_idx)));
            }
            base_.setPanelProperty(zlgui::PanelSettingIdx::kSuggestedNumBand, 0.0);
            base_.setPanelProperty(zlgui::PanelSettingIdx::kMatchPanel, 2.0);
            match_fft_panel_.setMatchPhase(MatchFFTPanel::MatchPhase::kMatch);
        };

        num_band_slider_.getSlider().setNormalisableRange(juce::NormalisableRange<double>(
            0.0, static_cast<double>(zlp::kBandNum), 1.0));
        num_band_slider_.getSlider().onValueChange = [this]() {
            match_fft_panel_.updateMatchNumBand(static_cast<size_t>(
                std::round(num_band_slider_.getSlider().getValue())));
        };
        num_band_slider_.setFontScale(.80f);
        num_band_slider_.getSlider().setSliderSnapsToMousePosition(false);
        num_band_slider_.setBufferedToImage(true);
        addChildComponent(num_band_slider_);

        base_.getPanelValueTree().addListener(this);
    }

    MatchControlPanel::~MatchControlPanel() {
        base_.getPanelValueTree().removeListener(this);
    }

    int MatchControlPanel::getIdealHeight() const {
        return juce::roundToInt(base_.getFontSize() * 16.2f);
    }

    int MatchControlPanel::getIdealWidth() const {
        return juce::roundToInt(base_.getFontSize() * 35.0f);
    }

    void MatchControlPanel::resized() {
        const auto font = base_.getFontSize();
        const auto padding = juce::jmax(6, juce::roundToInt(font * .72f));
        const auto button = juce::jmax(26, juce::roundToInt(font * 2.05f));
        const auto row = juce::jmax(28, juce::roundToInt(font * 2.15f));
        const auto label_h = juce::jmax(12, juce::roundToInt(font * .92f));

        control_background_.setBounds(getLocalBounds());
        auto content = getLocalBounds().reduced(padding + padding / 2, padding + padding / 2);

        auto header = content.removeFromTop(juce::roundToInt(font * 2.6f));
        title_bound_ = header.removeFromTop(juce::roundToInt(font * 1.32f));
        subtitle_bound_ = header;
        content.removeFromTop(padding / 3);

        // Target/source row.
        target_surface_bound_ = content.removeFromTop(row + label_h + padding).reduced(0, 1);
        auto target = target_surface_bound_.reduced(padding, padding / 2);
        target_label_bound_ = target.removeFromTop(label_h);
        auto target_controls = target;
        const auto utility = juce::jmax(button, juce::roundToInt(font * 2.15f));
        save_button_.setBounds(target_controls.removeFromRight(utility).withSizeKeepingCentre(utility, utility));
        target_controls.removeFromRight(padding / 3);
        draw_button_.setBounds(target_controls.removeFromRight(utility).withSizeKeepingCentre(utility, utility));
        target_controls.removeFromRight(padding / 2);
        target_box_.setBounds(target_controls);

        content.removeFromTop(padding / 2);

        // Difference shaping: three equal cells, matching the mockup's compact parameter grid.
        difference_surface_bound_ = content.removeFromTop(label_h + row + padding * 2).reduced(0, 1);
        auto diff = difference_surface_bound_.reduced(padding, padding / 2);
        difference_title_bound_ = diff.removeFromTop(label_h);
        diff.removeFromTop(padding / 4);
        const auto gap = juce::jmax(4, padding / 2);
        const auto cell_w = (diff.getWidth() - 2 * gap) / 3;
        auto shift_cell = diff.removeFromLeft(cell_w);
        diff.removeFromLeft(gap);
        auto scale_cell = diff.removeFromLeft(cell_w);
        diff.removeFromLeft(gap);
        auto slope_cell = diff;
        shift_label_.setBounds(shift_cell.removeFromTop(label_h));
        scale_label_.setBounds(scale_cell.removeFromTop(label_h));
        slope_label_.setBounds(slope_cell.removeFromTop(label_h));
        shift_slider_.setBounds(shift_cell);
        scale_slider_.setBounds(scale_cell);
        slope_slider_.setBounds(slope_cell);
        shift_slider_.getSlider().setMouseDragSensitivity(juce::jmax(80, cell_w));
        scale_slider_.getSlider().setMouseDragSensitivity(juce::jmax(80, cell_w));
        slope_slider_.getSlider().setMouseDragSensitivity(juce::jmax(80, cell_w));

        content.removeFromTop(padding / 2);

        // Fit controls: limit, optional fitted band count, then a clear primary Fit action.
        fit_surface_bound_ = content.removeFromTop(label_h + row + padding * 2).reduced(0, 1);
        auto fit = fit_surface_bound_.reduced(padding, padding / 2);
        fit_title_bound_ = fit.removeFromTop(label_h);
        fit.removeFromTop(padding / 4);

        auto fit_action = fit.removeFromRight(juce::roundToInt(font * 5.0f));
        fit_start_button_.setBounds(fit_action.reduced(1));
        fit.removeFromRight(padding / 2);

        const auto fit_cell_w = juce::jmax(72, fit.getWidth() / 2 - gap / 2);
        auto limit_cell = fit.removeFromLeft(juce::jmin(fit_cell_w, fit.getWidth()));
        if (fit.getWidth() > gap) fit.removeFromLeft(gap);
        auto bands_cell = fit;

        limit_label_bound_ = limit_cell.removeFromTop(label_h);
        limit_combobox_.setBounds(limit_cell);
        bands_label_bound_ = bands_cell.removeFromTop(label_h);
        num_band_slider_.setBounds(bands_cell);
        num_band_slider_.getSlider().setMouseDragSensitivity(juce::jmax(80, bands_cell.getWidth()));

        control_background_.setSurfaceBounds({target_surface_bound_, difference_surface_bound_, fit_surface_bound_});
    }

    void MatchControlPanel::paintOverChildren(juce::Graphics& g) {
        const auto font = base_.getFontSize();
        g.setColour(zlgui::glass::textPrimary().withAlpha(.94f));
        g.setFont(juce::FontOptions(font * 1.10f));
        g.drawText("EQ Match", title_bound_, juce::Justification::centredLeft, false);

        g.setColour(zlgui::glass::textSecondary().withMultipliedAlpha(.86f));
        g.setFont(juce::FontOptions(font * .63f));
        g.drawText("Shape a reference curve, then fit it with real EQ bands.", subtitle_bound_,
                   juce::Justification::centredLeft, false);

        g.setColour(zlgui::glass::textSecondary().withMultipliedAlpha(.78f));
        g.setFont(juce::FontOptions(font * .61f));
        g.drawText("TARGET", target_label_bound_, juce::Justification::centredLeft, false);
        g.drawText("DIFFERENCE", difference_title_bound_, juce::Justification::centredLeft, false);
        g.drawText("FIT", fit_title_bound_, juce::Justification::centredLeft, false);
        g.drawText("LIMIT", limit_label_bound_, juce::Justification::centredLeft, false);
        if (num_band_slider_.isVisible())
            g.drawText("BANDS", bands_label_bound_, juce::Justification::centredLeft, false);

        const auto draw_round_surface = [&g, font](const juce::Rectangle<int>& bounds, const bool active = false) {
            auto r = bounds.toFloat().reduced(1.f);
            zlgui::glass::fillGlassSurface(g, r, r.getHeight() * .5f,
                                           active ? .11f : .055f,
                                           active ? .18f : .095f,
                                           active ? .20f : .11f);
            return r;
        };

        // Save reference curve.
        {
            const auto r = draw_round_surface(save_button_.getBounds()).reduced(font * .52f);
            g.setColour(zlgui::glass::textPrimary().withAlpha(.58f));
            g.drawRoundedRectangle(r, 1.5f, 1.05f);
            g.drawLine(r.getX() + r.getWidth() * .24f, r.getY() + r.getHeight() * .18f,
                       r.getRight() - r.getWidth() * .24f, r.getY() + r.getHeight() * .18f, 1.05f);
            g.drawLine(r.getCentreX(), r.getY() + r.getHeight() * .18f,
                       r.getCentreX(), r.getBottom() - r.getHeight() * .18f, 1.05f);
        }

        // Draw/edit difference toggle.
        {
            const auto active = draw_button_.getToggleState();
            const auto r = draw_round_surface(draw_button_.getBounds(), active).reduced(font * .50f);
            juce::Path p;
            p.startNewSubPath(r.getX(), r.getBottom() - r.getHeight() * .18f);
            p.cubicTo(r.getX() + r.getWidth() * .26f, r.getY() + r.getHeight() * .20f,
                      r.getX() + r.getWidth() * .58f, r.getBottom() - r.getHeight() * .10f,
                      r.getRight(), r.getY() + r.getHeight() * .18f);
            g.setColour(zlgui::glass::textPrimary().withAlpha(active ? .92f : .52f));
            g.strokePath(p, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
        }

        // Primary fit action is text-forward rather than another unexplained legacy icon.
        {
            const auto r = fit_start_button_.getBounds().toFloat().reduced(.75f);
            zlgui::glass::fillGlassSurface(g, r, r.getHeight() * .5f, .13f, .22f, .22f);
            g.setColour(juce::Colour(126, 186, 245).withAlpha(.11f));
            g.fillRoundedRectangle(r.reduced(r.getHeight() * .10f), r.getHeight() * .42f);
            g.setColour(zlgui::glass::textPrimary().withAlpha(.94f));
            g.setFont(juce::FontOptions(font * .72f));
            g.drawText("Fit EQ", fit_start_button_.getBounds(), juce::Justification::centred, false);
        }
    }

    void MatchControlPanel::visibilityChanged() {
        if (!isVisible()) {
            return;
        }
        [[maybe_unused]] const auto migration_result = migrateLegacyPresets(kPresetDirectory);
        [[maybe_unused]] const auto creation_result = ensureDirectoryExists(kPresetDirectory);

        draw_button_.getButton().setToggleState(true, juce::sendNotificationSync);
        match_fft_panel_.setDiffDrawOn(true);
        target_box_.getBox().setSelectedItemIndex(0, juce::dontSendNotification);
        match_fft_panel_.setSideMode(MatchFFTPanel::SideMode::kSide);
        shift_slider_.getSlider().setValue(0.0, juce::sendNotificationSync);
        match_fft_panel_.setDiffShift(0.f);
        scale_slider_.getSlider().setValue(1.0, juce::sendNotificationSync);
        match_fft_panel_.setDiffScale(1.f);
        slope_slider_.getSlider().setValue(0.0, juce::sendNotificationSync);
        match_fft_panel_.setDiffSlope(0.f);
        limit_combobox_.getBox().setSelectedItemIndex(1, juce::sendNotificationSync);
        match_fft_panel_.setMatchLimit(12.f);
    }

    void MatchControlPanel::saveToPreset() {
        chooser_ = std::make_unique<juce::FileChooser>(
            "Save the match preset...", kPresetDirectory.getChildFile("match.csv"), "*.csv",
            true, false, nullptr);
        constexpr auto setting_save_flags = juce::FileBrowserComponent::saveMode |
            juce::FileBrowserComponent::warnAboutOverwriting;
        chooser_->launchAsync(setting_save_flags, [this](const juce::FileChooser& chooser) {
            if (chooser.getResults().size() <= 0) { return; }
            match_fft_panel_.saveToPreset(preset_freqs_, preset_dbs_);
            juce::File preset_file(chooser.getResult().withFileExtension("csv"));
            if (juce::FileOutputStream output(preset_file); output.openedOk()) {
                output.setPosition(0);
                output.truncate();
                for (size_t i = 0; i < preset_freqs_.size(); ++i) {
                    output.writeText(juce::String(preset_freqs_[i]), false, false, nullptr);
                    output.writeText(",", false, false, nullptr);
                    output.writeText(juce::String(preset_dbs_[i]), false, false, nullptr);
                    output.writeText("\n", false, false, nullptr);
                }
            }
        });
    }

    void MatchControlPanel::loadFromPreset() {
        chooser_ = std::make_unique<juce::FileChooser>(
            "Load the match preset...", kPresetDirectory, "*.csv",
            true, false, nullptr);
        constexpr auto settingOpenFlags = juce::FileBrowserComponent::openMode |
            juce::FileBrowserComponent::canSelectFiles;
        chooser_->launchAsync(settingOpenFlags, [this](const juce::FileChooser& chooser) {
            if (chooser.getResults().size() <= 0) { return; }
            const juce::File preset_file(chooser.getResult());
            if (!preset_file.existsAsFile()) { return; }
            if (juce::FileInputStream input(preset_file); input.openedOk()) {
                preset_freqs_.resize(0);
                preset_dbs_.resize(0);
                while (!input.isExhausted()) {
                    const auto line = input.readNextLine().toStdString();
                    try {
                        const size_t comma_pos = line.find(',');
                        if (comma_pos == std::string::npos) {
                            break;
                        }
                        std::string freq_str = line.substr(0, comma_pos);
                        std::string db_str = line.substr(comma_pos + 1);
                        float freq = std::stof(freq_str);
                        float db = std::stof(db_str);
                        preset_freqs_.emplace_back(freq);
                        preset_dbs_.emplace_back(db);
                    }
                    catch (const std::exception&) {
                        break;
                    }
                }
                if (preset_freqs_.size() < 2) {
                    return;
                }
                match_fft_panel_.loadFromPreset(preset_freqs_, preset_dbs_);
            }
        });
    }

    void MatchControlPanel::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property) {
        if (base_.isPanelIdentifier(zlgui::PanelSettingIdx::kMaximumNumBand, property)) {
            const auto max_num_band = static_cast<double>(
                base_.getPanelProperty(zlgui::PanelSettingIdx::kMaximumNumBand));
            num_band_slider_.getSlider().setNormalisableRange(juce::NormalisableRange<double>(
                0.0, max_num_band, 1.0));
        } else if (base_.isPanelIdentifier(zlgui::PanelSettingIdx::kSuggestedNumBand, property)) {
            const auto suggested_num_band = std::round(static_cast<double>(
                base_.getPanelProperty(zlgui::PanelSettingIdx::kSuggestedNumBand)));
            num_band_slider_.getSlider().setDoubleClickReturnValue(true, suggested_num_band);
            num_band_slider_.getSlider().setValue(suggested_num_band, juce::sendNotificationSync);
        } else if (base_.isPanelIdentifier(zlgui::PanelSettingIdx::kMatchPanel, property)) {
            const auto status_value = static_cast<double>(base_.getPanelProperty(zlgui::PanelSettingIdx::kMatchPanel));
            num_band_slider_.setVisible(status_value > 2.5);
            if (status_value < 0.5) {
                match_fft_panel_.resetAnalyzer();
            }
            resized();
            repaint();
        }
    }
}
