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

    juce::Slider ampWalk, timeWalk, ampMirror, timeMirror, level;
    juce::ComboBox presetBox;
    juce::ToggleButton monoButton { "MONO" };
    juce::TextButton discoverButton { "DISCOVER" };
    juce::Label title, subtitle, midiHint;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> ampWalkAttachment, timeWalkAttachment,
        ampMirrorAttachment, timeMirrorAttachment, levelAttachment;
    std::unique_ptr<ButtonAttachment> monoAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VeloriaAudioProcessorEditor)
};
