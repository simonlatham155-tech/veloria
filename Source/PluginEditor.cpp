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

float parameterValue(juce::AudioProcessorValueTreeState& state, const char* id)
{
    if (auto* value = state.getRawParameterValue(id))
        return value->load();
    return 0.0f;
}
}

void VeloriaAudioProcessorEditor::AuroraLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
    juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                          static_cast<float>(width), static_cast<float>(height));
    auto labelBounds = bounds.removeFromTop(18.0f).toNearestInt();
    g.setColour(juce::Colours::white.withAlpha(0.76f));
    g.setFont(juce::FontOptions(8.7f, juce::Font::bold));
    g.drawFittedText(slider.getName(), labelBounds, juce::Justification::centred, 1);

    bounds = bounds.reduced(7.0f, 2.0f);
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
    setSize(1400, 900);

    brand.setText({}, juce::dontSendNotification);
    addAndMakeVisible(brand);

    title.setText("V E L O R I A", juce::dontSendNotification);
    title.setJustificationType(juce::Justification::centred);
    title.setFont(juce::FontOptions(29.0f));
    title.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.90f));
    addAndMakeVisible(title);

    subtitle.setText("DYNAMIC STOCHASTIC SYNTHESIS", juce::dontSendNotification);
    subtitle.setJustificationType(juce::Justification::centred);
    subtitle.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.25f));
    subtitle.setFont(juce::FontOptions(7.8f));
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
    configureKnob(ampMirror, "ampMirror", "AMP BARRIER");
    configureKnob(timeMirror, "timeMirror", "TIME BARRIER");
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

    ampWalk.setNumDecimalPlacesToDisplay(3);
    timeWalk.setNumDecimalPlacesToDisplay(3);
    ampMirror.setNumDecimalPlacesToDisplay(3);
    timeMirror.setNumDecimalPlacesToDisplay(3);
    attack.setNumDecimalPlacesToDisplay(3);
    decay.setNumDecimalPlacesToDisplay(3);
    sustain.setNumDecimalPlacesToDisplay(2);
    release.setNumDecimalPlacesToDisplay(3);
    seed.setTextValueSuffix(" seed");
    seed.setNumDecimalPlacesToDisplay(0);
    level.setTextValueSuffix(" dB");
    level.setNumDecimalPlacesToDisplay(1);

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
        else presetStatus.setText("ENTER A VALID PRESET NAME", juce::dontSendNotification);
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
        else presetStatus.setText("RENAME FAILED - NAME MAY ALREADY EXIST", juce::dontSendNotification);
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

    for (auto* label : { &fieldStatus, &voiceStatus, &presetStatus, &footerStatus })
        addAndMakeVisible(label);
    fieldStatus.setJustificationType(juce::Justification::centred);
    fieldStatus.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.55f));
    fieldStatus.setFont(juce::FontOptions(9.0f));
    voiceStatus.setJustificationType(juce::Justification::centredLeft);
    voiceStatus.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.62f));
    voiceStatus.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    presetStatus.setJustificationType(juce::Justification::centredLeft);
    presetStatus.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.40f));
    presetStatus.setFont(juce::FontOptions(8.5f));
    presetStatus.setText("RIGHT-CLICK ANY KNOB FOR MIDI LEARN", juce::dontSendNotification);
    footerStatus.setText("LIVE STOCHASTIC PLANET  //  BREAKPOINT GEOMETRY  //  DURATION FLOW  //  FIELD ENERGY  //  SEEDED MEMORY",
                         juce::dontSendNotification);
    footerStatus.setJustificationType(juce::Justification::centred);
    footerStatus.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.25f));
    footerStatus.setFont(juce::FontOptions(8.0f));

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
    slider.setName(displayName);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 74, 16);
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha(0.78f));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::white.withAlpha(0.05f));
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
    for (int i = 0; i < factoryNames.size(); ++i) presetBox.addItem(factoryNames[i], i + 1);
    const auto userNames = audioProcessor.getUserPresetNames();
    if (! userNames.isEmpty())
    {
        presetBox.addSeparator();
        presetBox.addSectionHeading("USER PRESETS");
        for (int i = 0; i < userNames.size(); ++i) presetBox.addItem(userNames[i], firstUserPresetId + i);
    }
    if (selectUserPreset.isNotEmpty())
    {
        const auto index = userNames.indexOf(selectUserPreset);
        if (index >= 0) presetBox.setSelectedId(firstUserPresetId + index, juce::dontSendNotification);
    }
    else
    {
        const auto program = audioProcessor.getCurrentProgram();
        if (program >= 0 && program < factoryNames.size()) presetBox.setSelectedId(program + 1, juce::dontSendNotification);
    }
}

