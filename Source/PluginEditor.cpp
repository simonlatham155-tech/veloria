#include "PluginEditor.h"
#include <cmath>

namespace
{
const auto background = juce::Colour::fromRGB(4, 5, 9);
const auto panel = juce::Colour::fromRGB(10, 12, 18);
const auto panel2 = juce::Colour::fromRGB(15, 16, 24);
const auto purple = juce::Colour::fromRGB(181, 83, 255);
const auto violet = juce::Colour::fromRGB(111, 73, 255);
const auto magenta = juce::Colour::fromRGB(255, 76, 191);
const auto gold = juce::Colour::fromRGB(255, 180, 82);
const auto orange = juce::Colour::fromRGB(255, 112, 56);
const auto cyan = juce::Colour::fromRGB(103, 224, 255);
}

void VeloriaAudioProcessorEditor::AuroraLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
    juce::Slider&)
{
    auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                          static_cast<float>(width), static_cast<float>(height)).reduced(9.0f);
    const auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    auto dial = bounds.withSizeKeepingCentre(size, size);
    const auto centre = dial.getCentre();
    const auto radius = size * 0.43f;
    const auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    juce::ColourGradient outerGlow(purple.withAlpha(0.30f), centre.x, centre.y,
                                    juce::Colours::transparentBlack, centre.x + radius * 1.28f, centre.y, true);
    outerGlow.addColour(0.42, magenta.withAlpha(0.20f));
    outerGlow.addColour(0.72, orange.withAlpha(0.12f));
    g.setGradientFill(outerGlow);
    g.fillEllipse(dial.expanded(9.0f));

    g.setColour(juce::Colours::black.withAlpha(0.92f));
    g.fillEllipse(dial);
    g.setColour(juce::Colours::white.withAlpha(0.12f));
    g.drawEllipse(dial, 1.0f);

    const auto arcBounds = dial.reduced(4.0f);
    juce::Path baseArc;
    baseArc.addCentredArc(centre.x, centre.y, arcBounds.getWidth() * 0.5f, arcBounds.getHeight() * 0.5f,
                         0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.strokePath(baseArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    juce::Path valueArc;
    valueArc.addCentredArc(centre.x, centre.y, arcBounds.getWidth() * 0.5f, arcBounds.getHeight() * 0.5f,
                          0.0f, rotaryStartAngle, angle, true);
    juce::ColourGradient arcGradient(purple, dial.getX(), centre.y, gold, dial.getRight(), centre.y, false);
    arcGradient.addColour(0.5, magenta);
    g.setGradientFill(arcGradient);
    g.strokePath(valueArc, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

    juce::Path pointer;
    pointer.addRoundedRectangle(-1.3f, -radius * 0.72f, 2.6f, radius * 0.43f, 1.2f);
    g.setColour(juce::Colours::white.withAlpha(0.95f));
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));

    const auto sparkleCount = 16;
    for (int i = 0; i < sparkleCount; ++i)
    {
        const auto a = static_cast<float>(i) / sparkleCount * juce::MathConstants<float>::twoPi;
        const auto rr = radius * (1.04f + 0.11f * std::sin(a * 3.0f + sliderPosProportional * 5.0f));
        const auto px = centre.x + std::cos(a) * rr;
        const auto py = centre.y + std::sin(a) * rr;
        g.setColour((i % 3 == 0 ? gold : purple).withAlpha(0.10f + sliderPosProportional * 0.16f));
        g.fillEllipse(px - 1.0f, py - 1.0f, 2.0f, 2.0f);
    }
}

