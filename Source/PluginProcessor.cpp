#include "PluginProcessor.h"
#include "PluginEditor.h"

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
        "level", "Level", juce::NormalisableRange<float>(-48.0f, 0.0f), -12.0f));
    layout.push_back(std::make_unique<juce::AudioParameterInt>("seed", "Seed", 1, 999999, 2207));

    return { layout.begin(), layout.end() };
}

void VeloriaAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    oscillator.prepare(sampleRate);
    amplitudeEnvelope.setSampleRate(sampleRate);

    // Preset 01 is deliberately neutral: enough attack/release to audition the
    // evolving source without masking it with a conventional synth envelope.
    envelopeParameters.attack = 0.08f;
    envelopeParameters.decay = 0.45f;
    envelopeParameters.sustain = 0.88f;
    envelopeParameters.release = 1.4f;
    amplitudeEnvelope.setParameters(envelopeParameters);

    outputGain.prepare({ sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2 });
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

    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();
        if (message.isNoteOn())
        {
            currentMidiNote = message.getNoteNumber();
            currentFrequency = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(currentMidiNote));
            oscillator.reset();
            amplitudeEnvelope.noteOn();
        }
        else if (message.isNoteOff() && message.getNoteNumber() == currentMidiNote)
        {
            amplitudeEnvelope.noteOff();
            currentMidiNote = -1;
        }
    }

    oscillator.setFrequency(currentFrequency);
    oscillator.setAmplitudeWalk(parameters.getRawParameterValue("ampWalk")->load());
    oscillator.setTimeWalk(parameters.getRawParameterValue("timeWalk")->load());
    oscillator.setCorrelation(parameters.getRawParameterValue("correlation")->load());
    oscillator.setCurve(parameters.getRawParameterValue("curve")->load());

    const auto requestedSeed = static_cast<std::uint32_t>(parameters.getRawParameterValue("seed")->load());
    if (requestedSeed != currentSeed)
    {
        currentSeed = requestedSeed;
        oscillator.setSeed(currentSeed);
    }

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto value = oscillator.processSample() * amplitudeEnvelope.getNextSample() * 0.42f;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.setSample(channel, sample, value);
    }

    outputGain.setGainDecibels(parameters.getRawParameterValue("level")->load());
    juce::dsp::AudioBlock<float> block(buffer);
    outputGain.process(juce::dsp::ProcessContextReplacing<float>(block));
}

juce::AudioProcessorEditor* VeloriaAudioProcessor::createEditor() { return new VeloriaAudioProcessorEditor(*this); }

void VeloriaAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary(*xml, destination);
}

void VeloriaAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VeloriaAudioProcessor(); }