bool VeloriaAudioProcessorEditor::selectedPresetIsUser() const noexcept
{
    return presetBox.getSelectedId() >= firstUserPresetId;
}

void VeloriaAudioProcessorEditor::timerCallback()
{
    visualState = audioProcessor.getVisualState();
    rotationPhase += 0.0024f + visualState.energy * 0.0100f;
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
    juce::ColourGradient bgGlow(purple.withAlpha(0.09f), getWidth() * 0.52f, getHeight() * 0.32f,
                                background, getWidth() * 0.52f, static_cast<float>(getHeight()), true);
    bgGlow.addColour(0.54, magenta.withAlpha(0.035f));
    g.setGradientFill(bgGlow);
    g.fillRect(getLocalBounds());
    g.setColour(juce::Colours::white.withAlpha(0.06f));
    g.drawRoundedRectangle(getLocalBounds().reduced(6).toFloat(), 13.0f, 1.0f);
    g.setColour(purple.withAlpha(0.28f));
    g.drawLine(14.0f, 62.0f, getWidth() - 14.0f, 62.0f, 1.0f);

    // LATHAMAUDIO is always uppercase. Weight change separates the two halves.
    const juce::Font brandLight(juce::FontOptions(18.0f));
    const juce::Font brandBold(juce::FontOptions(18.0f, juce::Font::bold));
    const juce::String lhs("LATHAM");
    const juce::String rhs("AUDIO");
    const auto lhsWidth = brandLight.getStringWidthFloat(lhs);
    g.setColour(juce::Colours::white.withAlpha(0.78f));
    g.setFont(brandLight);
    g.drawText(lhs, 22, 14, static_cast<int>(std::ceil(lhsWidth)) + 2, 26, juce::Justification::centredLeft);
    g.setFont(brandBold);
    g.drawText(rhs, 22 + static_cast<int>(std::ceil(lhsWidth)), 14, 90, 26, juce::Justification::centredLeft);

    drawPanel(g, { 14.0f, 74.0f, 212.0f, 556.0f }, "STOCHASTIC FIELD");
    drawPanel(g, { 236.0f, 74.0f, 892.0f, 556.0f }, "LIVING PLANET / LIVE MATHEMATICAL STATE");
    drawPanel(g, { 1138.0f, 74.0f, 248.0f, 268.0f }, "EVOLUTION / STRUCTURE");
    drawPanel(g, { 1138.0f, 352.0f, 248.0f, 278.0f }, "VOICE / PROBABILITY STATE");
    drawPanel(g, { 14.0f, 640.0f, 500.0f, 220.0f }, "AMPLITUDE ENVELOPE");
    drawPanel(g, { 524.0f, 640.0f, 610.0f, 220.0f }, "PRESETS / FIELD MEMORY");
    drawPanel(g, { 1144.0f, 640.0f, 242.0f, 220.0f }, "OUTPUT");

    drawStochasticGlobe(g, { 252.0f, 92.0f, 860.0f, 520.0f });
    drawEvolutionGraph(g, { 1156.0f, 116.0f, 212.0f, 142.0f });

    const auto aw = parameterValue(audioProcessor.parameters, "ampWalk");
    const auto tw = parameterValue(audioProcessor.parameters, "timeWalk");
    const auto am = parameterValue(audioProcessor.parameters, "ampMirror");
    const auto tm = parameterValue(audioProcessor.parameters, "timeMirror");
    const auto structure = juce::jlimit(0.0f, 1.0f, 1.0f - (aw * 0.48f + tw * 0.30f + am * 0.12f + tm * 0.10f));
    const auto motion = juce::jlimit(0.0f, 1.0f, tw * 0.62f + aw * 0.28f + visualState.energy * 0.10f);
    const auto tension = juce::jlimit(0.0f, 1.0f, aw * 0.62f + am * 0.22f + visualState.energy * 0.16f);
    const auto memory = juce::jlimit(0.0f, 1.0f, 1.0f - (aw + tw) * 0.42f);

    g.setFont(9.0f);
    g.setColour(cyan.withAlpha(0.72f));
    g.drawText("STRUCTURE  " + juce::String(structure * 100.0f, 0) + "%", 1156, 278, 210, 16, juce::Justification::centredLeft);
    g.setColour(purple.withAlpha(0.82f));
    g.drawText("MOTION     " + juce::String(motion * 100.0f, 0) + "%", 1156, 296, 210, 16, juce::Justification::centredLeft);
    g.setColour(gold.withAlpha(0.82f));
    g.drawText("TENSION    " + juce::String(tension * 100.0f, 0) + "%", 1156, 314, 210, 16, juce::Justification::centredLeft);

    g.setColour(cyan.withAlpha(0.58f));
    g.drawText("POLY 8  /  SECOND-ORDER WALK", 1156, 394, 214, 18, juce::Justification::centredLeft);
    g.setColour(juce::Colours::white.withAlpha(0.43f));
    g.drawText("MEMORY     " + juce::String(memory * 100.0f, 0) + "%", 1156, 424, 210, 18, juce::Justification::centredLeft);
    g.drawText("AMP RANGE  +/- " + juce::String(am, 2), 1156, 450, 210, 18, juce::Justification::centredLeft);
    g.drawText("TIME RANGE " + juce::String(tm, 2), 1156, 476, 210, 18, juce::Justification::centredLeft);
    g.drawText("ENERGY     " + juce::String(visualState.energy * 100.0f, 0) + "%", 1156, 502, 210, 18, juce::Justification::centredLeft);
    g.drawText("FIELD STATE IS SEEDED AND RECALLABLE", 1156, 548, 210, 42, juce::Justification::centredLeft, true);

    g.setColour(juce::Colours::white.withAlpha(0.30f));
    g.setFont(8.5f);
    g.drawText("PRIMARY NODES = BREAKPOINTS  /  PARTICLES = FIELD ACTIVITY  /  TRAILS = MEMORY  /  ORBITS = DURATION", 342, 602, 675, 16,
               juce::Justification::centred);
}

