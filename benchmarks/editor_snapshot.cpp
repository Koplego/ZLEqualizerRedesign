#include "PluginProcessor.hpp"

#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

int main() {
    juce::ScopedJuceInitialiser_GUI juce_init;
    PluginProcessor processor;
    processor.prepareToPlay(48000.0, 512);
    for (int band = 0; band < 4; ++band) {
        const auto set = [&](const juce::String& id, float value) {
            auto* parameter = processor.parameters_.getParameter(id);
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        };
        const auto suffix = std::to_string(band);
        set(zlp::PFilterStatus::kID + suffix, static_cast<float>(zlp::FilterStatus::kOn));
        set(zlp::PGain::kID + suffix, band % 2 == 0 ? 4.f : -3.f);
        set(zlp::PFreq::kID + suffix, 60.f * std::pow(5.f, static_cast<float>(band) / 2.f));
    }
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    editor->setBounds(0, 0, 1440, 720);
    editor->setVisible(true);
    auto image = editor->createComponentSnapshot(editor->getLocalBounds());
    double total_seconds = 0.0;
    unsigned int checksum = 0;
    for (int i = 0; i < 20; ++i) {
        const auto start = std::chrono::steady_clock::now();
        image = editor->createComponentSnapshot(editor->getLocalBounds());
        total_seconds += std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start).count();
        checksum ^= image.getPixelAt((i * 47) % image.getWidth(),
                                     (i * 29) % image.getHeight()).getARGB();
    }
    std::cout << "snapshots=20 total_s=" << total_seconds
              << " average_ms=" << total_seconds * 50.0
              << " checksum=" << checksum << '\n';
}