VeloriaAudioProcessorEditor::VeloriaAudioProcessorEditor(VeloriaAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(1500, 940);

    brand.setText("L A T H A M   A U D I O", juce::dontSendNotification);
    brand.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.78f));
    brand.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    addAndMakeVisible(brand);

    title.setText("V E L O R I A", juce::dontSendNotification);
    title.setJustificationType(juce::Justification::centred);
    title.setFont(juce::FontOptions(36.0f, juce::Font::bold));
    addAndMakeVisible(title);

    subtitle.setText("FLAGSHIP GENDYN INSTRUMENT  //  ALTERNATE-EARTH STOCHASTIC SYNTHESIS", juce::dontSendNotification);
    subtitle.setJustificationType(juce::Justification::centred);
    subtitle.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.42f));
    subtitle.setFont(juce::FontOptions(10.0f));
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
    presetBox.setColour(juce::ComboBox::backgroundColourId, panel2);
    presetBox.setColour(juce::ComboBox::outlineColourId, purple.withAlpha(0.38f));
    presetBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    addAndMakeVisible(presetBox);

    discoverButton.onClick = [this]
    {
        audioProcessor.discover();
        presetBox.setText("Discovered", juce::dontSendNotification);
    };
    discoverButton.setColour(juce::TextButton::buttonColourId, purple.withAlpha(0.30f));
    discoverButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(discoverButton);

    newFieldButton.onClick = [this]
    {
        seed.setValue(1 + uiRandom.nextInt(999998), juce::sendNotificationSync);
        presetBox.setText("Field Variation", juce::dontSendNotification);
    };
    newFieldButton.setColour(juce::TextButton::buttonColourId, orange.withAlpha(0.24f));
    newFieldButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(newFieldButton);

    monoButton.setClickingTogglesState(true);
    monoButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    monoButton.setColour(juce::ToggleButton::tickColourId, cyan);
    addAndMakeVisible(monoButton);

    for (auto* slider : { &ampWalk, &timeWalk, &ampMirror, &timeMirror,
                           &attack, &decay, &sustain, &release, &seed, &level })
    {
        configureKnob(*slider);
        slider->setLookAndFeel(&auroraLookAndFeel);
        addAndMakeVisible(slider);
    }

    seed.setTextValueSuffix(" seed");
    seed.setNumDecimalPlacesToDisplay(0);
    level.setTextValueSuffix(" dB");

    ampWalkAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "ampWalk", ampWalk);
    timeWalkAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "timeWalk", timeWalk);
    ampMirrorAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "ampMirror", ampMirror);
    timeMirrorAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "timeMirror", timeMirror);
    attackAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "attack", attack);
    decayAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "decay", decay);
    sustainAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "sustain", sustain);
    releaseAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "release", release);
    seedAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "seed", seed);
    levelAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "level", level);
    monoAttachment = std::make_unique<ButtonAttachment>(audioProcessor.parameters, "mono", monoButton);

    fieldStatus.setJustificationType(juce::Justification::centred);
    fieldStatus.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.62f));
    fieldStatus.setFont(juce::FontOptions(10.0f));
    addAndMakeVisible(fieldStatus);

    voiceStatus.setJustificationType(juce::Justification::centredLeft);
    voiceStatus.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.70f));
    voiceStatus.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    addAndMakeVisible(voiceStatus);

    footerStatus.setText("GENDYN CORE  //  12 BREAKPOINTS  //  HOST AUTOMATABLE  //  ABLETON MIDI MAP READY",
                         juce::dontSendNotification);
    footerStatus.setJustificationType(juce::Justification::centred);
    footerStatus.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.30f));
    footerStatus.setFont(juce::FontOptions(9.0f));
    addAndMakeVisible(footerStatus);

    visualState = audioProcessor.getVisualState();
    startTimerHz(30);
}

VeloriaAudioProcessorEditor::~VeloriaAudioProcessorEditor()
{
    for (auto* slider : { &ampWalk, &timeWalk, &ampMirror, &timeMirror,
                           &attack, &decay, &sustain, &release, &seed, &level })
        slider->setLookAndFeel(nullptr);
}

void VeloriaAudioProcessorEditor::configureKnob(juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 18);
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha(0.78f));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::white.withAlpha(0.08f));
}

void VeloriaAudioProcessorEditor::timerCallback()
{
    visualState = audioProcessor.getVisualState();
    rotationPhase += 0.0025f + visualState.energy * 0.008f;
    if (rotationPhase > juce::MathConstants<float>::twoPi)
        rotationPhase -= juce::MathConstants<float>::twoPi;

    voiceStatus.setText("VOICE ENGINE   " + juce::String(visualState.activeVoices) + " / 8 ACTIVE",
                        juce::dontSendNotification);
    fieldStatus.setText(juce::String(visualState.activeVoices) + " VOICES  //  "
                        + juce::String(static_cast<int>(visualState.energy * 100.0f)) + "% FIELD ENERGY",
                        juce::dontSendNotification);
    repaint();
}