void VeloriaAudioProcessorEditor::drawKnobLabel(juce::Graphics&, juce::Slider&, const juce::String&)
{
}

void VeloriaAudioProcessorEditor::drawEvolutionGraph(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    juce::Path path;
    for (std::size_t i = 0; i < visualState.amplitudes.size(); ++i)
    {
        const auto x = bounds.getX() + bounds.getWidth() * static_cast<float>(i) / static_cast<float>(visualState.amplitudes.size() - 1);
        const auto y = bounds.getCentreY() - visualState.amplitudes[i] * bounds.getHeight() * 0.34f;
        if (i == 0) path.startNewSubPath(x, y); else path.lineTo(x, y);
    }
    g.setColour(purple.withAlpha(0.18f));
    g.strokePath(path, juce::PathStrokeType(7.0f, juce::PathStrokeType::curved));
    juce::ColourGradient graphGradient(purple, bounds.getX(), bounds.getCentreY(), gold, bounds.getRight(), bounds.getCentreY(), false);
    graphGradient.addColour(0.55, magenta);
    g.setGradientFill(graphGradient);
    g.strokePath(path, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved));
    for (std::size_t i = 0; i < visualState.amplitudes.size(); ++i)
    {
        const auto x = bounds.getX() + bounds.getWidth() * static_cast<float>(i) / static_cast<float>(visualState.amplitudes.size() - 1);
        const auto y = bounds.getCentreY() - visualState.amplitudes[i] * bounds.getHeight() * 0.34f;
        g.setColour(i % 3 == 0 ? gold : cyan);
        g.fillEllipse(x - 2.2f, y - 2.2f, 4.4f, 4.4f);
    }
}

