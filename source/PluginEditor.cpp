// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#include "PluginEditor.hpp"

#include "BinaryData.h"

PluginEditor::PluginEditor(PluginProcessor& p) :
    AudioProcessorEditor(&p),
    p_ref_(p),
    state_(dummy_processor_, nullptr,
           juce::Identifier(zlstate::schema::kUISettings),
           zlstate::getStateParameterLayout()),
    property_(state_),
    base_(state_),
    main_panel_(p, base_, static_cast<zlpanel::multilingual::TooltipLanguage>(std::round(
                    zlpanel::getValue(state_, zlstate::PTooltipLang::kID)))) {
    setOpaque(true);

#if defined(JUCE_WINDOWS)
    base_.font_ = juce::Typeface::createSystemTypefaceFor(
        BinaryData::InterSubsetMediumNoHinting_ttf, BinaryData::InterSubsetMediumNoHinting_ttfSize);
#else
    base_.font_ = juce::Typeface::createSystemTypefaceFor(
        BinaryData::InterSubsetMedium_ttf, BinaryData::InterSubsetMedium_ttfSize);
#endif
    juce::LookAndFeel::getDefaultLookAndFeel().setDefaultSansSerifTypeface(base_.font_);

    // Reference-locked live palette. These values are deliberately brighter and less
    // desaturated than the v1.8 pass because the approved image has a visible steel-blue
    // body and a much clearer analyzer against it.
    base_.setColourByIdx(zlgui::kTextColour,       juce::Colour(242, 248, 252));
    base_.setColourByIdx(zlgui::kBackgroundColour, juce::Colour(31, 63, 91));
    base_.setColourByIdx(zlgui::kShadowColour,     juce::Colour(5, 14, 25));
    base_.setColourByIdx(zlgui::kGlowColour,       juce::Colour(177, 218, 247).withAlpha(0.38f));
    base_.setColourByIdx(zlgui::kGridColour,       juce::Colour(219, 237, 248).withAlpha(0.075f));
    base_.setColourByIdx(zlgui::kPreColour,        juce::Colour(192, 211, 226).withAlpha(0.205f));
    base_.setColourByIdx(zlgui::kPostColour,       juce::Colour(231, 243, 251).withAlpha(0.295f));
    base_.setColourByIdx(zlgui::kSideColour,       juce::Colour(197, 186, 232).withAlpha(0.155f));
    base_.setColourByIdx(zlgui::kCollisionColour,  juce::Colour(255, 132, 153));

    addAndMakeVisible(main_panel_);
    main_panel_.getControlPanel().addMouseListener(this, true);
    main_panel_.getOutputPanel().addMouseListener(this, true);

    // The target body is very close to 2:1 once the host chrome is excluded.
    constexpr double glass_aspect = 2.00;
    setResizeLimits(708,
                    static_cast<int>(zlstate::PWindowH::kMinV - 1),
                    static_cast<int>(zlstate::PWindowW::kMaxV + 1),
                    static_cast<int>(zlstate::PWindowH::kMaxV + 1));
    getConstrainer()->setFixedAspectRatio(glass_aspect);
    setResizable(true, p.wrapperType != PluginProcessor::wrapperType_AudioUnitv3);

    this->resizableCorner = std::make_unique<zlgui::ResizeCorner>(base_, this, getConstrainer(),
                                                                  zlgui::ResizeCorner::kScaleWithFontSize, 1.25f);
    addChildComponent(this->resizableCorner.get());
    this->resizableCorner->setAlwaysOnTop(true);
    this->resizableCorner->resized();

    last_ui_width_.referTo(state_.getParameterAsValue(zlstate::PWindowW::kID));
    last_ui_height_.referTo(state_.getParameterAsValue(zlstate::PWindowH::kID));
    const auto initial_width = juce::jmax(840, static_cast<int>(last_ui_width_.getValue()));
    setSize(initial_width, juce::roundToInt(static_cast<double>(initial_width) / glass_aspect));

    startTimer(kVisibilityTimer, 1000);
    updateIsShowing();

    base_.setPanelProperty(zlgui::kUISettingChanged, true);
    base_.getPanelValueTree().addListener(this);
    sendLookAndFeelChange();
}

