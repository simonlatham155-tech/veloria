#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

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
    { "Drums",       0.36f, 0.24f, 0.94f, 0.72f, 0.001f,0.16f, 0.00f, 0.07f,11939 }
}};

const std::array<const char*, VeloriaAudioProcessor::midiLearnParameterCount>
VeloriaAudioProcessor::midiLearnParameterIds {{
    "ampWalk", "timeWalk", "ampMirror", "timeMirror",
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
    layout.push_back(std::make_unique<juce::AudioParameterFloat>("ampMirror", "Amplitude Mirror", juce::NormalisableRange<float>(0.05f, 1.0f), 0.88f));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>("timeMirror", "Time Mirror", juce::NormalisableRange<float>(0.05f, 1.0f), 0.45f));
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
    for (auto& voice : voices)
    {
        voice.oscillator.prepare(sampleRate);
        voice.envelope.setSampleRate(sampleRate);
        voice.active = false;
        voice.midiNote = -1;
    }

    outputGain.prepare({ sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2 });
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

    const auto voiceGain = 0.52f / std::sqrt(static_cast<float>(maxVoices));
    double energyAccumulator = 0.0;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float mixed = 0.0f;
        for (auto& voice : voices)
        {
            if (! voice.active)
                continue;

            const auto env = voice.envelope.getNextSample();
            mixed += voice.oscillator.processSample() * env * voiceGain;

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
    const auto ampMirror = parameters.getRawParameterValue("ampMirror")->load();
    const auto timeMirror = parameters.getRawParameterValue("timeMirror")->load();

    juce::ADSR::Parameters env;
    env.attack = parameters.getRawParameterValue("attack")->load();
    env.decay = parameters.getRawParameterValue("decay")->load();
    env.sustain = parameters.getRawParameterValue("sustain")->load();
    env.release = parameters.getRawParameterValue("release")->load();

    for (auto& voice : voices)
    {
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

int VeloriaAudioProcessor::getNumPrograms()
{
    return static_cast<int>(factoryPresets.size());
}

const juce::String VeloriaAudioProcessor::getProgramName(int index)
{
    return juce::isPositiveAndBelow(index, getNumPrograms())
        ? juce::String(factoryPresets[static_cast<std::size_t>(index)].name)
        : juce::String();
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
    const auto& p = factoryPresets[static_cast<std::size_t>(index)];
    setParameterValue("ampWalk", p.ampWalk);
    setParameterValue("timeWalk", p.timeWalk);
    setParameterValue("ampMirror", p.ampMirror);
    setParameterValue("timeMirror", p.timeMirror);
    setParameterValue("attack", p.attack);
    setParameterValue("decay", p.decay);
    setParameterValue("sustain", p.sustain);
    setParameterValue("release", p.release);
    setParameterValue("seed", static_cast<float>(p.seed));
}

void VeloriaAudioProcessor::discover()
{
    currentProgram = -1;
    setParameterValue("ampWalk", 0.01f + discoveryRandom.nextFloat() * 0.60f);
    setParameterValue("timeWalk", 0.005f + discoveryRandom.nextFloat() * 0.48f);
    setParameterValue("ampMirror", 0.25f + discoveryRandom.nextFloat() * 0.75f);
    setParameterValue("timeMirror", 0.08f + discoveryRandom.nextFloat() * 0.82f);
    setParameterValue("seed", static_cast<float>(1 + discoveryRandom.nextInt(999998)));
    stopAllVoices(false);
}

void VeloriaAudioProcessor::newField()
{
    currentProgram = -1;
    setParameterValue("seed", static_cast<float>(1 + discoveryRandom.nextInt(999998)));
    stopAllVoices(false);
}

void VeloriaAudioProcessor::setParameterValue(const juce::String& id, float value)
{
    if (auto* parameter = parameters.getParameter(id))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        parameter->endChangeGesture();
    }
}

int VeloriaAudioProcessor::findMidiParameterIndex(const juce::String& id) const noexcept
{
    for (int i = 0; i < midiLearnParameterCount; ++i)
        if (id == midiLearnParameterIds[static_cast<std::size_t>(i)])
            return i;
    return -1;
}

void VeloriaAudioProcessor::beginMidiLearn(const juce::String& parameterId) noexcept
{
    midiLearnTarget.store(findMidiParameterIndex(parameterId), std::memory_order_relaxed);
}

void VeloriaAudioProcessor::clearMidiMapping(const juce::String& parameterId) noexcept
{
    const auto index = findMidiParameterIndex(parameterId);
    if (index >= 0)
        midiCCMappings[static_cast<std::size_t>(index)].store(-1, std::memory_order_relaxed);
}

int VeloriaAudioProcessor::getMidiCCForParameter(const juce::String& parameterId) const noexcept
{
    const auto index = findMidiParameterIndex(parameterId);
    return index >= 0
        ? midiCCMappings[static_cast<std::size_t>(index)].load(std::memory_order_relaxed)
        : -1;
}

void VeloriaAudioProcessor::setParameterFromMidi(int parameterIndex, float normalisedValue) noexcept
{
    if (! juce::isPositiveAndBelow(parameterIndex, midiLearnParameterCount))
        return;

    if (auto* parameter = parameters.getParameter(midiLearnParameterIds[static_cast<std::size_t>(parameterIndex)]))
        parameter->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, normalisedValue));
}

