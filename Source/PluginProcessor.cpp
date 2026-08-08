#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
using Distribution = veloria::dsp::StochasticOscillator::Distribution;

Distribution distributionFromParameter(float value) noexcept
{
    switch (juce::jlimit(0, 5, static_cast<int>(std::lround(value))))
    {
        case 1: return Distribution::uniform;
        case 2: return Distribution::gaussian;
        case 3: return Distribution::logistic;
        case 4: return Distribution::cauchy;
        case 5: return Distribution::arcsine;
        case 0:
        default: return Distribution::adaptive;
    }
}
}

const std::array<VeloriaAudioProcessor::FactoryPreset, 10> VeloriaAudioProcessor::factoryPresets {{
    { "GENDYN Core", 0.12f, 0.08f, 0.88f, 0.45f, 0.08f, 0.45f, 0.88f, 1.40f, 2207 },
    { "Piano",       0.05f, 0.025f,0.72f, 0.20f, 0.003f,0.85f, 0.10f, 0.55f, 3101 },
    { "Pad",         0.16f, 0.10f, 0.92f, 0.62f, 1.20f, 1.70f, 0.84f, 4.20f, 4201 },
    { "Strings",     0.10f, 0.07f, 0.82f, 0.48f, 0.35f, 0.95f, 0.76f, 1.80f, 5303 },
    { "Bass",        0.045f,0.02f, 0.68f, 0.16f, 0.008f,0.22f, 0.70f, 0.30f, 6407 },
    { "Lead",        0.08f, 0.04f, 0.78f, 0.30f, 0.012f,0.30f, 0.76f, 0.45f, 7507 },
    { "Bell",        0.07f, 0.018f,0.84f, 0.22f, 0.002f,1.45f, 0.02f, 1.10f, 8609 },
    { "Pluck",       0.10f, 0.04f, 0.76f, 0.26f, 0.002f,0.32f, 0.04f, 0.25f, 9719 },
    { "FX",          0.55f, 0.50f, 1.00f, 0.95f, 0.04f, 1.00f, 0.60f, 3.20f,10831 },
    { "Drums",       0.62f, 0.52f, 0.96f, 0.86f, 0.001f,0.20f, 0.00f, 0.05f,11939 }
}};

const std::array<const char*, VeloriaAudioProcessor::midiLearnParameterCount>
VeloriaAudioProcessor::midiLearnParameterIds {{
    "ampWalk", "timeWalk", "ampMirror", "timeMirror",
    "ampDist", "timeDist", "ampStep", "timeStep",
    "walkOrder", "breakpoints", "pitchStability", "curve",
    "attack", "decay", "sustain", "release", "seed", "level"
}};

VeloriaAudioProcessor::VeloriaAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    for (auto& value : visualDurations)
        value.store(1.0f);
    for (auto& mapping : midiCCMappings)
        mapping.store(-1);
}

juce::AudioProcessorValueTreeState::ParameterLayout VeloriaAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> layout;
    layout.push_back(std::make_unique<juce::AudioParameterFloat>("ampWalk", "Amplitude Walk", juce::NormalisableRange<float>(0.0f, 1.0f), 0.12f));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>("timeWalk", "Time Walk", juce::NormalisableRange<float>(0.0f, 1.0f), 0.08f));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>("ampMirror", "Amplitude Barrier", juce::NormalisableRange<float>(0.05f, 1.0f), 0.88f));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>("timeMirror", "Time Barrier", juce::NormalisableRange<float>(0.05f, 1.0f), 0.45f));

    layout.push_back(std::make_unique<juce::AudioParameterInt>("ampDist", "Amplitude Distribution", 0, 5, 0));
    layout.push_back(std::make_unique<juce::AudioParameterInt>("timeDist", "Time Distribution", 0, 5, 0));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>("ampStep", "Amplitude Step", juce::NormalisableRange<float>(0.10f, 2.0f), 1.0f));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>("timeStep", "Time Step", juce::NormalisableRange<float>(0.10f, 2.0f), 1.0f));
    layout.push_back(std::make_unique<juce::AudioParameterInt>("walkOrder", "Walk Order", 1, 2, 2));
    layout.push_back(std::make_unique<juce::AudioParameterInt>("breakpoints", "Breakpoints", 4, 12, 12));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>("pitchStability", "Pitch Stability", juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>("curve", "Interpolation Curve", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    layout.push_back(std::make_unique<juce::AudioParameterFloat>("attack", "Attack", juce::NormalisableRange<float>(0.001f, 5.0f, 0.0f, 0.35f), 0.08f));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>("decay", "Decay", juce::NormalisableRange<float>(0.01f, 6.0f, 0.0f, 0.35f), 0.45f));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>("sustain", "Sustain", juce::NormalisableRange<float>(0.0f, 1.0f), 0.88f));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>("release", "Release", juce::NormalisableRange<float>(0.01f, 8.0f, 0.0f, 0.35f), 1.40f));
    layout.push_back(std::make_unique<juce::AudioParameterBool>("mono", "Mono", false));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>("level", "Level", juce::NormalisableRange<float>(-48.0f, 0.0f), -12.0f));
    layout.push_back(std::make_unique<juce::AudioParameterInt>("seed", "Seed", 1, 999999, 2207));
    return { layout.begin(), layout.end() };
}

void VeloriaAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = juce::jmax(1.0, sampleRate);
    for (auto& voice : voices)
    {
        voice.oscillator.prepare(currentSampleRate);
        voice.envelope.setSampleRate(currentSampleRate);
        voice.active = false;
        voice.percussion = false;
        voice.drumKind = DrumKind::none;
        voice.midiNote = -1;
    }
    outputGain.prepare({ currentSampleRate, static_cast<juce::uint32>(samplesPerBlock), 2 });
    updateVoiceParameters();
    publishVisualState(0.0f);
}

void VeloriaAudioProcessor::releaseResources() {}

bool VeloriaAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    return output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo();
}

void VeloriaAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    updateVoiceParameters();

    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();
        if (message.isController())
            handleMidiController(message);
        if (message.isNoteOn())
            startNote(message.getNoteNumber(), message.getFloatVelocity());
        else if (message.isNoteOff())
            stopNote(message.getNoteNumber());
        else if (message.isAllNotesOff() || message.isAllSoundOff())
            stopAllVoices(false);
    }

    const auto voiceGain = 0.58f / std::sqrt(static_cast<float>(maxVoices));
    double energyAccumulator = 0.0;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float mixed = 0.0f;
        for (auto& voice : voices)
        {
            if (! voice.active)
                continue;

            if (voice.percussion)
            {
                const auto length = juce::jmax<std::uint64_t>(1, voice.percussionLengthSamples);
                const auto age = juce::jmin(voice.percussionSample, length);
                const auto progress = juce::jlimit(0.0f, 1.0f, static_cast<float>(age) / static_cast<float>(length));
                const auto remaining = juce::jmax(0.0f, 1.0f - progress);
                const auto contraction = std::pow(remaining, voice.contractionPower);
                const auto ampWalk = voice.endAmpWalk + (voice.startAmpWalk - voice.endAmpWalk) * contraction;
                const auto timeWalk = voice.endTimeWalk + (voice.startTimeWalk - voice.endTimeWalk) * contraction;
                voice.oscillator.setAmplitudeWalk(ampWalk);
                voice.oscillator.setTimeWalk(timeWalk);
                voice.oscillator.setAmplitudeMirror(voice.amplitudeMirror);
                voice.oscillator.setTimeMirror(voice.timeMirror);

                const auto pitchShape = std::pow(remaining, voice.pitchPower);
                const auto frequency = voice.endFrequency + (voice.startFrequency - voice.endFrequency) * pitchShape;
                voice.oscillator.setFrequency(frequency);

                float env = 0.0f;
                const auto attack = juce::jmax<std::uint64_t>(1, voice.percussionAttackSamples);
                if (age < attack)
                    env = static_cast<float>(age) / static_cast<float>(attack);
                else
                {
                    const auto decayLength = juce::jmax<std::uint64_t>(1, length - attack);
                    const auto decayAge = juce::jmin(age - attack, decayLength);
                    const auto decayProgress = static_cast<float>(decayAge) / static_cast<float>(decayLength);
                    env = std::pow(juce::jmax(0.0f, 1.0f - decayProgress), voice.decayPower);
                }

                mixed += voice.oscillator.processSample() * env * voice.gain * voiceGain;
                ++voice.percussionSample;
                if (voice.percussionSample >= length)
                {
                    voice.active = false;
                    voice.percussion = false;
                    voice.drumKind = DrumKind::none;
                    voice.midiNote = -1;
                }
                continue;
            }

            const auto env = voice.envelope.getNextSample();
            mixed += voice.oscillator.processSample() * env * voice.gain * voiceGain;
            if (! voice.envelope.isActive())
            {
                voice.active = false;
                voice.midiNote = -1;
            }
        }

        energyAccumulator += static_cast<double>(mixed) * static_cast<double>(mixed);
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.setSample(channel, sample, mixed);
    }

    outputGain.setGainDecibels(parameters.getRawParameterValue("level")->load());
    juce::dsp::AudioBlock<float> block(buffer);
    outputGain.process(juce::dsp::ProcessContextReplacing<float>(block));
    const auto rms = buffer.getNumSamples() > 0
        ? static_cast<float>(std::sqrt(energyAccumulator / static_cast<double>(buffer.getNumSamples())))
        : 0.0f;
    publishVisualState(juce::jlimit(0.0f, 1.0f, rms * 5.0f));
}

