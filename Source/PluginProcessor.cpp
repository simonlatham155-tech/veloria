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
        "instability", "Instability", juce::NormalisableRange<float>(0.0f, 1.0f), 0.35f));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>(
        "level", "Level", juce::NormalisableRange<float>(-48.0f, 0.0f), -12.0f));
    layout.push_back(std::make_unique<juce::AudioParameterInt>(
        "seed", "Seed", 1, 999999, 1));
    return { layout.begin(), layout.end() };
}

void VeloriaAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    oscillator.prepare(sampleRate);
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
            currentFrequency = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(message.getNoteNumber()));
            noteActive = true;
        }
        else if (message.isNoteOff())
        {
            noteActive = false;
        }
    }

    oscillator.setFrequency(currentFrequency);
    oscillator.setInstability(parameters.getRawParameterValue("instability")->load());
    oscillator.setSeed(static_cast<std::uint32_t>(parameters.getRawParameterValue("seed")->load()));

    if (noteActive)
    {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const auto value = oscillator.processSample() * 0.2f;
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                buffer.setSample(channel, sample, value);
        }
    }

    outputGain.setGainDecibels(parameters.getRawParameterValue("level")->load());
    juce::dsp::AudioBlock<float> block(buffer);
    outputGain.process(juce::dsp::ProcessContextReplacing<float>(block));
}

juce::AudioProcessorEditor* VeloriaAudioProcessor::createEditor()
{
    return new VeloriaAudioProcessorEditor(*this);
}

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

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VeloriaAudioProcessor();
}
