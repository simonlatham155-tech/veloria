#include "PluginEditor.h"

VeloriaAudioProcessorEditor::VeloriaAudioProcessorEditor(VeloriaAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(760, 460);

    title.setText("VELORIA", juce::dontSendNotification);
    title.setJustificationType(juce::Justification::centred);
    title.setFont(juce::FontOptions(42.0f, juce::Font::bold));
    addAndMakeVisible(title);

    subtitle.setText("Fifty years of synthesis that never happened", juce::dontSendNotification);
    subtitle.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(subtitle);

    for (auto* slider : { &instability, &level })
    {
        slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 24);
        addAndMakeVisible(slider);
    }

    instability.setName("Instability");
    level.setName("Level");

    instabilityAttachment = std::make_unique<SliderAttachment>(processor.parameters, "instability", instability);
    levelAttachment = std::make_unique<SliderAttachment>(processor.parameters, "level", level);
}

void VeloriaAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(11, 10, 18));

    auto panel = getLocalBounds().reduced(28).toFloat();
    g.setColour(juce::Colour::fromRGB(33, 26, 49));
    g.fillRoundedRectangle(panel, 18.0f);

    g.setColour(juce::Colour::fromRGB(184, 140, 255));
    g.drawRoundedRectangle(panel, 18.0f, 1.5f);

    g.setColour(juce::Colours::white.withAlpha(0.75f));
    g.setFont(14.0f);
    g.drawFittedText("INSTABILITY", instability.getBounds().translated(0, -28), juce::Justification::centred, 1);
    g.drawFittedText("OUTPUT", level.getBounds().translated(0, -28), juce::Justification::centred, 1);
}

void VeloriaAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(48);
    title.setBounds(area.removeFromTop(70));
    subtitle.setBounds(area.removeFromTop(34));
    area.removeFromTop(55);

    const auto controlWidth = 180;
    instability.setBounds(area.removeFromLeft(controlWidth).withSizeKeepingCentre(150, 150));
    level.setBounds(area.removeFromRight(controlWidth).withSizeKeepingCentre(150, 150));
}