void VeloriaAudioProcessor::startNote(int midiNote, float velocity)
{
    if (drumMode)
    {
        startDrumNote(midiNote, velocity);
        return;
    }

    const auto mono = parameters.getRawParameterValue("mono")->load() > 0.5f;
    if (mono)
        stopAllVoices(false);

    auto& voice = mono ? voices.front() : findVoiceToStart();
    voice.active = true;
    voice.percussion = false;
    voice.drumKind = DrumKind::none;
    voice.midiNote = midiNote;
    voice.age = ++voiceCounter;
    voice.gain = 0.70f + juce::jlimit(0.0f, 1.0f, velocity) * 0.30f;

    const auto baseSeed = static_cast<std::uint32_t>(parameters.getRawParameterValue("seed")->load());
    const auto voiceIndex = static_cast<std::uint32_t>(&voice - voices.data());
    const auto derivedSeed = baseSeed + voiceIndex * 101u + static_cast<std::uint32_t>(midiNote) * 17u;
    voice.oscillator.setSeed(derivedSeed == 0 ? 1u : derivedSeed);
    voice.oscillator.setFrequency(static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNote)));
    voice.envelope.reset();
    voice.envelope.noteOn();
}

void VeloriaAudioProcessor::startDrumNote(int midiNote, float velocity)
{
    auto trigger = [this, midiNote, velocity](DrumKind kind, int layerIndex = 0)
    {
        auto& voice = findVoiceToStart();
        configureDrumVoice(voice, kind, midiNote, velocity, layerIndex);
    };

    switch (midiNote)
    {
        case 35:
        case 36: trigger(DrumKind::kick); break;
        case 38:
        case 40: trigger(DrumKind::snareBody, 0); trigger(DrumKind::snareWire, 1); break;
        case 42:
        case 44: trigger(DrumKind::closedHat); break;
        case 46: trigger(DrumKind::openHat); break;
        case 49:
        case 51: trigger(DrumKind::crash); break;
        case 41: case 43: case 45: case 47: case 48: case 50: trigger(DrumKind::tom); break;
        default:
        {
            const auto pitchClass = midiNote % 12;
            if (pitchClass == 0 || pitchClass == 5) trigger(DrumKind::kick);
            else if (pitchClass == 2 || pitchClass == 4) { trigger(DrumKind::snareBody, 0); trigger(DrumKind::snareWire, 1); }
            else if (pitchClass == 6 || pitchClass == 8) trigger(DrumKind::closedHat);
            else if (pitchClass == 10) trigger(DrumKind::openHat);
            else if (pitchClass == 1 || pitchClass == 3) trigger(DrumKind::crash);
            else trigger(DrumKind::tom);
            break;
        }
    }
}

