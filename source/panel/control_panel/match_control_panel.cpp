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
        close_button_(base, "v"),
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

        close_button_.getLAF().setFontScale(.66f);
        close_button_.getLAF().setJustification(juce::Justification::centred);
        close_button_.setBackgroundPainter([](juce::Graphics& g, juce::Button& b,
                                               bool highlighted, bool down) {
            const auto surface = b.getLocalBounds().toFloat().reduced(.5f);
            zlgui::glass::fillGlassSurface(g, surface, surface.getHeight() * .5f,
                highlighted || down ? .08f : .04f, .08f, .12f);
        });
        close_button_.getButton().onClick = [this]() {
            base_.setPanelProperty(zlgui::PanelSettingIdx::kMatchPanel, 0.0);
        };
        addAndMakeVisible(close_button_);

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
        target_box_.getLAF().setFontScale(.65f);
        target_box_.getLAF().setBoxAlpha(.20f);
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

        label_laf_.setFontScale(.52f);
        for (auto& l : {&shift_label_, &scale_label_, &slope_label_}) {
            l->setLookAndFeel(&label_laf_);
            l->setJustificationType(juce::Justification::centredLeft);
            l->setAlpha(.72f);
            l->setBufferedToImage(true);
            addAndMakeVisible(l);
        }

        for (auto& s : {&shift_slider_, &scale_slider_, &slope_slider_}) {
            s->setFontScale(.67f);
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
        limit_combobox_.getLAF().setFontScale(.65f);
        limit_combobox_.getLAF().setBoxAlpha(.20f);
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
        num_band_slider_.setFontScale(.67f);
        num_band_slider_.getSlider().setSliderSnapsToMousePosition(false);
        num_band_slider_.setBufferedToImage(true);
        addChildComponent(num_band_slider_);

        base_.getPanelValueTree().addListener(this);
    }

    MatchControlPanel::~MatchControlPanel() {
        base_.getPanelValueTree().removeListener(this);
    }

    int MatchControlPanel::getIdealHeight() const {
        return juce::jmax(72, juce::roundToInt(base_.getFontSize() * 4.85f));
    }

    int MatchControlPanel::getIdealWidth() const {
        return juce::roundToInt(base_.getFontSize() * 31.5f);
    }

    void MatchControlPanel::paint(juce::Graphics& g) {
        const auto surface = getLocalBounds().toFloat().reduced(.5f);
        const auto radius = juce::jmax(10.f, base_.getFontSize() * .76f);
        zlgui::glass::fillGlassSurface(g, surface, radius, .072f, .12f, .14f);
        juce::Path lens;
        lens.addRoundedRectangle(surface, radius);
        {
            juce::Graphics::ScopedSaveState state(g);
            g.reduceClipRegion(lens);
            for (const auto& light : node_lights_) {
                juce::ColourGradient field(light.colour.withAlpha(.065f),
                    light.point.x, light.point.y, light.colour.withAlpha(0.f),
                    light.point.x + light.radius, light.point.y, true);
                field.addColour(.45, light.colour.withAlpha(.027f));
                g.setGradientFill(field);
                g.fillRect(surface);
            }
        }
        g.setColour(juce::Colours::white.withAlpha(.18f));
        g.drawRoundedRectangle(surface.reduced(.6f), radius, .8f);
    }

    void MatchControlPanel::setNodeLights(std::vector<NodeLight> lights) {
        node_lights_ = std::move(lights);
        repaint();
    }

    void MatchControlPanel::resized() {
        const auto font = base_.getFontSize();
        const auto pad = juce::jmax(4, juce::roundToInt(font * .34f));
        const auto gap = juce::jmax(3, juce::roundToInt(font * .24f));
        const auto row_height = juce::jmax(20, juce::roundToInt(font * 1.42f));
        const auto label_height = juce::jmax(9, juce::roundToInt(font * .55f));

        auto content = getLocalBounds().reduced(pad);
        auto header = content.removeFromTop(row_height);
        title_bound_ = header.removeFromLeft(juce::roundToInt(font * 5.35f));
        header.removeFromLeft(gap);
        close_button_.setBounds(header.removeFromRight(row_height));
        header.removeFromRight(gap);
        save_button_.setBounds(header.removeFromRight(row_height));
        header.removeFromRight(gap);
        draw_button_.setBounds(header.removeFromRight(row_height));
        header.removeFromRight(gap);
        target_label_bound_ = header.removeFromLeft(juce::roundToInt(font * 3.0f));
        target_box_.setBounds(header.removeFromLeft(juce::jmin(header.getWidth(),
            juce::roundToInt(font * 7.4f))));
        subtitle_bound_ = header.reduced(gap, 0);

        content.removeFromTop(gap);
        const auto fit_width = juce::roundToInt(font * 4.8f);
        const auto cell_width = juce::jmax(1,
            (content.getWidth() - fit_width - 5 * gap) / 5);
        auto take_cell = [&](juce::Rectangle<int>& remaining) {
            auto cell = remaining.removeFromLeft(juce::jmin(cell_width, remaining.getWidth()));
            remaining.removeFromLeft(juce::jmin(gap, remaining.getWidth()));
            return cell;
        };
        auto shift = take_cell(content);
        auto scale = take_cell(content);
        auto slope = take_cell(content);
        auto limit = take_cell(content);
        auto bands = take_cell(content);
        fit_start_button_.setBounds(content.withSizeKeepingCentre(
            juce::jmin(fit_width, content.getWidth()),
            juce::jmin(row_height + label_height, content.getHeight())));

        shift_label_.setBounds(shift.removeFromTop(label_height));
        scale_label_.setBounds(scale.removeFromTop(label_height));
        slope_label_.setBounds(slope.removeFromTop(label_height));
        limit_label_bound_ = limit.removeFromTop(label_height);
        bands_label_bound_ = bands.removeFromTop(label_height);
        shift_slider_.setBounds(shift.removeFromTop(row_height));
        scale_slider_.setBounds(scale.removeFromTop(row_height));
        slope_slider_.setBounds(slope.removeFromTop(row_height));
        limit_combobox_.setBounds(limit.removeFromTop(row_height));
        num_band_slider_.setBounds(bands.removeFromTop(row_height));
        for (auto* slider : {&shift_slider_, &scale_slider_, &slope_slider_, &num_band_slider_})
            slider->getSlider().setMouseDragSensitivity(juce::jmax(80, cell_width));
    }

    void MatchControlPanel::paintOverChildren(juce::Graphics& g) {
        const auto font = base_.getFontSize();
        g.setColour(zlgui::glass::textPrimary().withAlpha(.94f));
        g.setFont(juce::FontOptions(font * .67f));
        g.drawText("EQ Match", title_bound_, juce::Justification::centredLeft, false);

        g.setColour(zlgui::glass::textSecondary().withMultipliedAlpha(.86f));
        g.setFont(juce::FontOptions(font * .51f));
        g.drawText("Reference curve", subtitle_bound_,
                   juce::Justification::centredLeft, false);

        g.setColour(zlgui::glass::textSecondary().withMultipliedAlpha(.78f));
        g.setFont(juce::FontOptions(font * .49f));
        g.drawText("TARGET", target_label_bound_, juce::Justification::centred, false);
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
