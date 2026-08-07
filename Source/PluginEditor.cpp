#include "PluginEditor.h"
#include <cmath>

namespace
{
const auto background = juce::Colour::fromRGB(3, 4, 8);
const auto panel = juce::Colour::fromRGB(9, 10, 16);
const auto panel2 = juce::Colour::fromRGB(14, 15, 23);
const auto purple = juce::Colour::fromRGB(176, 77, 255);
const auto magenta = juce::Colour::fromRGB(255, 72, 190);
const auto gold = juce::Colour::fromRGB(255, 184, 86);
const auto orange = juce::Colour::fromRGB(255, 107, 48);
const auto cyan = juce::Colour::fromRGB(111, 226, 255);
}

void VeloriaAudioProcessorEditor::AuroraLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
    juce::Slider&)
{
    auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                          static_cast<float>(width), static_cast<float>(height)).reduced(7.0f);
    const auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    auto dial = bounds.withSizeKeepingCentre(size, size);
    const auto centre = dial.getCentre();
    const auto radius = size * 0.43f;
    const auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    juce::ColourGradient glow(purple.withAlpha(0.30f), centre.x, centre.y,
                               juce::Colours::transparentBlack, centre.x + radius * 1.35f, centre.y, true);
    glow.addColour(0.46, magenta.withAlpha(0.20f));
    glow.addColour(0.78, orange.withAlpha(0.10f));
    g.setGradientFill(glow);
    g.fillEllipse(dial.expanded(8.0f));

    juce::ColourGradient metal(juce::Colour::fromRGB(33, 34, 43), dial.getX(), dial.getY(),
                                juce::Colour::fromRGB(6, 7, 11), dial.getRight(), dial.getBottom(), false);
    g.setGradientFill(metal);
    g.fillEllipse(dial);
    g.setColour(juce::Colours::white.withAlpha(0.10f));
    g.drawEllipse(dial, 1.0f);

    const auto arcBounds = dial.reduced(4.0f);
    juce::Path baseArc;
    baseArc.addCentredArc(centre.x, centre.y, arcBounds.getWidth() * 0.5f, arcBounds.getHeight() * 0.5f,
                         0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.strokePath(baseArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    juce::Path valueArc;
    valueArc.addCentredArc(centre.x, centre.y, arcBounds.getWidth() * 0.5f, arcBounds.getHeight() * 0.5f,
                          0.0f, rotaryStartAngle, angle, true);
    juce::ColourGradient arcGradient(purple, dial.getX(), centre.y, gold, dial.getRight(), centre.y, false);
    arcGradient.addColour(0.5, magenta);
    g.setGradientFill(arcGradient);
    g.strokePath(valueArc, juce::PathStrokeType(4.5f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

    juce::Path pointer;
    pointer.addRoundedRectangle(-1.2f, -radius * 0.72f, 2.4f, radius * 0.44f, 1.2f);
    g.setColour(juce::Colours::white.withAlpha(0.94f));
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
}

void VeloriaAudioProcessorEditor::MidiLearnSlider::mouseDown(const juce::MouseEvent& event)
{
    if (! event.mods.isPopupMenu())
    {
        juce::Slider::mouseDown(event);
        return;
    }

    const auto currentCC = currentCCCallback ? currentCCCallback() : -1;
    juce::PopupMenu menu;
    menu.addItem(1, currentCC >= 0
        ? "Relearn MIDI CC (currently CC " + juce::String(currentCC) + ")"
        : "Learn MIDI CC");
    menu.addItem(2, "Clear MIDI CC", currentCC >= 0);

    juce::Component::SafePointer<MidiLearnSlider> safeThis(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
                       [safeThis](int result)
                       {
                           if (safeThis == nullptr)
                               return;
                           if (result == 1 && safeThis->learnCallback)
                               safeThis->learnCallback();
                           else if (result == 2 && safeThis->clearCallback)
                               safeThis->clearCallback();
                       });
}

VeloriaAudioProcessorEditor::VeloriaAudioProcessorEditor(VeloriaAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(1160, 720);

    brand.setText("L A T H A M   A U D I O", juce::dontSendNotification);
    brand.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.68f));
    brand.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    addAndMakeVisible(brand);

    title.setText("V E L O R I A", juce::dontSendNotification);
    title.setJustificationType(juce::Justification::centred);
    title.setFont(juce::FontOptions(30.0f, juce::Font::bold));
    addAndMakeVisible(title);

    subtitle.setText("STOCHASTIC SYNTHESIZER", juce::dontSendNotification);
    subtitle.setJustificationType(juce::Justification::centred);
    subtitle.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.35f));
    subtitle.setFont(juce::FontOptions(9.0f));
    addAndMakeVisible(subtitle);

    presetBox.setColour(juce::ComboBox::backgroundColourId, panel2);
    presetBox.setColour(juce::ComboBox::outlineColourId, purple.withAlpha(0.38f));
    presetBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    presetBox.onChange = [this]
    {
        const auto id = presetBox.getSelectedId();
        const auto factoryCount = audioProcessor.getFactoryPresetNames().size();

        if (id > 0 && id <= factoryCount)
        {
            audioProcessor.setCurrentProgram(id - 1);
            presetNameEditor.setText(presetBox.getText() + " Copy", juce::dontSendNotification);
            presetStatus.setText("FACTORY PRESET - SAVE A USER COPY TO RENAME", juce::dontSendNotification);
        }
        else if (id >= firstUserPresetId)
        {
            const auto name = presetBox.getText();
            if (audioProcessor.loadUserPreset(name))
            {
                presetNameEditor.setText(name, juce::dontSendNotification);
                presetStatus.setText("USER PRESET LOADED", juce::dontSendNotification);
            }
        }
    };
    addAndMakeVisible(presetBox);
    refreshPresetBox();

    discoverButton.onClick = [this]
    {
        audioProcessor.discover();
        presetBox.setText("Discovered", juce::dontSendNotification);
        presetNameEditor.setText({}, juce::dontSendNotification);
        presetStatus.setText("DISCOVERED FIELD - NAME IT AND PRESS SAVE", juce::dontSendNotification);
    };

    newFieldButton.onClick = [this]
    {
        audioProcessor.newField();
        presetBox.setText("Field Variation", juce::dontSendNotification);
        presetStatus.setText("NEW FIELD - SAME SETTINGS, NEW STOCHASTIC SEED", juce::dontSendNotification);
    };

    for (auto* button : { &discoverButton, &newFieldButton, &savePresetButton, &renamePresetButton })
    {
        button->setColour(juce::TextButton::buttonColourId, purple.withAlpha(0.24f));
        button->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        addAndMakeVisible(button);
    }
    newFieldButton.setColour(juce::TextButton::buttonColourId, orange.withAlpha(0.22f));
    deletePresetButton.setColour(juce::TextButton::buttonColourId, orange.withAlpha(0.17f));
    deletePresetButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(deletePresetButton);

    monoButton.setClickingTogglesState(true);
    monoButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    monoButton.setColour(juce::ToggleButton::tickColourId, cyan);
    addAndMakeVisible(monoButton);

    configureKnob(ampWalk, "ampWalk", "AMP WALK");
    configureKnob(timeWalk, "timeWalk", "TIME WALK");
    configureKnob(ampMirror, "ampMirror", "AMP MIRROR");
    configureKnob(timeMirror, "timeMirror", "TIME MIRROR");
    configureKnob(attack, "attack", "ATTACK");
    configureKnob(decay, "decay", "DECAY");
    configureKnob(sustain, "sustain", "SUSTAIN");
    configureKnob(release, "release", "RELEASE");
    configureKnob(seed, "seed", "FIELD SEED");
    configureKnob(level, "level", "LEVEL");

    for (auto* slider : { &ampWalk, &timeWalk, &ampMirror, &timeMirror,
                           &attack, &decay, &sustain, &release, &seed, &level })
    {
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

    presetNameEditor.setTextToShowWhenEmpty("Name preset...", juce::Colours::white.withAlpha(0.28f));
    presetNameEditor.setColour(juce::TextEditor::backgroundColourId, panel2);
    presetNameEditor.setColour(juce::TextEditor::outlineColourId, purple.withAlpha(0.30f));
    presetNameEditor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    addAndMakeVisible(presetNameEditor);

    savePresetButton.onClick = [this]
    {
        const auto name = presetNameEditor.getText().trim();
        if (audioProcessor.saveUserPreset(name))
        {
            refreshPresetBox(name);
            presetStatus.setText("USER PRESET SAVED", juce::dontSendNotification);
        }
        else
            presetStatus.setText("ENTER A VALID PRESET NAME", juce::dontSendNotification);
    };

    renamePresetButton.onClick = [this]
    {
        if (! selectedPresetIsUser())
        {
            presetStatus.setText("FACTORY PRESETS ARE READ-ONLY - SAVE A USER COPY FIRST", juce::dontSendNotification);
            return;
        }

        const auto oldName = presetBox.getText();
        const auto newName = presetNameEditor.getText().trim();
        if (audioProcessor.renameUserPreset(oldName, newName))
        {
            refreshPresetBox(newName);
            presetStatus.setText("USER PRESET RENAMED", juce::dontSendNotification);
        }
        else
            presetStatus.setText("RENAME FAILED - NAME MAY ALREADY EXIST", juce::dontSendNotification);
    };

    deletePresetButton.onClick = [this]
    {
        if (! selectedPresetIsUser())
        {
            presetStatus.setText("FACTORY PRESETS CANNOT BE DELETED", juce::dontSendNotification);
            return;
        }

        const auto name = presetBox.getText();
        if (audioProcessor.deleteUserPreset(name))
        {
            refreshPresetBox();
            presetNameEditor.clear();
            presetStatus.setText("USER PRESET DELETED", juce::dontSendNotification);
        }
    };

    fieldStatus.setJustificationType(juce::Justification::centred);
    fieldStatus.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.55f));
    fieldStatus.setFont(juce::FontOptions(9.0f));
    addAndMakeVisible(fieldStatus);

    voiceStatus.setJustificationType(juce::Justification::centredLeft);
    voiceStatus.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.62f));
    voiceStatus.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    addAndMakeVisible(voiceStatus);

    presetStatus.setJustificationType(juce::Justification::centredLeft);
    presetStatus.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.40f));
    presetStatus.setFont(juce::FontOptions(8.5f));
    presetStatus.setText("RIGHT-CLICK ANY KNOB FOR MIDI LEARN", juce::dontSendNotification);
    addAndMakeVisible(presetStatus);

    footerStatus.setText("GENDYN CORE  //  12 BREAKPOINTS  //  USER PRESETS  //  MIDI LEARN",
                         juce::dontSendNotification);
    footerStatus.setJustificationType(juce::Justification::centred);
    footerStatus.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.25f));
    footerStatus.setFont(juce::FontOptions(8.0f));
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