void VeloriaAudioProcessor::configureDrumVoice(Voice& voice, DrumKind kind, int midiNote, float velocity, int layerIndex)
{
    voice.active = true;
    voice.percussion = true;
    voice.drumKind = kind;
    voice.midiNote = midiNote;
    voice.age = ++voiceCounter;
    voice.percussionSample = 0;

    float durationSeconds = 0.30f, attackMs = 0.7f;
    float startAmpWalk = 0.70f, endAmpWalk = 0.02f;
    float startTimeWalk = 0.55f, endTimeWalk = 0.015f;
    float ampMirror = 0.92f, timeMirror = 0.72f;
    float startFrequency = 220.0f, endFrequency = 180.0f;
    float contractionPower = 2.5f, pitchPower = 2.0f, decayPower = 2.0f, level = 1.0f;

    switch (kind)
    {
        case DrumKind::kick:
            durationSeconds=0.52f; attackMs=0.45f; startAmpWalk=0.58f; endAmpWalk=0.008f; startTimeWalk=0.34f; endTimeWalk=0.004f;
            ampMirror=0.80f; timeMirror=0.36f; startFrequency=126.0f; endFrequency=48.0f; contractionPower=3.8f; pitchPower=4.2f; decayPower=2.6f; level=1.45f; break;
        case DrumKind::snareBody:
            durationSeconds=0.42f; attackMs=0.45f; startAmpWalk=0.66f; endAmpWalk=0.018f; startTimeWalk=0.42f; endTimeWalk=0.010f;
            ampMirror=0.90f; timeMirror=0.55f; startFrequency=285.0f; endFrequency=175.0f; contractionPower=3.0f; pitchPower=2.8f; decayPower=2.1f; level=0.95f; break;
        case DrumKind::snareWire:
            durationSeconds=0.50f; attackMs=0.25f; startAmpWalk=0.98f; endAmpWalk=0.10f; startTimeWalk=0.88f; endTimeWalk=0.065f;
            ampMirror=1.00f; timeMirror=0.96f; startFrequency=1850.0f; endFrequency=980.0f; contractionPower=2.0f; pitchPower=1.6f; decayPower=2.4f; level=0.72f; break;
        case DrumKind::closedHat:
            durationSeconds=0.115f; attackMs=0.16f; startAmpWalk=1.00f; endAmpWalk=0.14f; startTimeWalk=0.94f; endTimeWalk=0.10f;
            ampMirror=1.00f; timeMirror=0.98f; startFrequency=7600.0f; endFrequency=5200.0f; contractionPower=4.0f; pitchPower=1.4f; decayPower=3.0f; level=0.72f; break;
        case DrumKind::openHat:
            durationSeconds=0.72f; attackMs=0.18f; startAmpWalk=0.98f; endAmpWalk=0.07f; startTimeWalk=0.92f; endTimeWalk=0.045f;
            ampMirror=1.00f; timeMirror=0.98f; startFrequency=7100.0f; endFrequency=4300.0f; contractionPower=1.8f; pitchPower=1.2f; decayPower=1.65f; level=0.66f; break;
        case DrumKind::crash:
            durationSeconds=1.85f; attackMs=0.25f; startAmpWalk=1.00f; endAmpWalk=0.075f; startTimeWalk=0.98f; endTimeWalk=0.055f;
            ampMirror=1.00f; timeMirror=1.00f; startFrequency=4700.0f; endFrequency=2500.0f; contractionPower=1.35f; pitchPower=1.0f; decayPower=1.28f; level=0.68f; break;
        case DrumKind::tom:
            durationSeconds=0.46f; attackMs=0.40f; startAmpWalk=0.50f; endAmpWalk=0.012f; startTimeWalk=0.30f; endTimeWalk=0.008f;
            ampMirror=0.82f; timeMirror=0.42f; startFrequency=juce::jlimit(95.0f,310.0f,static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNote))*0.72f);
            endFrequency=startFrequency*0.62f; contractionPower=3.2f; pitchPower=3.0f; decayPower=2.3f; level=1.0f; break;
        case DrumKind::none: break;
    }

    const auto globalAmpWalk = parameters.getRawParameterValue("ampWalk")->load();
    const auto globalTimeWalk = parameters.getRawParameterValue("timeWalk")->load();
    const auto globalAmpMirror = parameters.getRawParameterValue("ampMirror")->load();
    const auto globalTimeMirror = parameters.getRawParameterValue("timeMirror")->load();
    const auto ampScale = 0.72f + globalAmpWalk * 0.52f;
    const auto timeScale = 0.72f + globalTimeWalk * 0.52f;
    const auto mirrorScale = 0.88f + globalAmpMirror * 0.12f;
    const auto timeMirrorScale = 0.88f + globalTimeMirror * 0.12f;

    voice.startAmpWalk = juce::jlimit(0.0f,1.0f,startAmpWalk*ampScale);
    voice.endAmpWalk = juce::jlimit(0.0f,1.0f,endAmpWalk*ampScale);
    voice.startTimeWalk = juce::jlimit(0.0f,1.0f,startTimeWalk*timeScale);
    voice.endTimeWalk = juce::jlimit(0.0f,1.0f,endTimeWalk*timeScale);
    voice.amplitudeMirror = juce::jlimit(0.05f,1.0f,ampMirror*mirrorScale);
    voice.timeMirror = juce::jlimit(0.05f,1.0f,timeMirror*timeMirrorScale);
    voice.startFrequency = startFrequency;
    voice.endFrequency = endFrequency;
    voice.contractionPower = contractionPower;
    voice.pitchPower = pitchPower;
    voice.decayPower = decayPower;
    voice.gain = level * (0.30f + juce::jlimit(0.0f,1.0f,velocity)*0.70f);
    voice.percussionLengthSamples = juce::jmax<std::uint64_t>(1, static_cast<std::uint64_t>(durationSeconds*currentSampleRate));
    voice.percussionAttackSamples = juce::jmax<std::uint64_t>(1, static_cast<std::uint64_t>((attackMs*0.001f)*currentSampleRate));

    const auto baseSeed = static_cast<std::uint32_t>(parameters.getRawParameterValue("seed")->load());
    const auto voiceIndex = static_cast<std::uint32_t>(&voice - voices.data());
    const auto kindValue = static_cast<std::uint32_t>(kind);
    const auto derivedSeed = baseSeed + static_cast<std::uint32_t>(midiNote)*17u + voiceIndex*101u + kindValue*7919u + static_cast<std::uint32_t>(layerIndex)*3571u;
    voice.oscillator.setSeed(derivedSeed == 0 ? 1u : derivedSeed);
    voice.oscillator.setFrequency(voice.startFrequency);
    voice.oscillator.setAmplitudeWalk(voice.startAmpWalk);
    voice.oscillator.setTimeWalk(voice.startTimeWalk);
    voice.oscillator.setAmplitudeMirror(voice.amplitudeMirror);
    voice.oscillator.setTimeMirror(voice.timeMirror);
    voice.envelope.reset();
}

void VeloriaAudioProcessor::stopNote(int midiNote)
{
    if (drumMode)
        return;
    for (auto& voice : voices)
        if (voice.active && voice.midiNote == midiNote)
            voice.envelope.noteOff();
}