void VeloriaAudioProcessorEditor::drawStochasticGlobe(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    const auto aw = parameterValue(audioProcessor.parameters, "ampWalk");
    const auto tw = parameterValue(audioProcessor.parameters, "timeWalk");
    const auto am = parameterValue(audioProcessor.parameters, "ampMirror");
    const auto energy = juce::jlimit(0.0f, 1.0f, visualState.energy);
    const auto motion = juce::jlimit(0.0f, 1.0f, tw * 0.70f + aw * 0.20f + energy * 0.10f);
    const auto tension = juce::jlimit(0.0f, 1.0f, aw * 0.60f + am * 0.25f + energy * 0.15f);
    const auto memory = juce::jlimit(0.0f, 1.0f, 1.0f - (aw + tw) * 0.42f);

    auto globe = bounds.withSizeKeepingCentre(520.0f, 520.0f);
    const auto centre = globe.getCentre();
    const auto radius = globe.getWidth() * 0.455f;

    // Outer orbital field. Fine lines stay secondary to the main nodes.
    for (int ring = 0; ring < 18; ++ring)
    {
        const auto rr = radius * (0.98f + 0.026f * static_cast<float>(ring));
        const auto flatten = 0.52f + 0.022f * static_cast<float>((ring * 7) % 13);
        const auto phase = rotationPhase * (ring % 2 == 0 ? 0.70f : -0.48f) + ring * 0.39f;
        juce::Path arc;
        arc.addCentredArc(centre.x, centre.y, rr, rr * flatten, phase,
                          0.05f, juce::MathConstants<float>::twoPi - 0.05f, true);
        const auto c = ring % 5 == 0 ? gold : (ring % 4 == 0 ? cyan : purple);
        g.setColour(c.withAlpha(0.028f + motion * 0.035f + energy * 0.045f));
        g.strokePath(arc, juce::PathStrokeType(0.45f + 0.035f * static_cast<float>(ring % 5)));
    }

    juce::ColourGradient halo(purple.withAlpha(0.24f + energy * 0.24f), centre.x, centre.y,
                               juce::Colours::transparentBlack, centre.x, centre.y + radius * 1.30f, true);
    halo.addColour(0.35, magenta.withAlpha(0.18f + tension * 0.14f));
    halo.addColour(0.70, orange.withAlpha(0.07f + energy * 0.11f));
    g.setGradientFill(halo);
    g.fillEllipse(globe.expanded(28.0f));

    juce::ColourGradient body(juce::Colour::fromRGB(30, 12, 51).withAlpha(0.87f),
                               centre.x - radius * 0.38f, centre.y - radius * 0.48f,
                               juce::Colour::fromRGB(1, 2, 7).withAlpha(0.97f),
                               centre.x + radius * 0.80f, centre.y + radius * 0.84f, true);
    body.addColour(0.36, juce::Colour::fromRGB(70, 17, 100).withAlpha(0.44f));
    body.addColour(0.62, juce::Colour::fromRGB(26, 20, 82).withAlpha(0.28f));
    g.setGradientFill(body);
    g.fillEllipse(globe);

    // Deep background particles: apparent back hemisphere.
    for (int i = 0; i < 1250; ++i)
    {
        const auto fi = static_cast<float>(i);
        const auto phase = fi * 2.39996323f + rotationPhase * (0.08f + static_cast<float>(i % 17) * 0.007f);
        const auto radialNorm = std::sqrt(static_cast<float>((i * 73) % 1249) / 1249.0f);
        const auto r = radius * radialNorm * 0.99f;
        const auto z = std::sin(fi * 0.173f + rotationPhase * (0.32f + motion * 0.22f));
        const auto squash = 0.58f + 0.22f * (z + 1.0f) * 0.5f;
        const auto x = centre.x + std::cos(phase) * r;
        const auto y = centre.y + std::sin(phase) * r * squash;
        const auto depth = juce::jlimit(0.0f, 1.0f, (z + 1.0f) * 0.5f);
        const auto sz = 0.28f + depth * 0.75f + static_cast<float>(i % 4) * 0.13f;
        const auto c = i % 17 == 0 ? gold : (i % 9 == 0 ? magenta : (i % 5 == 0 ? cyan : purple));
        g.setColour(c.withAlpha(0.018f + depth * 0.045f + energy * 0.065f));
        g.fillEllipse(x - sz * 0.5f, y - sz * 0.5f, sz, sz);
    }

    float totalDuration = 0.0f;
    for (const auto d : visualState.durations)
        totalDuration += juce::jmax(0.001f, d);
    totalDuration = juce::jmax(0.001f, totalDuration);

    std::array<juce::Point<float>, VeloriaAudioProcessor::visualBreakpointCount> points {};
    std::array<float, VeloriaAudioProcessor::visualBreakpointCount> angles {};
    float cumulative = 0.0f;
    for (std::size_t i = 0; i < points.size(); ++i)
    {
        const auto duration = juce::jmax(0.001f, visualState.durations[i]);
        const auto angle = rotationPhase - juce::MathConstants<float>::halfPi
                         + juce::MathConstants<float>::twoPi * (cumulative / totalDuration);
        angles[i] = angle;
        cumulative += duration;
        const auto amplitude = juce::jlimit(-1.0f, 1.0f, visualState.amplitudes[i]);
        const auto radial = radius * (0.52f + amplitude * 0.29f + std::sin(angle * 3.0f) * tw * 0.032f);
        const auto depth = 0.70f + 0.30f * std::sin(angle * 2.0f + rotationPhase * 0.55f);
        points[i] = { centre.x + std::cos(angle) * radial,
                      centre.y + std::sin(angle) * radial * depth };
    }

    // Duration relationship arcs.
    for (std::size_t i = 0; i < points.size(); ++i)
    {
        const auto next = (i + 1) % points.size();
        auto start = angles[i];
        auto end = next == 0 ? angles[0] + juce::MathConstants<float>::twoPi : angles[next];
        const auto rr = radius * (0.79f + 0.026f * static_cast<float>(i % 6));
        juce::Path sector;
        sector.addCentredArc(centre.x, centre.y, rr, rr * 0.72f, 0.0f, start, end, true);
        const auto c = i % 3 == 0 ? gold : (i % 2 == 0 ? cyan : purple);
        g.setColour(c.withAlpha(0.045f + tw * 0.12f));
        g.strokePath(sector, juce::PathStrokeType(0.60f + visualState.durations[i] * 0.12f));
    }

    // Memory trails: many thin echoes of the exact breakpoint skeleton.
    for (int history = 0; history < 38; ++history)
    {
        juce::Path trace;
        const auto h = static_cast<float>(history - 19);
        for (std::size_t segment = 0; segment < points.size(); ++segment)
        {
            const auto next = (segment + 1) % points.size();
            const auto p0 = points[segment];
            const auto p1 = points[next];
            const auto tangent = p1 - p0;
            auto normal = juce::Point<float>(-tangent.y, tangent.x);
            const auto len = juce::jmax(1.0f, normal.getDistanceFromOrigin());
            normal /= len;
            const auto phase = rotationPhase * (0.24f + history * 0.006f)
                             + static_cast<float>(segment) * 0.73f;
            const auto offset = h * (0.42f + memory * 0.31f)
                              + std::sin(phase) * (1.0f + motion * 3.8f);
            const auto p = p0 + normal * offset;
            if (segment == 0) trace.startNewSubPath(p); else trace.lineTo(p);
        }
        trace.closeSubPath();
        const auto c = history % 9 == 0 ? gold : (history % 6 == 0 ? cyan : purple);
        g.setColour(c.withAlpha(0.014f + memory * 0.020f + energy * 0.025f));
        g.strokePath(trace, juce::PathStrokeType(0.38f + static_cast<float>(history % 4) * 0.08f,
                                                 juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    }

    // Fine filament cloud. Lines are subordinate to nodes and particles.
    for (int ribbon = 0; ribbon < 46; ++ribbon)
    {
        juce::Path filament;
        const auto ribbonOffset = static_cast<float>(ribbon - 23);
        for (std::size_t segment = 0; segment < points.size(); ++segment)
        {
            const auto next = (segment + 1) % points.size();
            const auto p0 = points[segment];
            const auto p1 = points[next];
            for (int s = 0; s <= 16; ++s)
            {
                const auto t = static_cast<float>(s) / 16.0f;
                auto p = p0 + (p1 - p0) * t;
                const auto tangent = p1 - p0;
                auto normal = juce::Point<float>(-tangent.y, tangent.x);
                const auto len = juce::jmax(1.0f, normal.getDistanceFromOrigin());
                normal /= len;
                const auto wave = std::sin(t * juce::MathConstants<float>::pi * (1.0f + tw * 2.6f)
                                         + static_cast<float>(segment) * (0.41f + aw * 0.64f)
                                         + rotationPhase * (0.34f + ribbon * 0.007f));
                p += normal * (ribbonOffset * (0.34f + memory * 0.34f)
                              + wave * (1.8f + energy * 9.0f + tension * 5.0f));
                if (segment == 0 && s == 0) filament.startNewSubPath(p); else filament.lineTo(p);
            }
        }
        filament.closeSubPath();
        const auto c = ribbon % 11 == 0 ? gold : (ribbon % 7 == 0 ? cyan : (ribbon % 5 == 0 ? magenta : purple));
        g.setColour(c.withAlpha(0.010f + motion * 0.012f + energy * 0.032f));
        g.strokePath(filament, juce::PathStrokeType(0.55f,
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    // Particle streams directly around every breakpoint-to-breakpoint segment.
    for (std::size_t segment = 0; segment < points.size(); ++segment)
    {
        const auto next = (segment + 1) % points.size();
        const auto p0 = points[segment];
        const auto p1 = points[next];
        const auto tangent = p1 - p0;
        auto normal = juce::Point<float>(-tangent.y, tangent.x);
        const auto nlen = juce::jmax(1.0f, normal.getDistanceFromOrigin());
        normal /= nlen;

        for (int j = 0; j < 78; ++j)
        {
            const auto lane = static_cast<float>((j % 9) - 4);
            const auto speed = 0.010f + motion * 0.030f + 0.0008f * static_cast<float>((j + segment) % 7);
            const auto t = std::fmod(static_cast<float>(j) / 78.0f
                                   + rotationPhase * speed
                                   + static_cast<float>(segment) * 0.031f, 1.0f);
            auto p = p0 + tangent * t;
            const auto localWave = std::sin(t * (9.0f + tw * 13.0f)
                                          + static_cast<float>(segment) * 1.7f
                                          + rotationPhase * (0.7f + j * 0.004f));
            p += normal * (lane * (0.55f + memory * 0.45f)
                         + localWave * (1.2f + motion * 4.0f + energy * 4.5f));

            const auto depth = 0.5f + 0.5f * std::sin(static_cast<float>(j) * 0.77f
                                                     + static_cast<float>(segment) * 0.41f
                                                     + rotationPhase * 0.9f);
            const auto c = ((j + static_cast<int>(segment)) % 13 == 0 ? gold
                          : ((j + static_cast<int>(segment)) % 8 == 0 ? cyan : purple))
                               .interpolatedWith(magenta, tension * 0.20f);
            const auto sz = 0.45f + depth * 1.25f + energy * 0.80f;
            g.setColour(c.withAlpha(0.045f + depth * 0.16f + energy * 0.16f));
            g.fillEllipse(p.x - sz * 0.5f, p.y - sz * 0.5f, sz, sz);
        }
    }

    // Free-floating near particles: apparent foreground, still parameter-driven.
    for (int i = 0; i < 720; ++i)
    {
        const auto fi = static_cast<float>(i);
        const auto base = fi * 2.39996323f;
        const auto speed = 0.10f + static_cast<float>(i % 19) * 0.006f + motion * 0.14f;
        const auto angle = base + rotationPhase * speed;
        const auto shell = 0.18f + 0.96f * static_cast<float>((i * 83) % 719) / 719.0f;
        const auto breathing = 1.0f + std::sin(fi * 0.21f + rotationPhase * (0.6f + motion)) * (0.015f + aw * 0.035f);
        const auto r = radius * shell * breathing;
        const auto z = std::sin(fi * 0.137f + rotationPhase * 0.72f + tw * 2.0f);
        const auto x = centre.x + std::cos(angle) * r;
        const auto y = centre.y + std::sin(angle) * r * (0.64f + 0.18f * z);
        const auto depth = juce::jlimit(0.0f, 1.0f, (z + 1.0f) * 0.5f);
        const auto c = i % 23 == 0 ? gold : (i % 11 == 0 ? magenta : (i % 7 == 0 ? cyan : purple));
        const auto sz = 0.50f + depth * 1.65f + energy * 1.05f;
        g.setColour(c.withAlpha(0.030f + depth * 0.20f + energy * 0.18f));
        g.fillEllipse(x - sz * 0.5f, y - sz * 0.5f, sz, sz);

        if (i % 37 == 0)
        {
            const auto tail = juce::Point<float>(std::cos(angle - 0.04f), std::sin(angle - 0.04f) * 0.72f);
            g.setColour(c.withAlpha(0.025f + energy * 0.045f));
            g.drawLine(x, y, x - tail.x * (4.0f + motion * 7.0f), y - tail.y * (4.0f + motion * 7.0f), 0.55f);
        }
    }

    // Small stochastic storms. Rare bright events, never replacing the skeleton.
    const int stormCount = 5 + static_cast<int>(tension * 10.0f);
    for (int storm = 0; storm < stormCount; ++storm)
    {
        const auto idx = static_cast<std::size_t>((storm * 5 + 1) % static_cast<int>(points.size()));
        const auto anchor = points[idx];
        const auto stormRadius = 5.0f + tension * 13.0f + energy * 8.0f;
        const auto wobbleX = std::sin(rotationPhase * (0.8f + storm * 0.05f) + storm) * stormRadius * 0.55f;
        const auto wobbleY = std::cos(rotationPhase * (0.6f + storm * 0.04f) + storm * 1.3f) * stormRadius * 0.40f;
        const juce::Point<float> sc { anchor.x + wobbleX, anchor.y + wobbleY };
        juce::ColourGradient sg((storm % 2 == 0 ? gold : magenta).withAlpha(0.08f + energy * 0.14f), sc.x, sc.y,
                                juce::Colours::transparentBlack, sc.x + stormRadius, sc.y, true);
        g.setGradientFill(sg);
        g.fillEllipse(sc.x - stormRadius, sc.y - stormRadius, stormRadius * 2.0f, stormRadius * 2.0f);
    }

    // Main reference points: always the brightest and clearest objects.
    for (std::size_t i = 0; i < points.size(); ++i)
    {
        const auto amp = std::abs(visualState.amplitudes[i]);
        const auto nodeRadius = 3.0f + amp * 4.0f + energy * 1.35f;
        const auto c = (i % 3 == 0 ? gold : purple).interpolatedWith(magenta, amp * 0.42f);
        g.setColour(c.withAlpha(0.08f));
        g.fillEllipse(points[i].x - nodeRadius * 3.3f, points[i].y - nodeRadius * 3.3f,
                      nodeRadius * 6.6f, nodeRadius * 6.6f);
        g.setColour(c.withAlpha(0.92f));
        g.fillEllipse(points[i].x - nodeRadius, points[i].y - nodeRadius,
                      nodeRadius * 2.0f, nodeRadius * 2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.94f));
        g.fillEllipse(points[i].x - 1.15f, points[i].y - 1.15f, 2.3f, 2.3f);
    }

    // Subtle rim preserves a planet silhouette without burying the particle field.
    juce::ColourGradient shadow(juce::Colours::transparentBlack, centre.x - radius * 0.35f, centre.y,
                                juce::Colours::black.withAlpha(0.30f), centre.x + radius * 0.96f, centre.y, false);
    g.setGradientFill(shadow);
    g.fillEllipse(globe);
    g.setColour(juce::Colours::white.withAlpha(0.10f));
    g.drawEllipse(globe, 1.0f);
    g.setColour(cyan.withAlpha(0.08f + energy * 0.07f));
    g.drawEllipse(globe.reduced(2.0f), 1.4f);

    g.setFont(9.0f);
    auto metric = [&g](const juce::String& label, const juce::String& value,
                       juce::Point<float> p, juce::Colour c)
    {
        g.setColour(c.withAlpha(0.72f));
        g.drawText(label, static_cast<int>(p.x - 55.0f), static_cast<int>(p.y - 16.0f), 110, 14, juce::Justification::centred);
        g.setColour(juce::Colours::white.withAlpha(0.75f));
        g.drawText(value, static_cast<int>(p.x - 55.0f), static_cast<int>(p.y), 110, 16, juce::Justification::centred);
    };
    metric("AMP WALK", juce::String(aw, 3), { centre.x - radius * 1.23f, centre.y - radius * 0.62f }, purple);
    metric("TIME WALK", juce::String(tw, 3), { centre.x, centre.y - radius * 1.18f }, cyan);
    metric("MEMORY", juce::String(memory * 100.0f, 0) + "%", { centre.x + radius * 1.20f, centre.y - radius * 0.62f }, cyan);
    metric("TENSION", juce::String(tension * 100.0f, 0) + "%", { centre.x + radius * 1.24f, centre.y + radius * 0.45f }, gold);
    metric("FLOW", juce::String(motion * 100.0f, 0) + "%", { centre.x + radius * 0.82f, centre.y + radius * 1.00f }, cyan);
    metric("ENERGY", juce::String(energy * 100.0f, 0) + "%", { centre.x - radius * 0.82f, centre.y + radius * 1.00f }, magenta);
}

void VeloriaAudioProcessorEditor::resized()
{
    brand.setBounds(22, 14, 230, 24);
    title.setBounds(515, 8, 370, 34);
    subtitle.setBounds(535, 40, 330, 11);
    presetBox.setBounds(930, 15, 190, 30);
    discoverButton.setBounds(1128, 15, 78, 30);
    newFieldButton.setBounds(1213, 15, 84, 30);
    monoButton.setBounds(1305, 15, 68, 30);

    ampWalk.setBounds(48, 106, 144, 125);
    timeWalk.setBounds(48, 234, 144, 125);
    ampMirror.setBounds(48, 362, 144, 125);
    timeMirror.setBounds(48, 490, 144, 125);

    attack.setBounds(24, 680, 116, 150);
    decay.setBounds(142, 680, 116, 150);
    sustain.setBounds(260, 680, 116, 150);
    release.setBounds(378, 680, 116, 150);

    seed.setBounds(548, 700, 86, 92);
    fieldStatus.setBounds(646, 716, 148, 35);
    presetNameEditor.setBounds(808, 674, 300, 29);
    savePresetButton.setBounds(808, 712, 78, 29);
    renamePresetButton.setBounds(894, 712, 82, 29);
    deletePresetButton.setBounds(984, 712, 82, 29);
    presetStatus.setBounds(808, 754, 300, 47);

    voiceStatus.setBounds(1156, 366, 205, 22);
    level.setBounds(1188, 674, 154, 150);
    footerStatus.setBounds(415, 872, 570, 14);
}
