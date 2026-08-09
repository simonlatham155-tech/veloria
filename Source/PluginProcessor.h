#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cmath>
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
    void newField();
    juce::StringArray getFactoryPresetNames() const;
    VisualState getVisualState() const noexcept;
    bool isDrumMode() const noexcept { return drumMode; }

    void setBrownIdssMode(bool shouldUseBrownModel) noexcept
    {
        brownIdssMode.store(shouldUseBrownModel, std::memory_order_relaxed);
        const auto model = shouldUseBrownModel
            ? veloria::dsp::StochasticOscillator::OperatingModel::brownIdss
            : veloria::dsp::StochasticOscillator::OperatingModel::veloria;
        for (auto& voice : voices)
            voice.oscillator.setOperatingModel(model);
    }

    bool isBrownIdssMode() const noexcept
    {
        return brownIdssMode.load(std::memory_order_relaxed);
    }

    void beginMidiLearn(const juce::String& parameterId) noexcept;
    void clearMidiMapping(const juce::String& parameterId) noexcept;
    int getMidiCCForParameter(const juce::String& parameterId) const noexcept;

    juce::StringArray getUserPresetNames() const;
    bool saveUserPreset(const juce::String& name);
    bool loadUserPreset(const juce::String& name);
    bool renameUserPreset(const juce::String& oldName, const juce::String& newName);
    bool deleteUserPreset(const juce::String& name);

    mutable juce::AudioProcessorValueTreeState parameters;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    enum class DrumKind
    {
        none,
        kick,
        snareBody,
        snareWire,
        closedHat,
        openHat,
        crash,
        tom
    };

    // Deterministic tonal envelope owned by Veloria rather than JUCE's ADSR state
    // machine. Note-off captures the current level and performs a finite linear
    // release to exactly zero, after which isActive() is false. This gives the voice
    // lifecycle one authoritative end condition and prevents stuck tonal voices.
    class StableADSR
    {
    public:
        void setSampleRate(double newSampleRate) noexcept
        {
            sampleRate = juce::jmax(1.0, newSampleRate);
        }

        void setParameters(const juce::ADSR::Parameters& newParameters) noexcept
        {
            parameters.attack = juce::jmax(0.0f, newParameters.attack);
            parameters.decay = juce::jmax(0.0f, newParameters.decay);
            parameters.sustain = juce::jlimit(0.0f, 1.0f, newParameters.sustain);
            parameters.release = juce::jmax(0.0f, newParameters.release);
        }

        void reset() noexcept
        {
            state = State::idle;
            level = 0.0f;
            releaseDelta = 0.0f;
            releaseSamplesRemaining = 0;
        }

        void noteOn() noexcept
        {
            level = 0.0f;
            state = State::attack;
            releaseDelta = 0.0f;
            releaseSamplesRemaining = 0;
        }

        void noteOff() noexcept
        {
            if (state == State::idle)
                return;

            const auto releaseSamples = juce::jmax<std::uint64_t>(
                1,
                static_cast<std::uint64_t>(std::ceil(static_cast<double>(parameters.release) * sampleRate)));
            releaseSamplesRemaining = releaseSamples;
            releaseDelta = level / static_cast<float>(releaseSamples);
            state = State::release;
        }

        float getNextSample() noexcept
        {
            switch (state)
            {
                case State::idle:
                    level = 0.0f;
                    break;

                case State::attack:
                {
                    const auto samples = juce::jmax(1.0, static_cast<double>(parameters.attack) * sampleRate);
                    level += static_cast<float>(1.0 / samples);
                    if (level >= 1.0f)
                    {
                        level = 1.0f;
                        state = State::decay;
                    }
                    break;
                }

                case State::decay:
                {
                    const auto target = parameters.sustain;
                    const auto samples = juce::jmax(1.0, static_cast<double>(parameters.decay) * sampleRate);
                    const auto delta = static_cast<float>((1.0 - static_cast<double>(target)) / samples);
                    level -= delta;
                    if (level <= target)
                    {
                        level = target;
                        state = State::sustain;
                    }
                    break;
                }

                case State::sustain:
                    level = parameters.sustain;
                    break;

                case State::release:
                    if (releaseSamplesRemaining > 0)
                    {
                        level = juce::jmax(0.0f, level - releaseDelta);
                        --releaseSamplesRemaining;
                    }
                    if (releaseSamplesRemaining == 0 || level <= 1.0e-7f)
                        reset();
                    break;
            }

            return level;
        }

        bool isActive() const noexcept { return state != State::idle; }

    private:
        enum class State { idle, attack, decay, sustain, release };
        juce::ADSR::Parameters parameters {};
        State state { State::idle };
        double sampleRate { 44100.0 };
        float level { 0.0f };
        float releaseDelta { 0.0f };
        std::uint64_t releaseSamplesRemaining { 0 };
    };

    struct Voice
    {
        veloria::dsp::StochasticOscillator oscillator;
        StableADSR envelope;
        int midiNote { -1 };
        bool active { false };
        bool percussion { false };
        std::uint64_t age { 0 };

        DrumKind drumKind { DrumKind::none };
        std::uint64_t percussionSample { 0 };
        std::uint64_t percussionLengthSamples { 1 };
        std::uint64_t percussionAttackSamples { 1 };
        float startAmpWalk { 0.0f };
        float endAmpWalk { 0.0f };
        float startTimeWalk { 0.0f };
        float endTimeWalk { 0.0f };
        float amplitudeMirror { 0.88f };
        float timeMirror { 0.45f };
        float startFrequency { 220.0f };
        float endFrequency { 220.0f };
        float contractionPower { 2.0f };
        float pitchPower { 2.0f };
        float decayPower { 2.0f };
        float gain { 1.0f };
        float pressure { 0.0f };
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
    static constexpr int midiLearnParameterCount = 18;
    static constexpr int drumPresetIndex = 9;
    static const std::array<FactoryPreset, 10> factoryPresets;
    static const std::array<const char*, midiLearnParameterCount> midiLearnParameterIds;

    void startNote(int midiNote, float velocity);
    void startDrumNote(int midiNote, float velocity);
    void configureDrumVoice(Voice& voice, DrumKind kind, int midiNote, float velocity, int layerIndex = 0);
    void stopNote(int midiNote);
    void stopAllVoices(bool allowTailOff);
    Voice& findVoiceToStart();
    void updateVoiceParameters();
    void applyFactoryPreset(int index);
    void discoverDrumField();
    void setParameterValue(const juce::String& id, float value);
    void setParameterFromMidi(int parameterIndex, float normalisedValue) noexcept;
    int findMidiParameterIndex(const juce::String& id) const noexcept;
    void handleMidiController(const juce::MidiMessage& message) noexcept;
    void publishVisualState(float energy) noexcept;

    juce::File getUserPresetDirectory() const;
    juce::File getUserPresetFile(const juce::String& name) const;
    juce::ValueTree makeSerializableState() const;
    void restoreSerializableState(const juce::ValueTree& state);
    void appendMidiMappingsToState(juce::ValueTree& state) const;
    void restoreMidiMappingsFromState(const juce::ValueTree& state);

    std::array<Voice, maxVoices> voices;
    juce::dsp::Gain<float> outputGain;
    juce::Random discoveryRandom { 0x56454c4f };
    std::uint64_t voiceCounter { 0 };
    int currentProgram { 0 };
    bool drumMode { false };
    std::atomic<bool> brownIdssMode { false };
    double currentSampleRate { 44100.0 };

    std::array<std::atomic<float>, visualBreakpointCount> visualAmplitudes {};
    std::array<std::atomic<float>, visualBreakpointCount> visualDurations {};
    std::atomic<int> visualActiveVoices { 0 };
    std::atomic<float> visualEnergy { 0.0f };

    std::array<std::atomic<int>, midiLearnParameterCount> midiCCMappings {};
    std::atomic<int> midiLearnTarget { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VeloriaAudioProcessor)
};