void VeloriaAudioProcessor::stopAllVoices(bool allowTailOff)
{
    for (auto& voice : voices)
    {
        if (allowTailOff && ! voice.percussion)
            voice.envelope.noteOff();
        else
        {
            voice.envelope.reset();
            voice.active = false;
            voice.percussion = false;
            voice.drumKind = DrumKind::none;
            voice.midiNote = -1;
            voice.percussionSample = 0;
        }
    }
}

VeloriaAudioProcessor::Voice& VeloriaAudioProcessor::findVoiceToStart()
{
    for (auto& voice : voices)
        if (! voice.active)
            return voice;
    auto* oldest = &voices.front();
    for (auto& voice : voices)
        if (voice.age < oldest->age)
            oldest = &voice;
    oldest->envelope.reset();
    oldest->active = false;
    oldest->percussion = false;
    oldest->drumKind = DrumKind::none;
    return *oldest;
}

void VeloriaAudioProcessor::updateVoiceParameters()
{
    const auto ampWalk = parameters.getRawParameterValue("ampWalk")->load();
    const auto timeWalk = parameters.getRawParameterValue("timeWalk")->load();
    const auto ampMirror = parameters.getRawParameterValue("ampMirror")->load();
    const auto timeMirror = parameters.getRawParameterValue("timeMirror")->load();
    const auto ampDist = distributionFromParameter(parameters.getRawParameterValue("ampDist")->load());
    const auto timeDist = distributionFromParameter(parameters.getRawParameterValue("timeDist")->load());
    const auto ampStep = parameters.getRawParameterValue("ampStep")->load();
    const auto timeStep = parameters.getRawParameterValue("timeStep")->load();
    const auto walkOrder = static_cast<int>(std::lround(parameters.getRawParameterValue("walkOrder")->load()));
    const auto breakpoints = static_cast<int>(std::lround(parameters.getRawParameterValue("breakpoints")->load()));
    const auto pitchStability = parameters.getRawParameterValue("pitchStability")->load();
    const auto curve = parameters.getRawParameterValue("curve")->load();

    juce::ADSR::Parameters env;
    env.attack = parameters.getRawParameterValue("attack")->load();
    env.decay = parameters.getRawParameterValue("decay")->load();
    env.sustain = parameters.getRawParameterValue("sustain")->load();
    env.release = parameters.getRawParameterValue("release")->load();

    for (auto& voice : voices)
    {
        voice.oscillator.setAmplitudeDistribution(ampDist);
        voice.oscillator.setTimeDistribution(timeDist);
        voice.oscillator.setAmplitudeStepScale(ampStep);
        voice.oscillator.setTimeStepScale(timeStep);
        voice.oscillator.setWalkOrder(walkOrder);
        voice.oscillator.setActiveBreakpointCount(breakpoints);
        voice.oscillator.setPitchStability(pitchStability);
        voice.oscillator.setInterpolationShape(curve);

        if (voice.percussion)
            continue;
        voice.oscillator.setAmplitudeWalk(ampWalk);
        voice.oscillator.setTimeWalk(timeWalk);
        voice.oscillator.setAmplitudeMirror(ampMirror);
        voice.oscillator.setTimeMirror(timeMirror);
        voice.envelope.setParameters(env);
    }
}

void VeloriaAudioProcessor::publishVisualState(float energy) noexcept
{
    const Voice* visualVoice = &voices.front();
    int activeCount = 0;
    for (const auto& voice : voices)
    {
        if (voice.active)
        {
            ++activeCount;
            if (visualVoice == &voices.front() && ! voices.front().active)
                visualVoice = &voice;
        }
    }
    std::array<float, visualBreakpointCount> amplitudes {};
    std::array<float, visualBreakpointCount> durations {};
    visualVoice->oscillator.copyState(amplitudes, durations);
    for (std::size_t i = 0; i < visualBreakpointCount; ++i)
    {
        visualAmplitudes[i].store(amplitudes[i], std::memory_order_relaxed);
        visualDurations[i].store(durations[i], std::memory_order_relaxed);
    }
    visualActiveVoices.store(activeCount, std::memory_order_relaxed);
    visualEnergy.store(energy, std::memory_order_relaxed);
}

VeloriaAudioProcessor::VisualState VeloriaAudioProcessor::getVisualState() const noexcept
{
    VisualState state;
    for (std::size_t i = 0; i < visualBreakpointCount; ++i)
    {
        state.amplitudes[i] = visualAmplitudes[i].load(std::memory_order_relaxed);
        state.durations[i] = visualDurations[i].load(std::memory_order_relaxed);
    }
    state.activeVoices = visualActiveVoices.load(std::memory_order_relaxed);
    state.energy = visualEnergy.load(std::memory_order_relaxed);
    return state;
}

int VeloriaAudioProcessor::getNumPrograms() { return static_cast<int>(factoryPresets.size()); }

