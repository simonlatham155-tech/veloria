#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class VeloriaAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit VeloriaAudioProcessorEditor(VeloriaAudioProcessor&);
    ~VeloriaAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    VeloriaAudioProcessor& processor;
    juce::Slider ampWalk, timeWalk, correlation, curve, level;
    juce::Label title, subtitle;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> ampWalkAttachment, timeWalkAttachment,
        correlationAttachment, curveAttachment, levelAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VeloriaAudioProcessorEditor)
};
