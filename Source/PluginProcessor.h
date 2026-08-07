#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cstdint>
#include "dsp/StochasticOscillator.h"

class VeloriaAudioProcessor final : public juce::AudioProcessor
{
public:
    static constexpr std::size_t visualBreakpointCount = veloria::dsp::StochasticOscillator::numBreakpoints;

    struct VisualState
    {
        std::array<float, visualBreakpointCount> amplitudes {};
        std::array<float, visualBreakpointCount> durations {};
        int activeVoices { 0 };
        float energy { 0.0f };
    };

    VeloriaAudioProcessor();
    ~VeloriaAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    void discover();
    juce::StringArray getFactoryPresetNames() const;
    VisualState getVisualState() const noexcept;

    void beginMidiLearn(const juce::String& parameterId) noexcept;
    void clearMidiMapping(const juce::String& parameterId) noexcept;
    int getMidiCCForParameter(const juce::String& parameterId) const noexcept;

    juce::StringArray getUserPresetNames() const;
    bool saveUserPreset(const juce::String& name);
    bool loadUserPreset(const juce::String& name);
    bool renameUserPreset(const juce::String& oldName, const juce::String& newName);
    bool deleteUserPreset(const juce::String& name);

    juce::AudioProcessorValueTreeState parameters;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    struct Voice
    {
        veloria::dsp::StochasticOscillator oscillator;
        juce::ADSR envelope;
        int midiNote { -1 };
        bool active { false };
        std::uint64_t age { 0 };
    };

    struct FactoryPreset
    {
        const char* name;
        float ampWalk;
        float timeWalk;
        float ampMirror;
        float timeMirror;
        float attack;
        float decay;
        float sustain;
        float release;
        int seed;
    };

    static constexpr int maxVoices = 8;
    static constexpr int midiLearnParameterCount = 10;
    static const std::array<FactoryPreset, 10> factoryPresets;
    static const std::array<const char*, midiLearnParameterCount> midiLearnParameterIds;

    void startNote(int midiNote, float velocity);
    void stopNote(int midiNote);
    void stopAllVoices(bool allowTailOff);
    Voice& findVoiceToStart();
    void updateVoiceParameters();
    void applyFactoryPreset(int index);
    void setParameterValue(const juce::String& id, float value);
    void setParameterFromMidi(int parameterIndex, float normalisedValue) noexcept;
    int findMidiParameterIndex(const juce::String& id) const noexcept;
    void handleMidiController(const juce::MidiMessage& message) noexcept;
    void publishVisualState(float energy) noexcept;

    juce::File getUserPresetDirectory() const;
    juce::File getUserPresetFile(const juce::String& name) const;
    juce::ValueTree makeSerializableState();
    void restoreSerializableState(const juce::ValueTree& state);
    void appendMidiMappingsToState(juce::ValueTree& state) const;
    void restoreMidiMappingsFromState(const juce::ValueTree& state);

    std::array<Voice, maxVoices> voices;
    juce::dsp::Gain<float> outputGain;
    juce::Random discoveryRandom { 0x56454c4f };
    std::uint64_t voiceCounter { 0 };
    int currentProgram { 0 };

    std::array<std::atomic<float>, visualBreakpointCount> visualAmplitudes {};
    std::array<std::atomic<float>, visualBreakpointCount> visualDurations {};
    std::atomic<int> visualActiveVoices { 0 };
    std::atomic<float> visualEnergy { 0.0f };

    std::array<std::atomic<int>, midiLearnParameterCount> midiCCMappings {};
    std::atomic<int> midiLearnTarget { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VeloriaAudioProcessor)
};