const juce::String VeloriaAudioProcessor::getProgramName(int index)
{
    return juce::isPositiveAndBelow(index, getNumPrograms()) ? juce::String(factoryPresets[static_cast<std::size_t>(index)].name) : juce::String();
}

juce::StringArray VeloriaAudioProcessor::getFactoryPresetNames() const
{
    juce::StringArray names;
    for (const auto& preset : factoryPresets) names.add(preset.name);
    return names;
}

void VeloriaAudioProcessor::setCurrentProgram(int index)
{
    if (! juce::isPositiveAndBelow(index, getNumPrograms())) return;
    currentProgram = index;
    applyFactoryPreset(index);
}

void VeloriaAudioProcessor::applyFactoryPreset(int index)
{
    const auto& p = factoryPresets[static_cast<std::size_t>(index)];
    drumMode = (index == drumPresetIndex);
    setParameterValue("ampWalk", p.ampWalk); setParameterValue("timeWalk", p.timeWalk);
    setParameterValue("ampMirror", p.ampMirror); setParameterValue("timeMirror", p.timeMirror);
    setParameterValue("attack", p.attack); setParameterValue("decay", p.decay);
    setParameterValue("sustain", p.sustain); setParameterValue("release", p.release);
    setParameterValue("seed", static_cast<float>(p.seed));

    // Mk-I species defaults. These remain fully user-editable and discoverable.
    switch (index)
    {
        case 1: setParameterValue("ampDist",4); setParameterValue("timeDist",2); setParameterValue("ampStep",1.25f); setParameterValue("timeStep",0.65f); setParameterValue("breakpoints",10); setParameterValue("pitchStability",1.0f); setParameterValue("curve",0.35f); break;
        case 2: setParameterValue("ampDist",2); setParameterValue("timeDist",1); setParameterValue("ampStep",0.55f); setParameterValue("timeStep",0.72f); setParameterValue("breakpoints",12); setParameterValue("pitchStability",0.96f); setParameterValue("curve",0.72f); break;
        case 3: setParameterValue("ampDist",3); setParameterValue("timeDist",2); setParameterValue("ampStep",0.72f); setParameterValue("timeStep",0.68f); setParameterValue("breakpoints",12); setParameterValue("pitchStability",0.98f); setParameterValue("curve",0.55f); break;
        case 4: setParameterValue("ampDist",2); setParameterValue("timeDist",2); setParameterValue("ampStep",0.42f); setParameterValue("timeStep",0.38f); setParameterValue("breakpoints",8); setParameterValue("pitchStability",1.0f); setParameterValue("curve",0.18f); break;
        case 5: setParameterValue("ampDist",3); setParameterValue("timeDist",2); setParameterValue("ampStep",0.78f); setParameterValue("timeStep",0.62f); setParameterValue("breakpoints",9); setParameterValue("pitchStability",0.99f); setParameterValue("curve",0.28f); break;
        case 6: setParameterValue("ampDist",4); setParameterValue("timeDist",3); setParameterValue("ampStep",1.15f); setParameterValue("timeStep",0.88f); setParameterValue("breakpoints",6); setParameterValue("pitchStability",0.94f); setParameterValue("curve",0.20f); break;
        case 7: setParameterValue("ampDist",1); setParameterValue("timeDist",2); setParameterValue("ampStep",1.30f); setParameterValue("timeStep",0.90f); setParameterValue("breakpoints",8); setParameterValue("pitchStability",1.0f); setParameterValue("curve",0.46f); break;
        case 8: setParameterValue("ampDist",5); setParameterValue("timeDist",4); setParameterValue("ampStep",1.55f); setParameterValue("timeStep",1.55f); setParameterValue("breakpoints",12); setParameterValue("pitchStability",0.55f); setParameterValue("curve",0.10f); break;
        case 9: setParameterValue("ampDist",4); setParameterValue("timeDist",4); setParameterValue("ampStep",1.45f); setParameterValue("timeStep",1.35f); setParameterValue("breakpoints",7); setParameterValue("pitchStability",0.90f); setParameterValue("curve",0.12f); break;
        case 0:
        default: setParameterValue("ampDist",0); setParameterValue("timeDist",0); setParameterValue("ampStep",1.0f); setParameterValue("timeStep",1.0f); setParameterValue("breakpoints",12); setParameterValue("pitchStability",1.0f); setParameterValue("curve",0.0f); break;
    }
    setParameterValue("walkOrder", 2.0f);
    stopAllVoices(false);
}