void VeloriaAudioProcessorEditor::configureKnob(MidiLearnSlider& slider,
                                                 const juce::String& parameterId,
                                                 const juce::String& displayName)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 16);
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha(0.70f));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::white.withAlpha(0.06f));

    slider.learnCallback = [this, parameterId, displayName]
    {
        audioProcessor.beginMidiLearn(parameterId);
        presetStatus.setText(displayName + " - MOVE A MIDI CC NOW", juce::dontSendNotification);
    };
    slider.clearCallback = [this, parameterId, displayName]
    {
        audioProcessor.clearMidiMapping(parameterId);
        presetStatus.setText(displayName + " - MIDI MAPPING CLEARED", juce::dontSendNotification);
    };
    slider.currentCCCallback = [this, parameterId]
    {
        return audioProcessor.getMidiCCForParameter(parameterId);
    };
}

void VeloriaAudioProcessorEditor::refreshPresetBox(const juce::String& selectUserPreset)
{
    presetBox.clear(juce::dontSendNotification);
    presetBox.addSectionHeading("FACTORY");

    const auto factoryNames = audioProcessor.getFactoryPresetNames();
    for (int i = 0; i < factoryNames.size(); ++i)
        presetBox.addItem(factoryNames[i], i + 1);

    const auto userNames = audioProcessor.getUserPresetNames();
    if (! userNames.isEmpty())
    {
        presetBox.addSeparator();
        presetBox.addSectionHeading("USER PRESETS");
        for (int i = 0; i < userNames.size(); ++i)
            presetBox.addItem(userNames[i], firstUserPresetId + i);
    }

    if (selectUserPreset.isNotEmpty())
    {
        const auto index = userNames.indexOf(selectUserPreset);
        if (index >= 0)
            presetBox.setSelectedId(firstUserPresetId + index, juce::dontSendNotification);
    }
    else
    {
        const auto program = audioProcessor.getCurrentProgram();
        if (program >= 0 && program < factoryNames.size())
            presetBox.setSelectedId(program + 1, juce::dontSendNotification);
    }
}

