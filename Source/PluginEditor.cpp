#include "PluginEditor.h"

VeloriaAudioProcessorEditor::VeloriaAudioProcessorEditor(VeloriaAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(860, 500);

    title.setText("VELORIA", juce::dontSendNotification);
    title.setJustificationType(juce::Justification::centred);
    title.setFont(juce::FontOptions(42.0f, juce::Font::bold));
    addAndMakeVisible(title);

    subtitle.setText("PRESET 01 — STOCHASTIC CORE", juce::dontSendNotification);
    subtitle.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(subtitle);

    for (auto* slider : { &ampWalk, &timeWalk, &correlation, &curve, &level })
    {
        slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 22);
        addAndMakeVisible(slider);
    }

    ampWalkAttachment = std::make_unique<SliderAttachment>(processor.parameters, "ampWalk", ampWalk);
    timeWalkAttachment = std::make_unique<SliderAttachment>(processor.parameters, "timeWalk", timeWalk);
    correlationAttachment = std::make_unique<SliderAttachment>(processor.parameters, "correlation", correlation);
    curveAttachment = std::make_unique<SliderAttachment>(processor.parameters, "curve", curve);
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

    const std::array<std::pair<juce::Slider*, const char*>, 5> labels {{
        { &ampWalk, "AMP WALK" },
        { &timeWalk, "TIME WALK" },
        { &correlation, "CORRELATION" },
        { &curve, "CURVE" },
        { &level, "OUTPUT" }
    }};

    g.setColour(juce::Colours::white.withAlpha(0.78f));
    g.setFont(13.0f);
    for (const auto& [slider, text] : labels)
        g.drawFittedText(text, slider->getBounds().translated(0, -24), juce::Justification::centred, 1);
}

void VeloriaAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(48);
    title.setBounds(area.removeFromTop(66));
    subtitle.setBounds(area.removeFromTop(30));
    area.removeFromTop(70);

    const auto width = area.getWidth() / 5;
    for (auto* slider : { &ampWalk, &timeWalk, &correlation, &curve, &level })
        slider->setBounds(area.removeFromLeft(width).withSizeKeepingCentre(130, 145));
}