PluginEditor::~PluginEditor() {
    base_.getPanelValueTree().removeListener(this);
    flushPendingPropertySave();
    vblank_.reset();
    stopTimer(kVisibilityTimer);
    p_ref_.getController().setEditorON(false);
}

void PluginEditor::paint(juce::Graphics& g) {
    juce::ignoreUnused(g);
}

void PluginEditor::resized() {
    main_panel_.setBounds(getLocalBounds());
    if (!base_.getWindowSizeFix()) {
        const auto width = std::clamp(getWidth(),
                                      static_cast<int>(zlstate::PWindowW::kMinV),
                                      static_cast<int>(zlstate::PWindowW::kMaxV));
        const auto height = std::clamp(getHeight(),
                                       static_cast<int>(zlstate::PWindowH::kMinV),
                                       static_cast<int>(zlstate::PWindowH::kMaxV));
        const bool size_changed = (width != static_cast<int>(last_ui_width_.getValue())) ||
            (height != static_cast<int>(last_ui_height_.getValue()));
        last_ui_width_ = width;
        last_ui_height_ = height;
        triggerAsyncUpdate();
        if (size_changed) schedulePropertySave();
    }
}

void PluginEditor::visibilityChanged() { updateIsShowing(); }
void PluginEditor::parentHierarchyChanged() { updateIsShowing(); }
void PluginEditor::minimisationStateChanged(bool) { updateIsShowing(); }

void PluginEditor::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property) {
    if (base_.isPanelIdentifier(zlgui::kUISettingChanged, property)) {
        triggerAsyncUpdate();
        schedulePropertySave();
    }
}

void PluginEditor::handleAsyncUpdate() { sendLookAndFeelChange(); }

void PluginEditor::timerCallback(const int timer_id) {
    if (timer_id == kVisibilityTimer) {
        updateIsShowing();
    } else if (timer_id == kPropertySaveTimer) {
        flushPendingPropertySave();
    }
}

void PluginEditor::schedulePropertySave() {
    startTimer(kPropertySaveTimer, kPropertySaveDelayMS);
}

void PluginEditor::flushPendingPropertySave() {
    if (isTimerRunning(kPropertySaveTimer)) {
        stopTimer(kPropertySaveTimer);
        property_.saveAPVTS(state_);
    }
}

void PluginEditor::updateIsShowing() {
    const auto is_showing = isShowing();
    if (is_showing != base_.getIsEditorShowing()) {
        base_.setIsEditorShowing(is_showing);
        p_ref_.getController().setEditorON(is_showing);
        if (is_showing) {
            main_panel_.startThreads();
            vblank_ = std::make_unique<juce::VBlankAttachment>(
                &main_panel_, [this](const double x) { main_panel_.repaintCallBack(x); });
        } else {
            vblank_.reset();
            main_panel_.stopThreads();
        }
    }
}

int PluginEditor::getControlParameterIndex(Component& c) {
    const auto id = c.getComponentID();
    if (id.isEmpty()) return -1;
    if (const auto para = p_ref_.parameters_.getParameter(id); para == nullptr) {
        return -1;
    } else {
        return para->getParameterIndex();
    }
}

void PluginEditor::mouseDown(const juce::MouseEvent& event) {
    if (event.mods.isRightButtonDown() && event.getNumberOfClicks() == 1) {
        if (event.originalComponent != nullptr) {
            if (const auto id = event.originalComponent->getComponentID(); !id.isEmpty()) {
                if (const auto para = p_ref_.parameters_.getParameter(id); para != nullptr) {
                    if (const auto* context = getHostContext(); context != nullptr) {
                        if (auto menu = context->getContextMenuForParameter(para)) {
                            menu->showNativeMenu(juce::Component::getMouseXYRelative());
                        }
                    }
                }
            }
        }
    }
}
