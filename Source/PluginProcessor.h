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

    // v1.1 render path: the stochastic oscillator and musical CHAOS layer are
    // unchanged, but MIDI is now consumed at its exact sample position and the
    // voices are rendered into a deterministic stereo field instead of copying a
    // mono mix to both channels.
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override
    {
        juce::ScopedNoDenormals noDenormals;
        processChaosMidi(midi, buffer.getNumSamples());
        buffer.clear();

        updateVoiceParameters();
        outputGain.setRampDurationSeconds(0.020);
        outputGain.setGainDecibels(parameters.getRawParameterValue("level")->load());

        juce::MidiBuffer::Iterator iterator(midi);
        juce::MidiMessage nextMessage;
        int nextSample = 0;
        bool hasEvent = iterator.getNextEvent(nextMessage, nextSample);

        const auto voiceGain = 0.58f / std::sqrt(static_cast<float>(maxVoices));
        double energyAccumulator = 0.0;

        auto releaseDeferredNotes = [this]
        {
            for (int note = 0; note < 128; ++note)
                if (! performanceHeldNotes[(std::size_t) note])
                    stopNote(note);
        };

        auto handleEvent = [this, &releaseDeferredNotes](const juce::MidiMessage& message)
        {
            const bool generatedChaosEvent = message.getChannel() == chaosMidiChannel;

            if (message.isController())
            {
                handleMidiController(message);

                if (message.getControllerNumber() == 64 && ! generatedChaosEvent)
                {
                    const bool wasDown = sustainPedalDown;
                    sustainPedalDown = message.getControllerValue() >= 64;
                    if (wasDown && ! sustainPedalDown)
                        releaseDeferredNotes();
                }
                else if (message.getControllerNumber() == 1 && ! generatedChaosEvent)
                {
                    modWheel = juce::jlimit(0.0f, 1.0f,
                                           static_cast<float>(message.getControllerValue()) / 127.0f);
                }
            }
            else if (message.isPitchWheel())
            {
                const auto normalised = (static_cast<float>(message.getPitchWheelValue()) - 8192.0f) / 8192.0f;
                pitchBendSemitones = juce::jlimit(-2.0f, 2.0f, normalised * 2.0f);
            }
            else if (message.isAftertouch())
            {
                const auto pressure = juce::jlimit(0.0f, 1.0f,
                    static_cast<float>(message.getAfterTouchValue()) / 127.0f);
                for (auto& voice : voices)
                    if (voice.active && ! voice.percussion && voice.midiNote == message.getNoteNumber())
                        voice.pressure = pressure;
            }
            else if (message.isChannelPressure())
            {
                channelPressure = juce::jlimit(0.0f, 1.0f,
                    static_cast<float>(message.getChannelPressureValue()) / 127.0f);
                for (auto& voice : voices)
                    if (voice.active && ! voice.percussion)
                        voice.pressure = channelPressure;
            }

            if (message.isNoteOn(false))
            {
                const auto note = message.getNoteNumber();
                if (! generatedChaosEvent && juce::isPositiveAndBelow(note, 128))
                    performanceHeldNotes[(std::size_t) note] = true;
                midiNoteOnCount.fetch_add(1, std::memory_order_relaxed);
                startNote(note, message.getFloatVelocity());
            }
            else if (message.isNoteOff(true))
            {
                const auto note = message.getNoteNumber();
                if (! generatedChaosEvent && juce::isPositiveAndBelow(note, 128))
                    performanceHeldNotes[(std::size_t) note] = false;

                midiNoteOffCount.fetch_add(1, std::memory_order_relaxed);

                bool matched = false;
                for (const auto& voice : voices)
                    matched = matched || (voice.active && ! voice.percussion && voice.midiNote == note);
                if (matched)
                    midiNoteOffMatchCount.fetch_add(1, std::memory_order_relaxed);

                if (generatedChaosEvent || ! sustainPedalDown)
                    stopNote(note);
            }
            else if (message.isAllNotesOff() || message.isAllSoundOff())
            {
                performanceHeldNotes.fill(false);
                sustainPedalDown = false;
                stopAllVoices(false);
            }

            // Pressure and learned-controller changes affect the very next sample.
            updateVoiceParameters();
        };

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            while (hasEvent && nextSample <= sample)
            {
                handleEvent(nextMessage);
                hasEvent = iterator.getNextEvent(nextMessage, nextSample);
            }

            float left = 0.0f;
            float right = 0.0f;

            for (std::size_t voiceIndex = 0; voiceIndex < voices.size(); ++voiceIndex)
            {
                auto& voice = voices[voiceIndex];
                if (! voice.active)
                    continue;

                float voiceSample = 0.0f;

                if (voice.percussion)
                {
                    const auto length = juce::jmax<std::uint64_t>(1, voice.percussionLengthSamples);
                    const auto age = juce::jmin(voice.percussionSample, length);
                    const auto progress = juce::jlimit(0.0f, 1.0f,
                        static_cast<float>(age) / static_cast<float>(length));
                    const auto remaining = juce::jmax(0.0f, 1.0f - progress);
                    const auto contraction = std::pow(remaining, voice.contractionPower);
                    const auto ampWalkNow = voice.endAmpWalk + (voice.startAmpWalk - voice.endAmpWalk) * contraction;
                    const auto timeWalkNow = voice.endTimeWalk + (voice.startTimeWalk - voice.endTimeWalk) * contraction;
                    voice.oscillator.setAmplitudeWalk(ampWalkNow);
                    voice.oscillator.setTimeWalk(timeWalkNow);
                    voice.oscillator.setAmplitudeMirror(voice.amplitudeMirror);
                    voice.oscillator.setTimeMirror(voice.timeMirror);

                    const auto pitchShape = std::pow(remaining, voice.pitchPower);
                    const auto frequency = voice.endFrequency + (voice.startFrequency - voice.endFrequency) * pitchShape;
                    voice.oscillator.setFrequency(frequency);

                    float env = 0.0f;
                    const auto attackSamples = juce::jmax<std::uint64_t>(1, voice.percussionAttackSamples);
                    if (age < attackSamples)
                        env = static_cast<float>(age) / static_cast<float>(attackSamples);
                    else
                    {
                        const auto decayLength = juce::jmax<std::uint64_t>(1, length - attackSamples);
                        const auto decayAge = juce::jmin(age - attackSamples, decayLength);
                        const auto decayProgress = static_cast<float>(decayAge) / static_cast<float>(decayLength);
                        env = std::pow(juce::jmax(0.0f, 1.0f - decayProgress), voice.decayPower);
                    }

                    voiceSample = voice.oscillator.processSample() * env * voice.gain * voiceGain;
                    ++voice.percussionSample;
                    if (voice.percussionSample >= length)
                    {
                        voice.active = false;
                        voice.held = false;
                        voice.percussion = false;
                        voice.drumKind = DrumKind::none;
                        voice.midiNote = -1;
                        voice.pressure = 0.0f;
                    }
                }
                else
                {
                    if (voice.midiNote >= 0)
                    {
                        const auto bendRatio = std::pow(2.0f, pitchBendSemitones / 12.0f);
                        const auto baseFrequency = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(voice.midiNote));
                        voice.oscillator.setFrequency(baseFrequency * bendRatio);
                    }

                    const auto env = voice.envelope.getNextSample();
                    const auto expressionGain = 1.0f + modWheel * 0.10f + channelPressure * 0.06f;
                    voiceSample = voice.oscillator.processSample() * env * voice.gain * voiceGain * expressionGain;

                    if (! voice.envelope.isActive())
                    {
                        voice.active = false;
                        voice.held = false;
                        voice.midiNote = -1;
                        voice.pressure = 0.0f;
                    }
                }

                if (! std::isfinite(voiceSample))
                    voiceSample = 0.0f;
                voiceSample = juce::jlimit(-4.0f, 4.0f, voiceSample);

                // Stable per-voice equal-power position. The MIDI note adds a small
                // deterministic offset so repeated chords do not collapse into the
                // same left/right pattern after voice stealing.
                const auto noteOffset = voice.midiNote >= 0 ? (voice.midiNote % 12) / 11.0f : 0.5f;
                const auto indexPosition = voices.size() > 1
                    ? static_cast<float>(voiceIndex) / static_cast<float>(voices.size() - 1)
                    : 0.5f;
                auto pan = ((indexPosition * 0.72f + noteOffset * 0.28f) * 2.0f - 1.0f);
                pan *= 0.72f + modWheel * 0.16f;
                pan = juce::jlimit(-0.92f, 0.92f, pan);
                const auto angle = (pan + 1.0f) * juce::MathConstants<float>::quarterPi;
                left += voiceSample * std::cos(angle);
                right += voiceSample * std::sin(angle);
            }

            if (! std::isfinite(left)) left = 0.0f;
            if (! std::isfinite(right)) right = 0.0f;
            left = juce::jlimit(-4.0f, 4.0f, left);
            right = juce::jlimit(-4.0f, 4.0f, right);

            energyAccumulator += 0.5 * (static_cast<double>(left) * static_cast<double>(left)
                                      + static_cast<double>(right) * static_cast<double>(right));

            if (buffer.getNumChannels() == 1)
            {
                buffer.setSample(0, sample, 0.70710678f * (left + right));
            }
            else
            {
                buffer.setSample(0, sample, left);
                buffer.setSample(1, sample, right);
                for (int channel = 2; channel < buffer.getNumChannels(); ++channel)
                    buffer.setSample(channel, sample, 0.0f);
            }
        }

        outputGain.process(juce::dsp::ProcessContextReplacing<float>(juce::dsp::AudioBlock<float>(buffer)));

        const auto rms = buffer.getNumSamples() > 0
            ? static_cast<float>(std::sqrt(energyAccumulator / static_cast<double>(buffer.getNumSamples())))
            : 0.0f;
        publishVisualState(juce::jlimit(0.0f, 1.0f, rms * 5.0f));
    }

    // Kept during the v1.1 transition so the old renderer remains available for
    // regression comparison while the new path is validated.
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
    static constexpr int chaosMidiChannel = 16;
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

        // Inspect only host/player MIDI before adding autonomous events. Generated
        // notes use a private MIDI channel so sustain handling cannot accidentally
        // hold the stochastic takeover voices forever.
        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();
            if (message.isNoteOn(false) && message.getChannel() != chaosMidiChannel)
            {
                const auto note = message.getNoteNumber();
                if (juce::isPositiveAndBelow(note, 128))
                    chaosHeldNotes[(std::size_t) note] = true;
            }
            else if (message.isNoteOff(true) && message.getChannel() != chaosMidiChannel)
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
                midi.addEvent(juce::MidiMessage::noteOff(chaosMidiChannel, e.midiNote), e.samplePosition);
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
                midi.addEvent(juce::MidiMessage::noteOff(chaosMidiChannel, e.midiNote), e.samplePosition);
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
                midi.addEvent(juce::MidiMessage::noteOn(chaosMidiChannel, e.midiNote, e.velocity), e.samplePosition);
            else
                midi.addEvent(juce::MidiMessage::noteOff(chaosMidiChannel, e.midiNote), e.samplePosition);
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

    // v1.1 performance state.
    std::array<bool, 128> performanceHeldNotes {};
    bool sustainPedalDown { false };
    float pitchBendSemitones { 0.0f };
    float modWheel { 0.0f };
    float channelPressure { 0.0f };

    // Shared musical CHAOS state. None of this depends on the oscillator operating
    // model, which keeps CHAOS behaviour identical in both stochastic engines.
    std::array<bool, 128> chaosHeldNotes {};
    std::array<ChaosGeneratedNote, 8> chaosGeneratedNotes {};
    juce::Random chaosNoteRandom { 0x4348414f };
    double chaosSpawnAccumulator { 0.0 };
    int chaosLastGeneratedNote { 60 };
    std::uint64_t chaosGeneratedCounter { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VeloriaAudioProcessor)
};

// PluginProcessor.cpp still contains the original renderer during the v1.1
// transition. Rename that definition to processBlockLegacy when compiling the
// processor translation unit so the public v1.1 renderer above owns processBlock.
#if defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 1
 #define processBlock processBlockLegacy
#endif