void VeloriaAudioProcessor::discover()
{
    if (drumMode) { discoverDrumField(); return; }
    currentProgram = -1;
    setParameterValue("ampWalk",0.01f+discoveryRandom.nextFloat()*0.60f);
    setParameterValue("timeWalk",0.005f+discoveryRandom.nextFloat()*0.48f);
    setParameterValue("ampMirror",0.25f+discoveryRandom.nextFloat()*0.75f);
    setParameterValue("timeMirror",0.08f+discoveryRandom.nextFloat()*0.82f);
    setParameterValue("ampDist",static_cast<float>(discoveryRandom.nextInt(6)));
    setParameterValue("timeDist",static_cast<float>(discoveryRandom.nextInt(6)));
    setParameterValue("ampStep",0.35f+discoveryRandom.nextFloat()*1.45f);
    setParameterValue("timeStep",0.35f+discoveryRandom.nextFloat()*1.45f);
    setParameterValue("walkOrder",discoveryRandom.nextBool()?1.0f:2.0f);
    setParameterValue("breakpoints",static_cast<float>(4+discoveryRandom.nextInt(9)));
    setParameterValue("pitchStability",0.55f+discoveryRandom.nextFloat()*0.45f);
    setParameterValue("curve",discoveryRandom.nextFloat());
    setParameterValue("seed",static_cast<float>(1+discoveryRandom.nextInt(999998)));
    stopAllVoices(false);
}

void VeloriaAudioProcessor::discoverDrumField()
{
    currentProgram=-1; drumMode=true;
    setParameterValue("ampWalk",0.48f+discoveryRandom.nextFloat()*0.28f);
    setParameterValue("timeWalk",0.38f+discoveryRandom.nextFloat()*0.27f);
    setParameterValue("ampMirror",0.82f+discoveryRandom.nextFloat()*0.18f);
    setParameterValue("timeMirror",0.62f+discoveryRandom.nextFloat()*0.34f);
    setParameterValue("ampDist",3.0f+static_cast<float>(discoveryRandom.nextInt(2)));
    setParameterValue("timeDist",3.0f+static_cast<float>(discoveryRandom.nextInt(2)));
    setParameterValue("ampStep",1.10f+discoveryRandom.nextFloat()*0.75f);
    setParameterValue("timeStep",1.05f+discoveryRandom.nextFloat()*0.75f);
    setParameterValue("walkOrder",2.0f);
    setParameterValue("breakpoints",static_cast<float>(6+discoveryRandom.nextInt(4)));
    setParameterValue("pitchStability",0.82f+discoveryRandom.nextFloat()*0.18f);
    setParameterValue("curve",discoveryRandom.nextFloat()*0.35f);
    setParameterValue("attack",0.001f); setParameterValue("decay",0.12f+discoveryRandom.nextFloat()*0.18f);
    setParameterValue("sustain",0.0f); setParameterValue("release",0.03f+discoveryRandom.nextFloat()*0.08f);
    setParameterValue("seed",static_cast<float>(1+discoveryRandom.nextInt(999998)));
    stopAllVoices(false);
}

void VeloriaAudioProcessor::newField()
{
    currentProgram=-1;
    setParameterValue("seed",static_cast<float>(1+discoveryRandom.nextInt(999998)));
    stopAllVoices(false);
}

void VeloriaAudioProcessor::setParameterValue(const juce::String& id, float value)
{
    if (auto* parameter=parameters.getParameter(id))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        parameter->endChangeGesture();
    }
}

int VeloriaAudioProcessor::findMidiParameterIndex(const juce::String& id) const noexcept
{
    for (int i=0;i<midiLearnParameterCount;++i)
        if (id==midiLearnParameterIds[static_cast<std::size_t>(i)]) return i;
    return -1;
}

void VeloriaAudioProcessor::beginMidiLearn(const juce::String& parameterId) noexcept { midiLearnTarget.store(findMidiParameterIndex(parameterId),std::memory_order_relaxed); }
void VeloriaAudioProcessor::clearMidiMapping(const juce::String& parameterId) noexcept { const auto i=findMidiParameterIndex(parameterId); if(i>=0)midiCCMappings[static_cast<std::size_t>(i)].store(-1,std::memory_order_relaxed); }
int VeloriaAudioProcessor::getMidiCCForParameter(const juce::String& parameterId) const noexcept { const auto i=findMidiParameterIndex(parameterId); return i>=0?midiCCMappings[static_cast<std::size_t>(i)].load(std::memory_order_relaxed):-1; }

void VeloriaAudioProcessor::setParameterFromMidi(int parameterIndex,float normalisedValue) noexcept
{
    if(!juce::isPositiveAndBelow(parameterIndex,midiLearnParameterCount))return;
    if(auto* p=parameters.getParameter(midiLearnParameterIds[static_cast<std::size_t>(parameterIndex)]))p->setValueNotifyingHost(juce::jlimit(0.0f,1.0f,normalisedValue));
}

void VeloriaAudioProcessor::handleMidiController(const juce::MidiMessage& message) noexcept
{
    const auto cc=message.getControllerNumber(); const auto value=static_cast<float>(message.getControllerValue())/127.0f;
    const auto learn=midiLearnTarget.exchange(-1,std::memory_order_relaxed);
    if(learn>=0&&learn<midiLearnParameterCount)midiCCMappings[static_cast<std::size_t>(learn)].store(cc,std::memory_order_relaxed);
    for(int i=0;i<midiLearnParameterCount;++i)if(midiCCMappings[static_cast<std::size_t>(i)].load(std::memory_order_relaxed)==cc)setParameterFromMidi(i,value);
}

