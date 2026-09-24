#include "PluginProcessor.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(__APPLE__)
#include <mach/mach.h>
#endif

namespace {
    size_t residentBytes() {
#if defined(__APPLE__)
        mach_task_basic_info_data_t info{};
        mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                      reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS)
            return static_cast<size_t>(info.resident_size);
#endif
        return 0;
    }
}

int main(int argc, char** argv) {
    const int instance_count = argc > 1 ? std::max(1, std::atoi(argv[1])) : 200;
    const int active_bands = argc > 2 ? std::clamp(std::atoi(argv[2]), 0, static_cast<int>(zlp::kBandNum)) : 4;
    const int block_count = argc > 3 ? std::max(1, std::atoi(argv[3])) : 300;
    const int dynamic_bands = argc > 4 ? std::clamp(std::atoi(argv[4]), 0, active_bands) : 0;
    const int structure = argc > 5 ? std::clamp(std::atoi(argv[5]), 0, 5) : 0;
    constexpr int block_size = 512;
    constexpr double sample_rate = 48000.0;
    juce::ScopedJuceInitialiser_GUI juce_init;

    std::vector<std::unique_ptr<PluginProcessor>> instances;
    instances.reserve(static_cast<size_t>(instance_count));
    const auto initial_resident_bytes = residentBytes();
    const auto preparation_start = std::chrono::steady_clock::now();
    for (int i = 0; i < instance_count; ++i) {
        instances.push_back(std::make_unique<PluginProcessor>());
    }
    const auto constructed_resident_bytes = residentBytes();
    for (auto& instance : instances) {
        instance->prepareToPlay(sample_rate, block_size);
        const auto set_parameter = [&](const juce::String& id, float value) {
            auto* parameter = instance->parameters_.getParameter(id);
            if (parameter == nullptr) throw std::runtime_error("Missing benchmark parameter");
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        };
        set_parameter(zlp::PFilterStructure::kID, static_cast<float>(structure));
        for (int band = 0; band < active_bands; ++band) {
            const auto suffix = std::to_string(band);
            set_parameter(zlp::PFilterStatus::kID + suffix, static_cast<float>(zlp::FilterStatus::kOn));
            set_parameter(zlp::PGain::kID + suffix, band % 2 == 0 ? 3.f : -3.f);
            set_parameter(zlp::PFreq::kID + suffix, 80.f * std::pow(1.22f, static_cast<float>(band)));
            if (band < dynamic_bands)
                set_parameter(zlp::PDynamicON::kID + suffix, 1.f);
        }
    }
    const auto resident_bytes = residentBytes();
    const auto preparation_seconds = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - preparation_start).count();

    juce::AudioBuffer<float> buffer(4, block_size);
    juce::MidiBuffer midi;
    double checksum = 0.0;
    double max_abs_output = 0.0;
    double processing_seconds = 0.0;
    std::array<float, block_size> left_input{}, right_input{};
    for (int block = 0; block < block_count; ++block) {
        for (int sample = 0; sample < block_size; ++sample) {
            const float input = .15f * std::sin(static_cast<float>((block * block_size + sample) * .021));
            left_input[static_cast<size_t>(sample)] = input;
            right_input[static_cast<size_t>(sample)] = input * .8f;
            buffer.setSample(2, sample, input * .7f);
            buffer.setSample(3, sample, input * .6f);
        }
        for (const auto& instance : instances) {
            buffer.copyFrom(0, 0, left_input.data(), block_size);
            buffer.copyFrom(1, 0, right_input.data(), block_size);
            const auto start = std::chrono::steady_clock::now();
            instance->processBlock(buffer, midi);
            processing_seconds += std::chrono::duration<double>(
                std::chrono::steady_clock::now() - start).count();
            const auto output = static_cast<double>(buffer.getSample(0, block % block_size));
            checksum += output;
            max_abs_output = std::max(max_abs_output, std::abs(output));
        }
    }
    const auto audio_seconds = static_cast<double>(block_count * block_size) / sample_rate;
    std::cout << "instances=" << instance_count << " active_bands=" << active_bands
              << " dynamic_bands=" << dynamic_bands
              << " structure=" << structure
              << " blocks=" << block_count << " processing_s=" << processing_seconds
              << " aggregate_realtime_ratio=" << processing_seconds / audio_seconds
              << " preparation_s=" << preparation_seconds
              << " processor_size=" << sizeof(PluginProcessor)
              << " initial_mib=" << initial_resident_bytes / (1024.0 * 1024.0)
              << " constructed_mib=" << constructed_resident_bytes / (1024.0 * 1024.0)
              << " resident_mib=" << resident_bytes / (1024.0 * 1024.0)
              << " checksum=" << checksum << " max_abs_output=" << max_abs_output << '\n';
}