void VeloriaAudioProcessor::handleMidiController(const juce::MidiMessage& message) noexcept
{
    const auto cc = message.getControllerNumber();
    const auto value = static_cast<float>(message.getControllerValue()) / 127.0f;

    const auto learnTarget = midiLearnTarget.exchange(-1, std::memory_order_relaxed);
    if (learnTarget >= 0 && learnTarget < midiLearnParameterCount)
        midiCCMappings[static_cast<std::size_t>(learnTarget)].store(cc, std::memory_order_relaxed);

    for (int i = 0; i < midiLearnParameterCount; ++i)
        if (midiCCMappings[static_cast<std::size_t>(i)].load(std::memory_order_relaxed) == cc)
            setParameterFromMidi(i, value);
}

juce::File VeloriaAudioProcessor::getUserPresetDirectory() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Latham Audio")
        .getChildFile("Veloria")
        .getChildFile("Presets");
}

juce::File VeloriaAudioProcessor::getUserPresetFile(const juce::String& name) const
{
    const auto legal = juce::File::createLegalFileName(name.trim());
    return getUserPresetDirectory().getChildFile(legal + ".veloria");
}

juce::StringArray VeloriaAudioProcessor::getUserPresetNames() const
{
    juce::StringArray names;
    const auto directory = getUserPresetDirectory();
    if (! directory.isDirectory())
        return names;

    juce::Array<juce::File> files;
    directory.findChildFiles(files, juce::File::findFiles, false, "*.veloria");
    files.sort();

    for (const auto& file : files)
        names.add(file.getFileNameWithoutExtension());

    return names;
}

void VeloriaAudioProcessor::appendMidiMappingsToState(juce::ValueTree& state) const
{
    for (int i = 0; i < midiLearnParameterCount; ++i)
    {
        const auto propertyName = juce::Identifier("midiCC_" + juce::String(midiLearnParameterIds[static_cast<std::size_t>(i)]));
        state.setProperty(propertyName,
                          midiCCMappings[static_cast<std::size_t>(i)].load(std::memory_order_relaxed),
                          nullptr);
    }
}

void VeloriaAudioProcessor::restoreMidiMappingsFromState(const juce::ValueTree& state)
{
    for (int i = 0; i < midiLearnParameterCount; ++i)
    {
        const auto propertyName = juce::Identifier("midiCC_" + juce::String(midiLearnParameterIds[static_cast<std::size_t>(i)]));
        const auto value = static_cast<int>(state.getProperty(propertyName, -1));
        midiCCMappings[static_cast<std::size_t>(i)].store(value, std::memory_order_relaxed);
    }
}

juce::ValueTree VeloriaAudioProcessor::makeSerializableState() const
{
    auto state = parameters.copyState();
    state.setProperty("currentProgram", currentProgram, nullptr);
    appendMidiMappingsToState(state);
    return state;
}

void VeloriaAudioProcessor::restoreSerializableState(const juce::ValueTree& state)
{
    if (! state.isValid())
        return;

    currentProgram = static_cast<int>(state.getProperty("currentProgram", -1));
    restoreMidiMappingsFromState(state);
    parameters.replaceState(state);
    stopAllVoices(false);
}

bool VeloriaAudioProcessor::saveUserPreset(const juce::String& name)
{
    const auto trimmed = name.trim();
    if (trimmed.isEmpty())
        return false;

    const auto directory = getUserPresetDirectory();
    if (! directory.exists() && directory.createDirectory().failed())
        return false;

    auto state = makeSerializableState();
    state.setProperty("presetName", trimmed, nullptr);
    if (auto xml = state.createXml())
        return getUserPresetFile(trimmed).replaceWithText(xml->toString());

    return false;
}

bool VeloriaAudioProcessor::loadUserPreset(const juce::String& name)
{
    const auto file = getUserPresetFile(name);
    if (! file.existsAsFile())
        return false;

    if (auto xml = juce::XmlDocument::parse(file))
    {
        restoreSerializableState(juce::ValueTree::fromXml(*xml));
        currentProgram = -1;
        return true;
    }

    return false;
}

bool VeloriaAudioProcessor::renameUserPreset(const juce::String& oldName, const juce::String& newName)
{
    const auto oldFile = getUserPresetFile(oldName);
    const auto trimmed = newName.trim();
    if (! oldFile.existsAsFile() || trimmed.isEmpty())
        return false;

    const auto newFile = getUserPresetFile(trimmed);
    if (newFile.existsAsFile())
        return false;

    if (! oldFile.moveFileTo(newFile))
        return false;

    if (auto xml = juce::XmlDocument::parse(newFile))
    {
        auto state = juce::ValueTree::fromXml(*xml);
        state.setProperty("presetName", trimmed, nullptr);
        if (auto updated = state.createXml())
            newFile.replaceWithText(updated->toString());
    }

    return true;
}

bool VeloriaAudioProcessor::deleteUserPreset(const juce::String& name)
{
    const auto file = getUserPresetFile(name);
    return file.existsAsFile() && file.deleteFile();
}

juce::AudioProcessorEditor* VeloriaAudioProcessor::createEditor()
{
    return new VeloriaAudioProcessorEditor(*this);
}

void VeloriaAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    if (auto xml = makeSerializableState().createXml())
        copyXmlToBinary(*xml, destination);
}

void VeloriaAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        restoreSerializableState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VeloriaAudioProcessor();
}
