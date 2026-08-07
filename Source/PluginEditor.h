#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class VeloriaAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit VeloriaAudioProcessorEditor(VeloriaAudioProcessor&);
    ~VeloriaAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawStochasticGlobe(juce::Graphics&, juce::Rectangle<float> bounds);
    void drawEvolutionGraph(juce::Graphics&, juce::Rectangle<float> bounds);
    void drawPanel(juce::Graphics&, juce::Rectangle<float> bounds, const juce::String& title);
    void drawKnobLabel(juce::Graphics&, juce::Slider&, const juce::String& text);
    void configureKnob(juce::Slider&);

    VeloriaAudioProcessor& audioProcessor;
    VeloriaAudioProcessor::VisualState visualState;

    juce::Slider ampWalk, timeWalk, ampMirror, timeMirror;
    juce::Slider attack, decay, sustain, release;
    juce::Slider seed, level;

    juce::ComboBox presetBox;
    juce::ToggleButton monoButton { "MONO" };
    juce::TextButton discoverButton { "DISCOVER" };
    juce::Label title, brand, subtitle, fieldStatus, voiceStatus, footerStatus;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> ampWalkAttachment, timeWalkAttachment,
        ampMirrorAttachment, timeMirrorAttachment;
    std::unique_ptr<SliderAttachment> attackAttachment, decayAttachment,
        sustainAttachment, releaseAttachment, seedAttachment, levelAttachment;
    std::unique_ptr<ButtonAttachment> monoAttachment;

    float rotationPhase { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VeloriaAudioProcessorEditor)
};