bool VeloriaAudioProcessorEditor::selectedPresetIsUser() const noexcept
{
    return presetBox.getSelectedId() >= firstUserPresetId;
}

void VeloriaAudioProcessorEditor::timerCallback()
{
    visualState = audioProcessor.getVisualState();
    rotationPhase += 0.0020f + visualState.energy * 0.0065f;
    if (rotationPhase > juce::MathConstants<float>::twoPi)
        rotationPhase -= juce::MathConstants<float>::twoPi;

    voiceStatus.setText("VOICE ENGINE   " + juce::String(visualState.activeVoices) + " / 8 ACTIVE",
                        juce::dontSendNotification);
    fieldStatus.setText(juce::String(visualState.activeVoices) + " VOICES  //  "
                        + juce::String(static_cast<int>(visualState.energy * 100.0f)) + "% ENERGY",
                        juce::dontSendNotification);
    repaint();
}

void VeloriaAudioProcessorEditor::drawPanel(juce::Graphics& g, juce::Rectangle<float> bounds,
                                             const juce::String& panelTitle)
{
    g.setColour(panel.withAlpha(0.90f));
    g.fillRoundedRectangle(bounds, 9.0f);
    g.setColour(juce::Colours::white.withAlpha(0.07f));
    g.drawRoundedRectangle(bounds, 9.0f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.62f));
    g.setFont(9.5f);
    g.drawText(panelTitle, bounds.toNearestInt().reduced(10, 7).removeFromTop(15), juce::Justification::centredLeft);
}

void VeloriaAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);

    juce::ColourGradient bgGlow(purple.withAlpha(0.09f), getWidth() * 0.52f, getHeight() * 0.34f,
                                background, getWidth() * 0.52f, static_cast<float>(getHeight()), true);
    bgGlow.addColour(0.54, magenta.withAlpha(0.035f));
    g.setGradientFill(bgGlow);
    g.fillRect(getLocalBounds());

    g.setColour(juce::Colours::white.withAlpha(0.06f));
    g.drawRoundedRectangle(getLocalBounds().reduced(6).toFloat(), 13.0f, 1.0f);
    g.setColour(purple.withAlpha(0.28f));
    g.drawLine(14.0f, 62.0f, getWidth() - 14.0f, 62.0f, 1.0f);

    drawPanel(g, { 14.0f, 74.0f, 190.0f, 438.0f }, "GENDYN FIELD");
    drawPanel(g, { 214.0f, 74.0f, 700.0f, 438.0f }, "STOCHASTIC FIELD VIEW");
    drawPanel(g, { 924.0f, 74.0f, 222.0f, 212.0f }, "EVOLUTION");
    drawPanel(g, { 924.0f, 296.0f, 222.0f, 216.0f }, "VOICE / STATE");
    drawPanel(g, { 14.0f, 522.0f, 430.0f, 164.0f }, "AMPLITUDE ENVELOPE");
    drawPanel(g, { 454.0f, 522.0f, 468.0f, 164.0f }, "PRESETS / FIELD");
    drawPanel(g, { 932.0f, 522.0f, 214.0f, 164.0f }, "OUTPUT");

    drawStochasticGlobe(g, { 226.0f, 88.0f, 676.0f, 410.0f });
    drawEvolutionGraph(g, { 941.0f, 111.0f, 188.0f, 128.0f });

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

    g.setColour(juce::Colours::white.withAlpha(0.28f));
    g.setFont(8.5f);
    g.drawText("LIVE GENDYN AURORA - 12 STOCHASTIC BREAKPOINTS", 370, 484, 380, 16,
               juce::Justification::centred);

    g.setColour(cyan.withAlpha(0.58f));
    g.drawText("POLY 8", 944, 347, 70, 16, juce::Justification::centredLeft);
    g.setColour(juce::Colours::white.withAlpha(0.40f));
    g.drawText("Independent stochastic state per voice", 944, 372, 180, 32,
               juce::Justification::centredLeft, true);
    g.drawText("DISCOVER reshapes the field. NEW FIELD keeps the patch and changes only the seed.",
               944, 418, 180, 57, juce::Justification::centredLeft, true);
}

