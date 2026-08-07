#include "PluginEditor.h"
#include <cmath>

namespace
{
const auto background = juce::Colour::fromRGB(5, 6, 10);
const auto panel = juce::Colour::fromRGB(12, 13, 20);
const auto purple = juce::Colour::fromRGB(185, 95, 255);
const auto violet = juce::Colour::fromRGB(118, 74, 255);
const auto gold = juce::Colour::fromRGB(255, 189, 92);
const auto cyan = juce::Colour::fromRGB(106, 225, 255);
}

VeloriaAudioProcessorEditor::VeloriaAudioProcessorEditor(VeloriaAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(1180, 760);

    title.setText("V E L O R I A", juce::dontSendNotification);
    title.setJustificationType(juce::Justification::centred);
    title.setFont(juce::FontOptions(38.0f, juce::Font::bold));
    addAndMakeVisible(title);

    subtitle.setText("GENDYN // DYNAMIC STOCHASTIC INSTRUMENT", juce::dontSendNotification);
    subtitle.setJustificationType(juce::Justification::centred);
    subtitle.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.55f));
    subtitle.setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(subtitle);

    auto names = audioProcessor.getFactoryPresetNames();
    for (int i = 0; i < names.size(); ++i)
        presetBox.addItem(names[i], i + 1);
    presetBox.setSelectedId(juce::jmax(1, audioProcessor.getCurrentProgram() + 1), juce::dontSendNotification);
    presetBox.onChange = [this]
    {
        const auto index = presetBox.getSelectedId() - 1;
        if (index >= 0)
            audioProcessor.setCurrentProgram(index);
    };
    presetBox.setColour(juce::ComboBox::backgroundColourId, panel.brighter(0.15f));
    presetBox.setColour(juce::ComboBox::outlineColourId, purple.withAlpha(0.40f));
    presetBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    addAndMakeVisible(presetBox);

    discoverButton.onClick = [this]
    {
        audioProcessor.discover();
        presetBox.setText("Discovered", juce::dontSendNotification);
    };
    discoverButton.setColour(juce::TextButton::buttonColourId, purple.withAlpha(0.28f));
    discoverButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(discoverButton);

    monoButton.setClickingTogglesState(true);
    monoButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    monoButton.setColour(juce::ToggleButton::tickColourId, cyan);
    addAndMakeVisible(monoButton);

    voiceStatus.setJustificationType(juce::Justification::centred);
    voiceStatus.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.68f));
    voiceStatus.setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(voiceStatus);

    midiHint.setText("LIVE STOCHASTIC FIELD  //  GLOBE = ACTUAL BREAKPOINT AMPLITUDE + TIME STATE", juce::dontSendNotification);
    midiHint.setJustificationType(juce::Justification::centred);
    midiHint.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.38f));
    midiHint.setFont(juce::FontOptions(10.0f));
    addAndMakeVisible(midiHint);

    for (auto* slider : { &ampWalk, &timeWalk, &ampMirror, &timeMirror, &level })
    {
        slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 84, 20);
        slider->setColour(juce::Slider::rotarySliderFillColourId, purple);
        slider->setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::white.withAlpha(0.12f));
        slider->setColour(juce::Slider::thumbColourId, cyan);
        slider->setColour(juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha(0.82f));
        slider->setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        slider->setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::white.withAlpha(0.10f));
        addAndMakeVisible(slider);
    }

    ampWalkAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "ampWalk", ampWalk);
    timeWalkAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "timeWalk", timeWalk);
    ampMirrorAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "ampMirror", ampMirror);
    timeMirrorAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "timeMirror", timeMirror);
    levelAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "level", level);
    monoAttachment = std::make_unique<ButtonAttachment>(audioProcessor.parameters, "mono", monoButton);

    visualState = audioProcessor.getVisualState();
    startTimerHz(30);
}

