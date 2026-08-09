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

    class EngineModeButton final : public juce::TextButton,
                                   private juce::Timer
    {
    public:
        EngineModeButton(VeloriaAudioProcessorEditor& editorToUse,
                         VeloriaAudioProcessor& processorToUse)
            : juce::TextButton("ENGINE: VELORIA"),
              editor(editorToUse), processor(processorToUse)
        {
            setTooltip("Switch between Veloria's modern DSS model and the Andrew R. Brown / Greg Jenkins IDSS-inspired operating model.");
            onClick = [this]
            {
                processor.setBrownIdssMode(! processor.isBrownIdssMode());
                editor.presetStatus.setText(processor.isBrownIdssMode()
                    ? "BROWN IDSS: INTERACTIVE DSS OPERATING MODEL"
                    : "VELORIA: MODERN DSS OPERATING MODEL",
                    juce::dontSendNotification);
                updateAppearance();
            };
            startTimerHz(12);
        }

    private:
        void timerCallback() override
        {
            if (getParentComponent() == nullptr)
                editor.addAndMakeVisible(*this);

            setBounds(103, 600, 215, 20);
            setVisible(! editor.whatIfOpen);
            updateAppearance();
        }

        void updateAppearance()
        {
            const auto brown = processor.isBrownIdssMode();
            setButtonText(brown ? "ENGINE: BROWN IDSS" : "ENGINE: VELORIA");
            const auto active = brown ? juce::Colour::fromRGB(255, 184, 86)
                                      : juce::Colour::fromRGB(176, 77, 255);
            setColour(juce::TextButton::buttonColourId, active.withAlpha(brown ? 0.24f : 0.16f));
            setColour(juce::TextButton::textColourOffId,
                      (brown ? juce::Colour::fromRGB(255, 184, 86) : juce::Colours::white).withAlpha(0.92f));
        }

        VeloriaAudioProcessorEditor& editor;
        VeloriaAudioProcessor& processor;
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
    EngineModeButton engineButton { *this, audioProcessor };
    static constexpr int firstUserPresetId = 1001;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VeloriaAudioProcessorEditor)
};
