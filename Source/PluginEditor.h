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
        void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
                              float sliderPos, float minSliderPos, float maxSliderPos,
                              juce::Slider::SliderStyle, juce::Slider&) override;
    };

    class MidiLearnSlider final : public juce::Slider
    {
    public:
        std::function<void()> learnCallback;
        std::function<void()> clearCallback;
        std::function<int()> currentCCCallback;
        void mouseDown(const juce::MouseEvent&) override;
    };

    void timerCallback() override;
    void drawStochasticGlobe(juce::Graphics&, juce::Rectangle<float> bounds);
    void drawEvolutionGraph(juce::Graphics&, juce::Rectangle<float> bounds);
    void drawPanel(juce::Graphics&, juce::Rectangle<float> bounds, const juce::String& title);
    void drawWhatIfOverlay(juce::Graphics&);
    void setWhatIfMode(bool shouldOpen);
    void configureKnob(MidiLearnSlider&, const juce::String& parameterId, const juce::String& displayName);
    void configureFieldSlider(MidiLearnSlider&, const juce::String& parameterId, const juce::String& displayName);
    void refreshPresetBox(const juce::String& selectUserPreset = {});
    bool selectedPresetIsUser() const noexcept;
    void refreshOrderButton();

    VeloriaAudioProcessor& audioProcessor;
    VeloriaAudioProcessor::VisualState visualState;
    AuroraLookAndFeel auroraLookAndFeel;

    MidiLearnSlider ampWalk, timeWalk, ampMirror, timeMirror;
    MidiLearnSlider ampDist, timeDist, ampStep, timeStep;
    MidiLearnSlider chaos, breakpoints, pitchStability, curve;
    MidiLearnSlider attack, decay, sustain, release;
    MidiLearnSlider seed, level;

    juce::ComboBox presetBox;
    juce::ToggleButton monoButton { "MONO" };
    juce::TextButton orderButton { "ORDER 2" };
    juce::TextButton discoverButton { "DISCOVER" };
    juce::TextButton newFieldButton { "NEW FIELD" };
    juce::TextButton whatIfButton { "WHAT IF?" };
    juce::TextButton savePresetButton { "SAVE" };
    juce::TextButton renamePresetButton { "RENAME" };
    juce::TextButton deletePresetButton { "DELETE" };
    juce::TextEditor presetNameEditor;
    juce::Label title, brand, subtitle, fieldStatus, voiceStatus, footerStatus, presetStatus;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> ampWalkAttachment, timeWalkAttachment,
        ampMirrorAttachment, timeMirrorAttachment;
    std::unique_ptr<SliderAttachment> ampDistAttachment, timeDistAttachment,
        ampStepAttachment, timeStepAttachment, chaosAttachment,
        breakpointsAttachment, pitchStabilityAttachment, curveAttachment;
    std::unique_ptr<SliderAttachment> attackAttachment, decayAttachment,
        sustainAttachment, releaseAttachment, seedAttachment, levelAttachment;
    std::unique_ptr<ButtonAttachment> monoAttachment;

    float rotationPhase { 0.0f };
    bool whatIfOpen { false };
    static constexpr int firstUserPresetId = 1001;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VeloriaAudioProcessorEditor)
};
