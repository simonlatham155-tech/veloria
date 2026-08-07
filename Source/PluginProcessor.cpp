#include "PluginProcessor.h"
#include "PluginEditor.h"

const std::array<VeloriaAudioProcessor::FactoryPreset, 10> VeloriaAudioProcessor::factoryPresets {{
    { "Core",    0.14f, 0.08f, 0.22f, 0.65f, 0.08f, 0.45f, 0.88f, 1.40f, 2207 },
    { "Piano",   0.08f, 0.03f, 0.42f, 0.78f, 0.005f, 0.90f, 0.12f, 0.65f, 3101 },
    { "Pad",     0.18f, 0.12f, 0.58f, 0.92f, 1.50f, 1.80f, 0.82f, 4.50f, 4201 },
    { "Strings", 0.11f, 0.08f, 0.70f, 0.88f, 0.45f, 0.90f, 0.74f, 1.70f, 5303 },
    { "Bass",    0.06f, 0.03f, 0.55f, 0.52f, 0.01f, 0.25f, 0.68f, 0.35f, 6407 },
    { "Lead",    0.12f, 0.05f, 0.35f, 0.45f, 0.015f, 0.35f, 0.75f, 0.50f, 7507 },
    { "Bell",    0.09f, 0.02f, 0.28f, 0.82f, 0.002f, 1.60f, 0.02f, 1.20f, 8609 },
    { "Pluck",   0.14f, 0.08f, 0.25f, 0.60f, 0.002f, 0.35f, 0.05f, 0.30f, 9719 },
    { "FX",      0.60f, 0.55f, 0.05f, 0.20f, 0.05f, 1.00f, 0.65f, 3.50f, 10831 },
    { "Drums",   0.40f, 0.30f, 0.10f, 0.15f, 0.001f, 0.18f, 0.00f, 0.08f, 11939 }
}};

VeloriaAudioProcessor::VeloriaAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout VeloriaAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> layout;

    layout.push_back(std::make_unique<juce::AudioParameterFloat>(
        "ampWalk", "Amplitude Walk", juce::NormalisableRange<float>(0.0f, 1.0f), 0.14f));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>(
        "timeWalk", "Time Walk", juce::NormalisableRange<float>(0.0f, 1.0f), 0.08f));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>(
        "correlation", "Correlation", juce::NormalisableRange<float>(0.0f, 1.0f), 0.22f));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>(
        "curve", "Curve", juce::NormalisableRange<float>(0.0f, 1.0f), 0.65f));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>(
        "attack", "Attack", juce::NormalisableRange<float>(0.001f, 5.0f, 0.0f, 0.35f), 0.08f));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>(
        "decay", "Decay", juce::NormalisableRange<float>(0.01f, 6.0f, 0.0f, 0.35f), 0.45f));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>(
        "sustain", "Sustain", juce::NormalisableRange<float>(0.0f, 1.0f), 0.88f));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>(
        "release", "Release", juce::NormalisableRange<float>(0.01f, 8.0f, 0.0f, 0.35f), 1.40f));
    layout.push_back(std::make_unique<juce::AudioParameterBool>("mono", "Mono", false));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>(
        "level", "Level", juce::NormalisableRange<float>(-48.0f, 0.0f), -12.0f));
    layout.push_back(std::make_unique<juce::AudioParameterInt>("seed", "Seed", 1, 999999, 2207));

    return { layout.begin(), layout.end() };
}

void VeloriaAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    for (auto& voice : voices)
    {
        voice.oscillator.prepare(sampleRate);
        voice.envelope.setSampleRate(sampleRate);
        voice.active = false;
        voice.midiNote = -1;
    }

    outputGain.prepare({ sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2 });
    updateVoiceParameters();
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
        if (message.isNoteOn())
            startNote(message.getNoteNumber(), message.getFloatVelocity());
        else if (message.isNoteOff())
            stopNote(message.getNoteNumber());
        else if (message.isAllNotesOff() || message.isAllSoundOff())
            stopAllVoices(false);
    }

    const auto outputChannels = buffer.getNumChannels();
    const auto voiceGain = 0.42f / std::sqrt(static_cast<float>(maxVoices));

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float mixed = 0.0f;

        for (auto& voice : voices)
        {
            if (! voice.active)
                continue;

            const auto envelopeValue = voice.envelope.getNextSample();
            mixed += voice.oscillator.processSample() * envelopeValue * voiceGain;

            if (! voice.envelope.isActive())
            {
                voice.active = false;
                voice.midiNote = -1;
            }
        }

        for (int channel = 0; channel < outputChannels; ++channel)
            buffer.setSample(channel, sample, mixed);
    }

    outputGain.setGainDecibels(parameters.getRawParameterValue("level")->load());
    juce::dsp::AudioBlock<float> block(buffer);
    outputGain.process(juce::dsp::ProcessContextReplacing<float>(block));
}

void VeloriaAudioProcessor::startNote(int midiNote, float)
{
    const auto mono = parameters.getRawParameterValue("mono")->load() > 0.5f;
    if (mono)
        stopAllVoices(false);

    auto& voice = mono ? voices.front() : findVoiceToStart();
    voice.active = true;
    voice.midiNote = midiNote;
    voice.age = ++voiceCounter;

    const auto baseSeed = static_cast<std::uint32_t>(parameters.getRawParameterValue("seed")->load());
    const auto voiceIndex = static_cast<std::uint32_t>(&voice - voices.data());
    const auto derivedSeed = baseSeed + voiceIndex * 101u + static_cast<std::uint32_t>(midiNote) * 17u;

    voice.oscillator.setSeed(derivedSeed == 0 ? 1u : derivedSeed);
    voice.oscillator.setFrequency(static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNote)));
    voice.oscillator.reset();
    voice.envelope.reset();
    voice.envelope.noteOn();
}