void VeloriaAudioProcessorEditor::timerCallback()
{
    visualState = audioProcessor.getVisualState();
    rotationPhase += 0.003f + visualState.energy * 0.010f;
    if (rotationPhase > juce::MathConstants<float>::twoPi)
        rotationPhase -= juce::MathConstants<float>::twoPi;

    voiceStatus.setText(visualState.activeVoices == 1 ? "1 ACTIVE VOICE"
                                                      : juce::String(visualState.activeVoices) + " ACTIVE VOICES",
                        juce::dontSendNotification);
    repaint();
}

void VeloriaAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);

    juce::ColourGradient bgGlow(purple.withAlpha(0.12f), getWidth() * 0.5f, getHeight() * 0.42f,
                                background, getWidth() * 0.5f, static_cast<float>(getHeight()), true);
    g.setGradientFill(bgGlow);
    g.fillRect(getLocalBounds());

    auto outer = getLocalBounds().reduced(10).toFloat();
    g.setColour(juce::Colours::white.withAlpha(0.07f));
    g.drawRoundedRectangle(outer, 18.0f, 1.0f);

    g.setColour(purple.withAlpha(0.32f));
    g.drawLine(20.0f, 76.0f, getWidth() - 20.0f, 76.0f, 1.0f);

    const auto globeBounds = juce::Rectangle<float>(310.0f, 145.0f, 560.0f, 500.0f);
    drawStochasticGlobe(g, globeBounds);

    drawKnobLabel(g, ampWalk, "AMP WALK");
    drawKnobLabel(g, timeWalk, "TIME WALK");
    drawKnobLabel(g, ampMirror, "AMP MIRROR");
    drawKnobLabel(g, timeMirror, "TIME MIRROR");
    drawKnobLabel(g, level, "OUTPUT");

    g.setColour(juce::Colours::white.withAlpha(0.20f));
    g.setFont(10.0f);
    g.drawText("BREAKPOINTS", 500, 636, 180, 18, juce::Justification::centred);
}

void VeloriaAudioProcessorEditor::drawKnobLabel(juce::Graphics& g, juce::Slider& slider, const juce::String& text)
{
    auto labelBounds = slider.getBounds().translated(0, -25);
    g.setColour(juce::Colours::white.withAlpha(0.82f));
    g.setFont(12.0f);
    g.drawFittedText(text, labelBounds, juce::Justification::centred, 1);
}