void VeloriaAudioProcessorEditor::drawKnobLabel(juce::Graphics& g, juce::Slider& slider,
                                                 const juce::String& text)
{
    auto labelBounds = slider.getBounds().translated(0, -15);
    g.setColour(juce::Colours::white.withAlpha(0.68f));
    g.setFont(8.5f);
    g.drawFittedText(text, labelBounds, juce::Justification::centred, 1);
}

void VeloriaAudioProcessorEditor::drawEvolutionGraph(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    juce::Path path;
    for (std::size_t i = 0; i < visualState.amplitudes.size(); ++i)
    {
        const auto x = bounds.getX() + bounds.getWidth() * static_cast<float>(i)
                                     / static_cast<float>(visualState.amplitudes.size() - 1);
        const auto y = bounds.getCentreY() - visualState.amplitudes[i] * bounds.getHeight() * 0.34f;
        if (i == 0) path.startNewSubPath(x, y); else path.lineTo(x, y);
    }

    g.setColour(purple.withAlpha(0.18f));
    g.strokePath(path, juce::PathStrokeType(7.0f, juce::PathStrokeType::curved));
    juce::ColourGradient graphGradient(purple, bounds.getX(), bounds.getCentreY(),
                                        gold, bounds.getRight(), bounds.getCentreY(), false);
    graphGradient.addColour(0.55, magenta);
    g.setGradientFill(graphGradient);
    g.strokePath(path, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved));

    for (std::size_t i = 0; i < visualState.amplitudes.size(); ++i)
    {
        const auto x = bounds.getX() + bounds.getWidth() * static_cast<float>(i)
                                     / static_cast<float>(visualState.amplitudes.size() - 1);
        const auto y = bounds.getCentreY() - visualState.amplitudes[i] * bounds.getHeight() * 0.34f;
        g.setColour(i % 3 == 0 ? gold : cyan);
        g.fillEllipse(x - 2.2f, y - 2.2f, 4.4f, 4.4f);
    }
}

