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
        int midiNoteOns { 0 };
        int midiNoteOffs { 0 };
        int midiNoteOffMatches { 0 };
    };

    VeloriaAudioProcessor();
    ~VeloriaAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    // CHAOS is implemented as a musical event layer in front of the existing
    // synthesis engine. It is intentionally shared by VELORIA and BROWN IDSS:
    // the chord defines the legal pitch classes, while the selected engine only
    // determines how each resulting note is synthesised.
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override
    {
        processChaosMidi(midi, buffer.getNumSamples());
        processBlockLegacy(buffer, midi);
    }
    void processBlockLegacy(juce::AudioBuffer<float>&, juce::MidiBuffer&);

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
        bool held { false };
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

    struct ChaosGeneratedNote
    {
        int midiNote { -1 };
        std::int64_t samplesRemaining { 0 };
        std::uint64_t age { 0 };
        bool active { false };
    };

    struct ChaosMidiEvent
    {
        int midiNote { 60 };
        int samplePosition { 0 };
        float velocity { 0.8f };
        bool noteOn { true };
    };

    static constexpr int maxVoices = 8;
    static constexpr int midiLearnParameterCount = 22;
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

    bool chaosPitchClassHeld(int pitchClass) const noexcept
    {
        for (int note = 0; note < 128; ++note)
            if (chaosHeldNotes[(std::size_t) note] && note % 12 == pitchClass)
                return true;
        return false;
    }

    bool chaosExactNoteHeld(int midiNote) const noexcept
    {
        return juce::isPositiveAndBelow(midiNote, 128)
            && chaosHeldNotes[(std::size_t) midiNote];
    }

    bool chaosGeneratedNoteActive(int midiNote) const noexcept
    {
        for (const auto& note : chaosGeneratedNotes)
            if (note.active && note.midiNote == midiNote)
                return true;
        return false;
    }

    int chooseChaosNote(float chaos) noexcept
    {
        std::array<int, 85> candidates {};
        int candidateCount = 0;
        int heldSum = 0;
        int heldCount = 0;

        for (int note = 0; note < 128; ++note)
        {
            if (chaosHeldNotes[(std::size_t) note])
            {
                heldSum += note;
                ++heldCount;
            }
        }

        if (heldCount == 0)
            return -1;

        const auto centre = heldSum / heldCount;
        const auto octaveRadius = chaos < 0.30f ? 12 : (chaos < 0.65f ? 24 : 36);
        const auto low = juce::jlimit(24, 108, centre - octaveRadius);
        const auto high = juce::jlimit(24, 108, centre + octaveRadius);

        for (int note = low; note <= high && candidateCount < (int) candidates.size(); ++note)
        {
            if (! chaosPitchClassHeld(note % 12))
                continue;
            if (chaosExactNoteHeld(note) || chaosGeneratedNoteActive(note))
                continue;
            candidates[(std::size_t) candidateCount++] = note;
        }

        if (candidateCount == 0)
            return -1;

        const auto sign = chaosNoteRandom.nextBool() ? 1 : -1;
        const auto r = chaosNoteRandom.nextFloat();
        int leap = 2 + chaosNoteRandom.nextInt(6);
        if (r < chaos * chaos * 0.10f)
            leap = 24 + chaosNoteRandom.nextInt(13);
        else if (r < chaos * chaos * 0.34f)
            leap = 12 + chaosNoteRandom.nextInt(13);

        const auto target = juce::jlimit(low, high, chaosLastGeneratedNote + sign * leap);
        int best = candidates[0];
        int bestDistance = std::abs(best - target);
        for (int i = 1; i < candidateCount; ++i)
        {
            const auto candidate = candidates[(std::size_t) i];
            const auto distance = std::abs(candidate - target);
            if (distance < bestDistance)
            {
                best = candidate;
                bestDistance = distance;
            }
        }

        chaosLastGeneratedNote = best;
        return best;
    }

    void processChaosMidi(juce::MidiBuffer& midi, int blockSamples) noexcept
    {
        if (blockSamples <= 0)
            return;

        std::array<ChaosMidiEvent, 48> pending {};
        int pendingCount = 0;
        bool hasHeldNotes = false;
        bool panic = false;

        // Inspect only the host/player MIDI before adding our own events. Generated
        // notes therefore never become harmonic authorities themselves.
        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();
            if (message.isNoteOn(false))
            {
                const auto note = message.getNoteNumber();
                if (juce::isPositiveAndBelow(note, 128))
                    chaosHeldNotes[(std::size_t) note] = true;
            }
            else if (message.isNoteOff(true))
            {
                const auto note = message.getNoteNumber();
                if (juce::isPositiveAndBelow(note, 128))
                    chaosHeldNotes[(std::size_t) note] = false;
            }
            else if (message.isAllNotesOff() || message.isAllSoundOff())
            {
                panic = true;
                chaosHeldNotes.fill(false);
            }
        }

        for (const auto held : chaosHeldNotes)
            hasHeldNotes = hasHeldNotes || held;

        auto queueOff = [&](int note, int sample)
        {
            if (pendingCount < (int) pending.size())
                pending[(std::size_t) pendingCount++] = { note, juce::jlimit(0, blockSamples - 1, sample), 0.0f, false };
        };

        // Advance autonomous note lifetimes. Their note-offs are independent and
        // intentionally abrupt at high CHAOS so voices can audibly fight/replace
        // one another rather than accumulating into an undifferentiated cluster.
        for (auto& generated : chaosGeneratedNotes)
        {
            if (! generated.active)
                continue;

            if (panic || ! hasHeldNotes)
            {
                queueOff(generated.midiNote, 0);
                generated = {};
                continue;
            }

            if (generated.samplesRemaining <= blockSamples)
            {
                queueOff(generated.midiNote, (int) juce::jmax<std::int64_t>(0, generated.samplesRemaining));
                generated = {};
            }
            else
            {
                generated.samplesRemaining -= blockSamples;
            }
        }

        if (drumMode || ! hasHeldNotes || panic)
        {
            chaosSpawnAccumulator = 0.0;
            for (int i = 0; i < pendingCount; ++i)
            {
                const auto& e = pending[(std::size_t) i];
                midi.addEvent(juce::MidiMessage::noteOff(1, e.midiNote), e.samplePosition);
            }
            return;
        }

        const auto chaos = juce::jlimit(0.0f, 1.0f,
            parameters.getRawParameterValue("chaos")->load());

        if (chaos <= 0.001f)
        {
            chaosSpawnAccumulator = 0.0;
            for (int i = 0; i < pendingCount; ++i)
            {
                const auto& e = pending[(std::size_t) i];
                midi.addEvent(juce::MidiMessage::noteOff(1, e.midiNote), e.samplePosition);
            }
            return;
        }

        const auto blockSeconds = (double) blockSamples / juce::jmax(1.0, currentSampleRate);
        const auto eventsPerSecond = 0.45 + 34.0 * std::pow((double) chaos, 1.72);
        chaosSpawnAccumulator += blockSeconds * eventsPerSecond;

        int spawnCount = (int) std::floor(chaosSpawnAccumulator);
        chaosSpawnAccumulator -= spawnCount;
        if (chaosNoteRandom.nextDouble() < chaosSpawnAccumulator * 0.18)
        {
            ++spawnCount;
            chaosSpawnAccumulator *= 0.35;
        }
        spawnCount = juce::jlimit(0, 6, spawnCount);

        for (int spawn = 0; spawn < spawnCount; ++spawn)
        {
            const auto note = chooseChaosNote(chaos);
            if (note < 0)
                break;

            const auto samplePosition = chaosNoteRandom.nextInt(juce::jmax(1, blockSamples));
            const auto velocity = juce::jlimit(0.20f, 1.0f,
                0.28f + chaos * 0.48f + chaosNoteRandom.nextFloat() * (0.16f + chaos * 0.16f));

            ChaosGeneratedNote* slot = nullptr;
            for (auto& generated : chaosGeneratedNotes)
                if (! generated.active) { slot = &generated; break; }

            // No free stochastic slot: a new note must defeat an old one. This is
            // the audible takeover mechanism at the top of the CHAOS range.
            if (slot == nullptr)
            {
                slot = &chaosGeneratedNotes.front();
                for (auto& generated : chaosGeneratedNotes)
                    if (generated.age < slot->age)
                        slot = &generated;
                queueOff(slot->midiNote, samplePosition);
            }

            const auto minMs = juce::jmap(chaos, 190.0f, 38.0f);
            const auto maxMs = juce::jmap(chaos, 1050.0f, 310.0f);
            const auto durationMs = minMs + chaosNoteRandom.nextFloat() * (maxMs - minMs);
            const auto durationSamples = juce::jmax<std::int64_t>(
                1, (std::int64_t) std::llround(durationMs * 0.001 * currentSampleRate));

            *slot = { note, durationSamples, ++chaosGeneratedCounter, true };
            if (pendingCount < (int) pending.size())
                pending[(std::size_t) pendingCount++] = { note, samplePosition, velocity, true };
        }

        for (int i = 0; i < pendingCount; ++i)
        {
            const auto& e = pending[(std::size_t) i];
            if (e.noteOn)
                midi.addEvent(juce::MidiMessage::noteOn(1, e.midiNote, e.velocity), e.samplePosition);
            else
                midi.addEvent(juce::MidiMessage::noteOff(1, e.midiNote), e.samplePosition);
        }
    }

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

    std::atomic<int> midiNoteOnCount { 0 };
    std::atomic<int> midiNoteOffCount { 0 };
    std::atomic<int> midiNoteOffMatchCount { 0 };

    std::array<std::atomic<int>, midiLearnParameterCount> midiCCMappings {};
    std::atomic<int> midiLearnTarget { -1 };

    // Shared musical CHAOS state. None of this depends on the oscillator operating
    // model, which is what keeps CHAOS behaviour identical in both engines.
    std::array<bool, 128> chaosHeldNotes {};
    std::array<ChaosGeneratedNote, 8> chaosGeneratedNotes {};
    juce::Random chaosNoteRandom { 0x4348414f };
    double chaosSpawnAccumulator { 0.0 };
    int chaosLastGeneratedNote { 60 };
    std::uint64_t chaosGeneratedCounter { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VeloriaAudioProcessor)
};

// PluginProcessor.cpp currently owns the legacy audio renderer. When this header
// is included directly by that translation unit, rename that existing definition
// to processBlockLegacy so the wrapper above can insert the shared note-event layer.
// Nested includes (for example PluginEditor.cpp -> PluginEditor.h -> this header)
// are deliberately left untouched.
#if defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 1
 #define processBlock processBlockLegacy
#endif