void VeloriaAudioProcessorEditor::drawStochasticGlobe(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    auto globe = bounds.withSizeKeepingCentre(470.0f, 470.0f);
    const auto centre = globe.getCentre();
    const auto radius = globe.getWidth() * 0.43f;
    const auto energy = juce::jlimit(0.0f, 1.0f, visualState.energy);

    juce::ColourGradient glow(purple.withAlpha(0.10f + energy * 0.18f), centre.x, centre.y,
                              juce::Colours::transparentBlack, centre.x, centre.y + radius, true);
    g.setGradientFill(glow);
    g.fillEllipse(globe.reduced(18.0f));

    g.setColour(juce::Colours::white.withAlpha(0.07f));
    for (int ring = 1; ring <= 4; ++ring)
    {
        const auto r = radius * static_cast<float>(ring) / 4.0f;
        g.drawEllipse(centre.x - r, centre.y - r, r * 2.0f, r * 2.0f, 1.0f);
    }

    g.setColour(purple.withAlpha(0.18f));
    g.drawEllipse(centre.x - radius, centre.y - radius * 0.35f, radius * 2.0f, radius * 0.70f, 1.0f);
    g.drawEllipse(centre.x - radius * 0.35f, centre.y - radius, radius * 0.70f, radius * 2.0f, 1.0f);

    float totalDuration = 0.0f;
    for (const auto d : visualState.durations)
        totalDuration += juce::jmax(0.001f, d);
    totalDuration = juce::jmax(0.001f, totalDuration);

    std::array<juce::Point<float>, VeloriaAudioProcessor::visualBreakpointCount> points {};
    float cumulative = 0.0f;
    for (std::size_t i = 0; i < points.size(); ++i)
    {
        const auto duration = juce::jmax(0.001f, visualState.durations[i]);
        const auto angle = rotationPhase - juce::MathConstants<float>::halfPi
                         + juce::MathConstants<float>::twoPi * (cumulative / totalDuration);
        cumulative += duration;

        const auto amplitude = juce::jlimit(-1.0f, 1.0f, visualState.amplitudes[i]);
        const auto radial = radius * (0.62f + amplitude * 0.28f);
        const auto depth = 0.76f + 0.24f * std::sin(angle * 2.0f + rotationPhase * 0.5f);
        points[i] = { centre.x + std::cos(angle) * radial,
                      centre.y + std::sin(angle) * radial * depth };
    }

    juce::Path waveform;
    waveform.startNewSubPath(points.front());
    for (std::size_t i = 1; i < points.size(); ++i)
        waveform.lineTo(points[i]);
    waveform.closeSubPath();

    g.setColour(purple.withAlpha(0.18f));
    g.strokePath(waveform, juce::PathStrokeType(9.0f));
    g.setColour(cyan.withAlpha(0.22f + energy * 0.30f));
    g.strokePath(waveform, juce::PathStrokeType(3.0f));
    g.setColour(juce::Colours::white.withAlpha(0.65f));
    g.strokePath(waveform, juce::PathStrokeType(1.0f));

    for (std::size_t i = 0; i < points.size(); ++i)
    {
        const auto amplitude = std::abs(visualState.amplitudes[i]);
        const auto nodeRadius = 4.0f + amplitude * 5.0f + energy * 2.0f;
        const auto nodeColour = (i % 3 == 0 ? gold : purple).interpolatedWith(cyan, amplitude * 0.35f);

        g.setColour(nodeColour.withAlpha(0.16f));
        g.fillEllipse(points[i].x - nodeRadius * 2.2f, points[i].y - nodeRadius * 2.2f,
                      nodeRadius * 4.4f, nodeRadius * 4.4f);
        g.setColour(nodeColour);
        g.fillEllipse(points[i].x - nodeRadius, points[i].y - nodeRadius,
                      nodeRadius * 2.0f, nodeRadius * 2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.fillEllipse(points[i].x - 1.6f, points[i].y - 1.6f, 3.2f, 3.2f);
    }

    // Small deterministic particle field: it follows engine energy rather than
    // pretending to be audio data. The actual polygon and nodes above are the DSP state.
    for (int i = 0; i < 80; ++i)
    {
        const auto phase = static_cast<float>(i) * 2.399963f + rotationPhase * (0.25f + (i % 5) * 0.05f);
        const auto spread = radius * (0.18f + static_cast<float>((i * 37) % 100) / 130.0f);
        const auto x = centre.x + std::cos(phase) * spread;
        const auto y = centre.y + std::sin(phase * 0.83f) * spread * 0.78f;
        const auto alpha = 0.04f + energy * 0.20f + static_cast<float>(i % 7) * 0.01f;
        g.setColour((i % 4 == 0 ? gold : purple).withAlpha(juce::jlimit(0.02f, 0.35f, alpha)));
        g.fillEllipse(x, y, 1.2f + energy * 1.8f, 1.2f + energy * 1.8f);
    }

    g.setColour(juce::Colours::white.withAlpha(0.72f));
    g.setFont(11.0f);
    g.drawText(juce::String(points.size()) + " GENDYN NODES  //  "
               + juce::String(visualState.activeVoices) + " VOICES",
               static_cast<int>(globe.getX()), static_cast<int>(globe.getBottom() - 10.0f),
               static_cast<int>(globe.getWidth()), 20, juce::Justification::centred);
}

void VeloriaAudioProcessorEditor::resized()
{
    title.setBounds(330, 16, 520, 48);
    subtitle.setBounds(365, 57, 450, 16);

    presetBox.setBounds(22, 24, 220, 32);
    discoverButton.setBounds(950, 22, 110, 34);
    monoButton.setBounds(1070, 22, 86, 34);

    ampWalk.setBounds(62, 190, 150, 150);
    timeWalk.setBounds(62, 430, 150, 150);
    ampMirror.setBounds(968, 190, 150, 150);
    timeMirror.setBounds(968, 430, 150, 150);
    level.setBounds(997, 602, 94, 108);

    voiceStatus.setBounds(450, 677, 280, 18);
    midiHint.setBounds(280, 715, 620, 20);
}