void VeloriaAudioProcessorEditor::drawPanel(juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& panelTitle)
{
    g.setColour(panel.withAlpha(0.92f));
    g.fillRoundedRectangle(bounds, 10.0f);
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawRoundedRectangle(bounds, 10.0f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.72f));
    g.setFont(11.0f);
    g.drawText(panelTitle, bounds.toNearestInt().reduced(14, 8).removeFromTop(18), juce::Justification::centredLeft);
}

void VeloriaAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);

    juce::ColourGradient bgGlow(purple.withAlpha(0.10f), getWidth() * 0.48f, getHeight() * 0.36f,
                                background, getWidth() * 0.48f, static_cast<float>(getHeight()), true);
    g.setGradientFill(bgGlow);
    g.fillRect(getLocalBounds());

    g.setColour(juce::Colours::white.withAlpha(0.07f));
    g.drawRoundedRectangle(getLocalBounds().reduced(8).toFloat(), 15.0f, 1.0f);
    g.setColour(purple.withAlpha(0.30f));
    g.drawLine(18.0f, 74.0f, getWidth() - 18.0f, 74.0f, 1.0f);

    drawPanel(g, { 18.0f, 94.0f, 230.0f, 565.0f }, "GENDYN FIELD");
    drawPanel(g, { 1260.0f, 94.0f, 222.0f, 285.0f }, "EVOLUTION");
    drawPanel(g, { 1260.0f, 392.0f, 222.0f, 267.0f }, "VOICE / STATE");
    drawPanel(g, { 18.0f, 676.0f, 460.0f, 220.0f }, "AMPLITUDE ENVELOPE");
    drawPanel(g, { 492.0f, 676.0f, 300.0f, 220.0f }, "FIELD CONTROLS");
    drawPanel(g, { 806.0f, 676.0f, 440.0f, 220.0f }, "FIELD TELEMETRY");
    drawPanel(g, { 1260.0f, 676.0f, 222.0f, 220.0f }, "OUTPUT");

    drawStochasticGlobe(g, { 270.0f, 102.0f, 970.0f, 555.0f });
    drawEvolutionGraph(g, { 1277.0f, 132.0f, 187.0f, 190.0f });

    drawKnobLabel(g, ampWalk, "AMP WALK");
    drawKnobLabel(g, timeWalk, "TIME WALK");
    drawKnobLabel(g, ampMirror, "AMP MIRROR");
    drawKnobLabel(g, timeMirror, "TIME MIRROR");
    drawKnobLabel(g, attack, "ATTACK");
    drawKnobLabel(g, decay, "DECAY");
    drawKnobLabel(g, sustain, "SUSTAIN");
    drawKnobLabel(g, release, "RELEASE");
    drawKnobLabel(g, seed, "FIELD SEED");
    drawKnobLabel(g, level, "LEVEL");

    g.setColour(juce::Colours::white.withAlpha(0.36f));
    g.setFont(10.0f);
    g.drawText("LIVE 12-NODE STOCHASTIC AURORA", 530, 625, 450, 18, juce::Justification::centred);

    g.setColour(cyan.withAlpha(0.65f));
    g.drawText("POLY 8", 1280, 446, 78, 18, juce::Justification::centredLeft);
    g.setColour(juce::Colours::white.withAlpha(0.48f));
    g.drawText("Independent GENDYN state per voice", 1280, 478, 175, 35, juce::Justification::centredLeft, true);
    g.drawText("DISCOVER changes the bounded field", 1280, 526, 175, 34, juce::Justification::centredLeft, true);
    g.drawText("NEW FIELD keeps settings and changes seed", 1280, 566, 175, 42, juce::Justification::centredLeft, true);

    g.setColour(juce::Colours::white.withAlpha(0.42f));
    g.drawText("The aurora follows live breakpoint amplitude and timing.", 825, 732, 395, 22,
               juce::Justification::centredLeft);
    g.drawText("Purple / magenta = stable field; gold = energetic motion.", 825, 760, 395, 22,
               juce::Justification::centredLeft);
    g.drawText("Every visible node is a real GENDYN breakpoint.", 825, 788, 395, 22,
               juce::Justification::centredLeft);
}

void VeloriaAudioProcessorEditor::drawKnobLabel(juce::Graphics& g, juce::Slider& slider, const juce::String& text)
{
    auto labelBounds = slider.getBounds().translated(0, -21);
    g.setColour(juce::Colours::white.withAlpha(0.78f));
    g.setFont(10.0f);
    g.drawFittedText(text, labelBounds, juce::Justification::centred, 1);
}