void VeloriaAudioProcessor::stopNote(int midiNote)
{
    for (auto& voice : voices)
        if (voice.active && voice.midiNote == midiNote)
            voice.envelope.noteOff();
}

void VeloriaAudioProcessor::stopAllVoices(bool allowTailOff)
{
    for (auto& voice : voices)
    {
        if (allowTailOff)
            voice.envelope.noteOff();
        else
        {
            voice.envelope.reset();
            voice.active = false;
            voice.midiNote = -1;
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
    return *oldest;
}

void VeloriaAudioProcessor::updateVoiceParameters()
{
    const auto ampWalk = parameters.getRawParameterValue("ampWalk")->load();
    const auto timeWalk = parameters.getRawParameterValue("timeWalk")->load();
    const auto correlation = parameters.getRawParameterValue("correlation")->load();
    const auto curve = parameters.getRawParameterValue("curve")->load();

    juce::ADSR::Parameters envelopeParameters;
    envelopeParameters.attack = parameters.getRawParameterValue("attack")->load();
    envelopeParameters.decay = parameters.getRawParameterValue("decay")->load();
    envelopeParameters.sustain = parameters.getRawParameterValue("sustain")->load();
    envelopeParameters.release = parameters.getRawParameterValue("release")->load();

    for (auto& voice : voices)
    {
        voice.oscillator.setAmplitudeWalk(ampWalk);
        voice.oscillator.setTimeWalk(timeWalk);
        voice.oscillator.setCorrelation(correlation);
        voice.oscillator.setCurve(curve);
        voice.envelope.setParameters(envelopeParameters);
    }
}

int VeloriaAudioProcessor::getNumPrograms()
{
    return static_cast<int>(factoryPresets.size());
}

const juce::String VeloriaAudioProcessor::getProgramName(int index)
{
    if (juce::isPositiveAndBelow(index, getNumPrograms()))
        return factoryPresets[static_cast<std::size_t>(index)].name;
    return {};
}

juce::StringArray VeloriaAudioProcessor::getFactoryPresetNames() const
{
    juce::StringArray names;
    for (const auto& preset : factoryPresets)
        names.add(preset.name);
    return names;
}

void VeloriaAudioProcessor::setCurrentProgram(int index)
{
    if (! juce::isPositiveAndBelow(index, getNumPrograms()))
        return;

    currentProgram = index;
    applyFactoryPreset(index);
}

void VeloriaAudioProcessor::applyFactoryPreset(int index)
{
    const auto& preset = factoryPresets[static_cast<std::size_t>(index)];
    setParameter("ampWalk", preset.ampWalk);
    setParameter("timeWalk", preset.timeWalk);
    setParameter("correlation", preset.correlation);
    setParameter("curve", preset.curve);
    setParameter("attack", preset.attack);
    setParameter("decay", preset.decay);
    setParameter("sustain", preset.sustain);
    setParameter("release", preset.release);
    setParameter("seed", static_cast<float>(preset.seed));
}

void VeloriaAudioProcessor::discover()
{
    currentProgram = -1;

    setParameter("ampWalk", 0.03f + discoveryRandom.nextFloat() * 0.72f);
    setParameter("timeWalk", 0.01f + discoveryRandom.nextFloat() * 0.54f);
    setParameter("correlation", discoveryRandom.nextFloat() * 0.85f);
    setParameter("curve", 0.10f + discoveryRandom.nextFloat() * 0.90f);

    const auto percussive = discoveryRandom.nextBool();
    if (percussive)
    {
        setParameter("attack", 0.001f + discoveryRandom.nextFloat() * 0.08f);
        setParameter("decay", 0.08f + discoveryRandom.nextFloat() * 1.50f);
        setParameter("sustain", discoveryRandom.nextFloat() * 0.45f);
        setParameter("release", 0.05f + discoveryRandom.nextFloat() * 1.50f);
    }
    else
    {
        setParameter("attack", 0.05f + discoveryRandom.nextFloat() * 1.75f);
        setParameter("decay", 0.20f + discoveryRandom.nextFloat() * 2.50f);
        setParameter("sustain", 0.45f + discoveryRandom.nextFloat() * 0.50f);
        setParameter("release", 0.40f + discoveryRandom.nextFloat() * 4.60f);
    }

    setParameter("seed", static_cast<float>(1 + discoveryRandom.nextInt(999998)));
    stopAllVoices(false);
}

void VeloriaAudioProcessor::setParameter(const juce::String& id, float value)
{
    if (auto* parameter = parameters.getParameter(id))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        parameter->endChangeGesture();
    }
}

juce::AudioProcessorEditor* VeloriaAudioProcessor::createEditor()
{
    return new VeloriaAudioProcessorEditor(*this);
}

void VeloriaAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    auto state = parameters.copyState();
    state.setProperty("currentProgram", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destination);
}

void VeloriaAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
    {
        auto state = juce::ValueTree::fromXml(*xml);
        currentProgram = static_cast<int>(state.getProperty("currentProgram", -1));
        parameters.replaceState(state);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VeloriaAudioProcessor();
}