void VeloriaAudioProcessorEditor::drawStochasticGlobe(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    auto globe = bounds.withSizeKeepingCentre(410.0f, 410.0f);
    const auto centre = globe.getCentre();
    const auto radius = globe.getWidth() * 0.46f;
    const auto energy = juce::jlimit(0.0f, 1.0f, visualState.energy);

    juce::ColourGradient halo(purple.withAlpha(0.20f + energy * 0.20f), centre.x, centre.y,
                               juce::Colours::transparentBlack, centre.x, centre.y + radius * 1.22f, true);
    halo.addColour(0.40, magenta.withAlpha(0.13f + energy * 0.12f));
    halo.addColour(0.70, orange.withAlpha(0.06f + energy * 0.08f));
    g.setGradientFill(halo);
    g.fillEllipse(globe.expanded(18.0f));

    // Deep spherical body: no radar rings, just haze and depth.
    juce::ColourGradient body(juce::Colour::fromRGB(28, 13, 48).withAlpha(0.78f),
                               centre.x - radius * 0.35f, centre.y - radius * 0.45f,
                               juce::Colour::fromRGB(2, 3, 8).withAlpha(0.96f),
                               centre.x + radius * 0.78f, centre.y + radius * 0.82f, true);
    body.addColour(0.48, juce::Colour::fromRGB(70, 17, 89).withAlpha(0.35f + energy * 0.15f));
    g.setGradientFill(body);
    g.fillEllipse(globe);

    // Deterministic volumetric particles inside the sphere.
    for (int i = 0; i < 260; ++i)
    {
        const auto fi = static_cast<float>(i);
        const auto phase = fi * 2.39996323f + rotationPhase * (0.20f + static_cast<float>(i % 9) * 0.025f);
        const auto radialNorm = std::sqrt(static_cast<float>((i * 53) % 257) / 257.0f);
        const auto r = radius * radialNorm * 0.94f;
        const auto squash = 0.72f + 0.24f * std::sin(fi * 0.37f + rotationPhase * 0.8f);
        const auto x = centre.x + std::cos(phase) * r;
        const auto y = centre.y + std::sin(phase) * r * squash;
        const auto sz = 0.55f + static_cast<float>(i % 4) * 0.28f + energy * 0.9f;
        const auto c = (i % 7 == 0 ? gold : (i % 3 == 0 ? magenta : purple));
        g.setColour(c.withAlpha(0.035f + energy * 0.12f));
        g.fillEllipse(x - sz * 0.5f, y - sz * 0.5f, sz, sz);
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
        const auto radial = radius * (0.52f + amplitude * 0.29f);
        const auto depth = 0.72f + 0.28f * std::sin(angle * 2.0f + rotationPhase * 0.55f);
        points[i] = { centre.x + std::cos(angle) * radial,
                      centre.y + std::sin(angle) * radial * depth };
    }

    // Broad northern-lights ribbons flow through the real breakpoint path.
    for (int ribbon = 0; ribbon < 14; ++ribbon)
    {
        juce::Path aurora;
        const auto ribbonOffset = static_cast<float>(ribbon - 7);

        for (std::size_t segment = 0; segment < points.size(); ++segment)
        {
            const auto next = (segment + 1) % points.size();
            const auto p0 = points[segment];
            const auto p1 = points[next];

            for (int s = 0; s <= 18; ++s)
            {
                const auto t = static_cast<float>(s) / 18.0f;
                auto p = p0 + (p1 - p0) * t;
                const auto tangent = p1 - p0;
                auto normal = juce::Point<float>(-tangent.y, tangent.x);
                const auto len = juce::jmax(1.0f, normal.getDistanceFromOrigin());
                normal /= len;

                const auto wave = std::sin(t * juce::MathConstants<float>::pi
                                         + static_cast<float>(segment) * 0.55f
                                         + rotationPhase * (0.65f + ribbon * 0.025f));
                p += normal * (ribbonOffset * 1.8f + wave * (4.0f + energy * 10.0f));

                if (segment == 0 && s == 0) aurora.startNewSubPath(p);
                else aurora.lineTo(p);
            }
        }
        aurora.closeSubPath();

        const auto c = ribbon % 4 == 0 ? gold : (ribbon % 3 == 0 ? magenta : purple);
        g.setColour(c.withAlpha(0.018f + energy * 0.025f));
        g.strokePath(aurora, juce::PathStrokeType(12.0f - ribbon * 0.22f,
                                                  juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
        g.setColour(c.withAlpha(0.055f + energy * 0.080f));
        g.strokePath(aurora, juce::PathStrokeType(1.6f,
                                                  juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }

    // Moving particles along the actual stochastic interpolation.
    for (std::size_t segment = 0; segment < points.size(); ++segment)
    {
        const auto next = (segment + 1) % points.size();
        const auto p0 = points[segment];
        const auto p1 = points[next];
        for (int j = 0; j < 22; ++j)
        {
            const auto t = std::fmod(static_cast<float>(j) / 22.0f
                                   + rotationPhase * (0.022f + 0.0025f * static_cast<float>(segment)), 1.0f);
            auto p = p0 + (p1 - p0) * t;
            const auto tangent = p1 - p0;
            auto normal = juce::Point<float>(-tangent.y, tangent.x);
            const auto len = juce::jmax(1.0f, normal.getDistanceFromOrigin());
            normal /= len;
            const auto wobble = std::sin(t * 11.0f + static_cast<float>(segment) + rotationPhase * 2.8f);
            p += normal * wobble * (2.5f + energy * 8.0f);

            const auto c = ((j + static_cast<int>(segment)) % 5 == 0 ? gold : purple)
                               .interpolatedWith(magenta, 0.40f);
            const auto sz = 0.8f + energy * 1.2f + static_cast<float>(j % 3) * 0.26f;
            g.setColour(c.withAlpha(0.08f + energy * 0.27f));
            g.fillEllipse(p.x - sz * 0.5f, p.y - sz * 0.5f, sz, sz);
        }
    }

    // Breakpoint nodes stay visible, but secondary to the aurora field.
    for (std::size_t i = 0; i < points.size(); ++i)
    {
        const auto amp = std::abs(visualState.amplitudes[i]);
        const auto nodeRadius = 3.0f + amp * 3.4f + energy * 1.2f;
        const auto c = (i % 3 == 0 ? gold : purple).interpolatedWith(magenta, amp * 0.35f);
        g.setColour(c.withAlpha(0.12f));
        g.fillEllipse(points[i].x - nodeRadius * 2.4f, points[i].y - nodeRadius * 2.4f,
                      nodeRadius * 4.8f, nodeRadius * 4.8f);
        g.setColour(c.withAlpha(0.88f));
        g.fillEllipse(points[i].x - nodeRadius, points[i].y - nodeRadius,
                      nodeRadius * 2.0f, nodeRadius * 2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.82f));
        g.fillEllipse(points[i].x - 1.1f, points[i].y - 1.1f, 2.2f, 2.2f);
    }

    g.setColour(juce::Colours::white.withAlpha(0.07f));
    g.drawEllipse(globe, 1.0f);
}

void VeloriaAudioProcessorEditor::resized()
{
    brand.setBounds(22, 14, 190, 24);
    title.setBounds(410, 8, 340, 34);
    subtitle.setBounds(435, 40, 290, 13);

    presetBox.setBounds(752, 15, 178, 30);
    discoverButton.setBounds(938, 15, 72, 30);
    newFieldButton.setBounds(1017, 15, 78, 30);
    monoButton.setBounds(1100, 15, 52, 30);

    ampWalk.setBounds(47, 120, 120, 103);
    timeWalk.setBounds(47, 231, 120, 103);
    ampMirror.setBounds(47, 342, 120, 103);
    timeMirror.setBounds(47, 453, 120, 48);

    attack.setBounds(32, 568, 91, 95);
    decay.setBounds(132, 568, 91, 95);
    sustain.setBounds(232, 568, 91, 95);
    release.setBounds(332, 568, 91, 95);

    seed.setBounds(474, 560, 92, 99);
    fieldStatus.setBounds(566, 578, 118, 35);

    presetNameEditor.setBounds(690, 548, 210, 27);
    savePresetButton.setBounds(690, 582, 62, 27);
    renamePresetButton.setBounds(758, 582, 68, 27);
    deletePresetButton.setBounds(832, 582, 68, 27);
    presetStatus.setBounds(690, 616, 210, 39);

    voiceStatus.setBounds(944, 315, 175, 22);
    level.setBounds(970, 557, 138, 108);

    footerStatus.setBounds(360, 694, 440, 14);
}