void VeloriaAudioProcessorEditor::drawEvolutionGraph(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    for (int i = 1; i < 5; ++i)
    {
        const auto x = bounds.getX() + bounds.getWidth() * static_cast<float>(i) / 5.0f;
        g.drawVerticalLine(static_cast<int>(x), bounds.getY(), bounds.getBottom());
    }

    juce::Path path;
    for (std::size_t i = 0; i < visualState.amplitudes.size(); ++i)
    {
        const auto x = bounds.getX() + bounds.getWidth() * static_cast<float>(i)
                                     / static_cast<float>(visualState.amplitudes.size() - 1);
        const auto y = bounds.getCentreY() - visualState.amplitudes[i] * bounds.getHeight() * 0.36f;
        if (i == 0) path.startNewSubPath(x, y); else path.lineTo(x, y);
    }

    g.setColour(purple.withAlpha(0.20f));
    g.strokePath(path, juce::PathStrokeType(8.0f));
    g.setColour(purple);
    g.strokePath(path, juce::PathStrokeType(2.0f));

    for (std::size_t i = 0; i < visualState.amplitudes.size(); ++i)
    {
        const auto x = bounds.getX() + bounds.getWidth() * static_cast<float>(i)
                                     / static_cast<float>(visualState.amplitudes.size() - 1);
        const auto y = bounds.getCentreY() - visualState.amplitudes[i] * bounds.getHeight() * 0.36f;
        g.setColour(i % 3 == 0 ? gold : cyan);
        g.fillEllipse(x - 2.4f, y - 2.4f, 4.8f, 4.8f);
    }
}