juce::File VeloriaAudioProcessor::getUserPresetDirectory() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Latham Audio").getChildFile("Veloria").getChildFile("Presets");
}

juce::File VeloriaAudioProcessor::getUserPresetFile(const juce::String& name) const
{
    return getUserPresetDirectory().getChildFile(juce::File::createLegalFileName(name.trim())+".veloria");
}

juce::StringArray VeloriaAudioProcessor::getUserPresetNames() const
{
    juce::StringArray names; const auto directory=getUserPresetDirectory(); if(!directory.isDirectory())return names;
    juce::Array<juce::File> files; directory.findChildFiles(files,juce::File::findFiles,false,"*.veloria"); files.sort();
    for(const auto& file:files)names.add(file.getFileNameWithoutExtension()); return names;
}

void VeloriaAudioProcessor::appendMidiMappingsToState(juce::ValueTree& state) const
{
    for(int i=0;i<midiLearnParameterCount;++i)
        state.setProperty(juce::Identifier("midiCC_"+juce::String(midiLearnParameterIds[static_cast<std::size_t>(i)])),midiCCMappings[static_cast<std::size_t>(i)].load(std::memory_order_relaxed),nullptr);
}

void VeloriaAudioProcessor::restoreMidiMappingsFromState(const juce::ValueTree& state)
{
    for(int i=0;i<midiLearnParameterCount;++i)
    {
        const auto property=juce::Identifier("midiCC_"+juce::String(midiLearnParameterIds[static_cast<std::size_t>(i)]));
        midiCCMappings[static_cast<std::size_t>(i)].store(static_cast<int>(state.getProperty(property,-1)),std::memory_order_relaxed);
    }
}

juce::ValueTree VeloriaAudioProcessor::makeSerializableState() const
{
    auto state=parameters.copyState(); state.setProperty("currentProgram",currentProgram,nullptr); state.setProperty("drumMode",drumMode,nullptr); appendMidiMappingsToState(state); return state;
}

void VeloriaAudioProcessor::restoreSerializableState(const juce::ValueTree& state)
{
    if(!state.isValid())return; currentProgram=static_cast<int>(state.getProperty("currentProgram",-1)); drumMode=static_cast<bool>(state.getProperty("drumMode",currentProgram==drumPresetIndex)); restoreMidiMappingsFromState(state); parameters.replaceState(state); stopAllVoices(false);
}

bool VeloriaAudioProcessor::saveUserPreset(const juce::String& name)
{
    const auto trimmed=name.trim(); if(trimmed.isEmpty())return false; const auto directory=getUserPresetDirectory(); if(!directory.exists()&&directory.createDirectory().failed())return false;
    auto state=makeSerializableState(); state.setProperty("presetName",trimmed,nullptr); if(auto xml=state.createXml())return getUserPresetFile(trimmed).replaceWithText(xml->toString()); return false;
}

bool VeloriaAudioProcessor::loadUserPreset(const juce::String& name)
{
    const auto file=getUserPresetFile(name); if(!file.existsAsFile())return false; if(auto xml=juce::XmlDocument::parse(file)){restoreSerializableState(juce::ValueTree::fromXml(*xml)); currentProgram=-1; return true;} return false;
}

bool VeloriaAudioProcessor::renameUserPreset(const juce::String& oldName,const juce::String& newName)
{
    const auto oldFile=getUserPresetFile(oldName); const auto trimmed=newName.trim(); if(!oldFile.existsAsFile()||trimmed.isEmpty())return false; const auto newFile=getUserPresetFile(trimmed); if(newFile.existsAsFile())return false; if(!oldFile.moveFileTo(newFile))return false;
    if(auto xml=juce::XmlDocument::parse(newFile)){auto state=juce::ValueTree::fromXml(*xml); state.setProperty("presetName",trimmed,nullptr); if(auto updated=state.createXml())newFile.replaceWithText(updated->toString());} return true;
}

bool VeloriaAudioProcessor::deleteUserPreset(const juce::String& name)
{
    const auto file=getUserPresetFile(name); return file.existsAsFile()&&file.deleteFile();
}

juce::AudioProcessorEditor* VeloriaAudioProcessor::createEditor(){return new VeloriaAudioProcessorEditor(*this);}
void VeloriaAudioProcessor::getStateInformation(juce::MemoryBlock& destination){if(auto xml=makeSerializableState().createXml())copyXmlToBinary(*xml,destination);}
void VeloriaAudioProcessor::setStateInformation(const void* data,int size){if(auto xml=getXmlFromBinary(data,size))restoreSerializableState(juce::ValueTree::fromXml(*xml));}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new VeloriaAudioProcessor();}
