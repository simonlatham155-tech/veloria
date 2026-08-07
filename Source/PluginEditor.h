#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class VeloriaAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit VeloriaAudioProcessorEditor(VeloriaAudioProcessor&);
    ~VeloriaAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    class AuroraLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                              float sliderPosProportional, float rotaryStartAngle,
                              float rotaryEndAngle, juce::Slider&) override;
    };

    void timerCallback() override;
    void drawStochasticGlobe(juce::Graphics&, juce::Rectangle<float> bounds);
    void drawEvolutionGraph(juce::Graphics&, juce::Rectangle<float> bounds);
    void drawPanel(juce::Graphics&, juce::Rectangle<float> bounds, const juce::String& title);
    void drawKnobLabel(juce::Graphics&, juce::Slider&, const juce::String& text);
    void configureKnob(juce::Slider&);

    VeloriaAudioProcessor& audioProcessor;
    VeloriaAudioProcessor::VisualState visualState;
    AuroraLookAndFeel auroraLookAndFeel;

    juce::Slider ampWalk, timeWalk, ampMirror, timeMirror;
    juce::Slider attack, decay, sustain, release;
    juce::Slider seed, level;

    juce::ComboBox presetBox;
    juce::ToggleButton monoButton { "MONO" };
    juce::TextButton discoverButton { "DISCOVER" };
    juce::TextButton newFieldButton { "NEW FIELD" };
    juce::Label title, brand, subtitle, fieldStatus, voiceStatus, footerStatus;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> ampWalkAttachment, timeWalkAttachment,
        ampMirrorAttachment, timeMirrorAttachment;
    std::unique_ptr<SliderAttachment> attackAttachment, decayAttachment,
        sustainAttachment, releaseAttachment, seedAttachment, levelAttachment;
    std::unique_ptr<ButtonAttachment> monoAttachment;

    juce::Random uiRandom { 0x56454c4f };
    float rotationPhase { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VeloriaAudioProcessorEditor)
};