void VeloriaAudioProcessorEditor::drawStochasticGlobe(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    auto globe = bounds.withSizeKeepingCentre(535.0f, 535.0f);
    const auto centre = globe.getCentre();
    const auto radius = globe.getWidth() * 0.43f;
    const auto energy = juce::jlimit(0.0f, 1.0f, visualState.energy);

    juce::ColourGradient glow(purple.withAlpha(0.10f + energy * 0.22f), centre.x, centre.y,
                              juce::Colours::transparentBlack, centre.x, centre.y + radius, true);
    glow.addColour(0.36, magenta.withAlpha(0.08f + energy * 0.14f));
    glow.addColour(0.68, orange.withAlpha(0.04f + energy * 0.08f));
    g.setGradientFill(glow);
    g.fillEllipse(globe.reduced(8.0f));

    for (int ring = 1; ring <= 5; ++ring)
    {
        const auto r = radius * static_cast<float>(ring) / 5.0f;
        g.setColour((ring % 2 == 0 ? gold : purple).withAlpha(0.05f + energy * 0.025f));
        g.drawEllipse(centre.x - r, centre.y - r, r * 2.0f, r * 2.0f, 1.0f);
    }

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
        const auto radial = radius * (0.61f + amplitude * 0.30f);
        const auto depth = 0.76f + 0.24f * std::sin(angle * 2.0f + rotationPhase * 0.5f);
        points[i] = { centre.x + std::cos(angle) * radial,
                      centre.y + std::sin(angle) * radial * depth };
    }

    // Aurora ribbons interpolate between the real GENDYN breakpoints.
    for (int ribbon = 0; ribbon < 9; ++ribbon)
    {
        juce::Path aurora;
        const auto phaseOffset = ribbon * 0.23f;
        for (std::size_t segment = 0; segment < points.size(); ++segment)
        {
            const auto next = (segment + 1) % points.size();
            const auto p0 = points[segment];
            const auto p1 = points[next];
            for (int s = 0; s <= 14; ++s)
            {
                const auto t = static_cast<float>(s) / 14.0f;
                auto p = p0 + (p1 - p0) * t;
                const auto flutter = std::sin(t * juce::MathConstants<float>::pi
                                            + phaseOffset + rotationPhase * (0.7f + ribbon * 0.05f));
                const auto tangent = p1 - p0;
                auto normal = juce::Point<float>(-tangent.y, tangent.x);
                const auto length = juce::jmax(1.0f, normal.getDistanceFromOrigin());
                normal /= length;
                p += normal * flutter * (5.0f + ribbon * 1.3f) * (0.45f + energy);
                if (segment == 0 && s == 0) aurora.startNewSubPath(p); else aurora.lineTo(p);
            }
        }
        aurora.closeSubPath();

        const auto baseColour = ribbon % 3 == 0 ? gold : (ribbon % 3 == 1 ? magenta : purple);
        g.setColour(baseColour.withAlpha(0.025f + energy * 0.035f));
        g.strokePath(aurora, juce::PathStrokeType(14.0f - ribbon * 0.6f,
                                                  juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
        g.setColour(baseColour.withAlpha(0.10f + energy * 0.09f));
        g.strokePath(aurora, juce::PathStrokeType(2.3f,
                                                  juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }

    // Particle traces run along interpolated segments, not arbitrary decoration.
    for (std::size_t segment = 0; segment < points.size(); ++segment)
    {
        const auto next = (segment + 1) % points.size();
        const auto p0 = points[segment];
        const auto p1 = points[next];
        for (int j = 0; j < 18; ++j)
        {
            const auto t = std::fmod((static_cast<float>(j) / 18.0f)
                                   + rotationPhase * (0.025f + 0.004f * static_cast<float>(segment)), 1.0f);
            auto p = p0 + (p1 - p0) * t;
            const auto tangent = p1 - p0;
            auto normal = juce::Point<float>(-tangent.y, tangent.x);
            const auto length = juce::jmax(1.0f, normal.getDistanceFromOrigin());
            normal /= length;
            const auto wobble = std::sin((t * 8.0f + segment) * 1.7f + rotationPhase * 3.0f);
            p += normal * wobble * (3.0f + energy * 10.0f);
            const auto particleColour = ((j + static_cast<int>(segment)) % 4 == 0 ? gold : purple)
                                      .interpolatedWith(magenta, 0.35f);
            g.setColour(particleColour.withAlpha(0.10f + energy * 0.30f));
            const auto sz = 0.8f + energy * 1.7f + static_cast<float>(j % 3) * 0.35f;
            g.fillEllipse(p.x - sz * 0.5f, p.y - sz * 0.5f, sz, sz);
        }
    }

    for (std::size_t i = 0; i < points.size(); ++i)
    {
        const auto amplitude = std::abs(visualState.amplitudes[i]);
        const auto nodeRadius = 4.0f + amplitude * 5.0f + energy * 2.0f;
        const auto nodeColour = (i % 3 == 0 ? gold : purple).interpolatedWith(magenta, amplitude * 0.30f);
        g.setColour(nodeColour.withAlpha(0.15f));
        g.fillEllipse(points[i].x - nodeRadius * 2.7f, points[i].y - nodeRadius * 2.7f,
                      nodeRadius * 5.4f, nodeRadius * 5.4f);
        g.setColour(nodeColour);
        g.fillEllipse(points[i].x - nodeRadius, points[i].y - nodeRadius,
                      nodeRadius * 2.0f, nodeRadius * 2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.90f));
        g.fillEllipse(points[i].x - 1.6f, points[i].y - 1.6f, 3.2f, 3.2f);
    }
}

void VeloriaAudioProcessorEditor::resized()
{
    brand.setBounds(34, 20, 250, 30);
    title.setBounds(490, 10, 520, 45);
    subtitle.setBounds(475, 50, 550, 16);

    presetBox.setBounds(1020, 20, 220, 34);
    discoverButton.setBounds(1250, 20, 92, 34);
    newFieldButton.setBounds(1350, 20, 92, 34);
    monoButton.setBounds(1445, 20, 48, 34);

    ampWalk.setBounds(60, 160, 145, 135);
    timeWalk.setBounds(60, 338, 145, 135);
    ampMirror.setBounds(60, 516, 145, 125);
    timeMirror.setBounds(1268, 420, 145, 125);

    attack.setBounds(48, 736, 96, 125);
    decay.setBounds(151, 736, 96, 125);
    sustain.setBounds(254, 736, 96, 125);
    release.setBounds(357, 736, 96, 125);

    seed.setBounds(530, 736, 105, 125);
    fieldStatus.setBounds(645, 754, 120, 55);
    voiceStatus.setBounds(1280, 412, 170, 24);
    level.setBounds(1303, 735, 135, 135);
    footerStatus.setBounds(430, 908, 650, 18);
}
