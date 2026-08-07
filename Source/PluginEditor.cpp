#include "PluginEditor.h"

VeloriaAudioProcessorEditor::VeloriaAudioProcessorEditor(VeloriaAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(920, 590);

    title.setText("VELORIA", juce::dontSendNotification);
    title.setJustificationType(juce::Justification::centred);
    title.setFont(juce::FontOptions(42.0f, juce::Font::bold));
    addAndMakeVisible(title);

    subtitle.setText("DYNAMIC STOCHASTIC INSTRUMENT", juce::dontSendNotification);
    subtitle.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(subtitle);

    auto names = processor.getFactoryPresetNames();
    for (int i = 0; i < names.size(); ++i)
        presetBox.addItem(names[i], i + 1);
    presetBox.setSelectedId(juce::jmax(1, processor.getCurrentProgram() + 1), juce::dontSendNotification);
    presetBox.onChange = [this]
    {
        const auto index = presetBox.getSelectedId() - 1;
        if (index >= 0)
            processor.setCurrentProgram(index);
    };
    addAndMakeVisible(presetBox);

    discoverButton.onClick = [this]
    {
        processor.discover();
        presetBox.setText("Discovered", juce::dontSendNotification);
    };
    addAndMakeVisible(discoverButton);

    monoButton.setClickingTogglesState(true);
    addAndMakeVisible(monoButton);

    midiHint.setText("All parameters are host-automatable — use Ableton MIDI Map for controller testing", juce::dontSendNotification);
    midiHint.setJustificationType(juce::Justification::centred);
    midiHint.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.55f));
    midiHint.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(midiHint);

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
    monoAttachment = std::make_unique<ButtonAttachment>(processor.parameters, "mono", monoButton);
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

    g.setFont(11.0f);
    g.setColour(juce::Colours::white.withAlpha(0.45f));
    g.drawFittedText("POLY = 8 VOICES", monoButton.getBounds().translated(0, 28), juce::Justification::centred, 1);
}

void VeloriaAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(48);
    title.setBounds(area.removeFromTop(60));
    subtitle.setBounds(area.removeFromTop(26));
    area.removeFromTop(20);

    auto toolbar = area.removeFromTop(42);
    presetBox.setBounds(toolbar.removeFromLeft(250));
    toolbar.removeFromLeft(12);
    discoverButton.setBounds(toolbar.removeFromLeft(140));
    toolbar.removeFromLeft(12);
    monoButton.setBounds(toolbar.removeFromLeft(105));

    area.removeFromTop(76);

    auto controls = area.removeFromTop(175);
    const auto width = controls.getWidth() / 5;
    for (auto* slider : { &ampWalk, &timeWalk, &correlation, &curve, &level })
        slider->setBounds(controls.removeFromLeft(width).withSizeKeepingCentre(135, 150));

    area.removeFromTop(35);
    midiHint.setBounds(area.removeFromTop(28));
}
