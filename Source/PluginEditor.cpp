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
    auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height);
    auto label = bounds.removeFromTop(17.0f).toNearestInt();
    g.setColour(juce::Colours::white.withAlpha(0.76f));
    g.setFont(juce::FontOptions(8.2f, juce::Font::bold));
    g.drawFittedText(slider.getName(), label, juce::Justification::centred, 1);

    bounds = bounds.reduced(6.0f, 2.0f);
    const auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    auto dial = bounds.withSizeKeepingCentre(size, size);
    const auto centre = dial.getCentre();
    const auto radius = size * 0.43f;
    const auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    juce::ColourGradient glow(purple.withAlpha(0.28f), centre.x, centre.y,
                              juce::Colours::transparentBlack, centre.x + radius * 1.3f, centre.y, true);
    glow.addColour(0.5, magenta.withAlpha(0.18f));
    g.setGradientFill(glow);
    g.fillEllipse(dial.expanded(6.0f));

    g.setColour(juce::Colour::fromRGB(17, 18, 25));
    g.fillEllipse(dial);
    g.setColour(juce::Colours::white.withAlpha(0.10f));
    g.drawEllipse(dial, 1.0f);

    juce::Path arc;
    arc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f,
                     rotaryStartAngle, angle, true);
    juce::ColourGradient grad(purple, dial.getX(), centre.y, gold, dial.getRight(), centre.y, false);
    grad.addColour(0.55, magenta);
    g.setGradientFill(grad);
    g.strokePath(arc, juce::PathStrokeType(3.4f, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));

    juce::Path pointer;
    pointer.addRoundedRectangle(-1.0f, -radius * 0.72f, 2.0f, radius * 0.42f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.92f));
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
}

void VeloriaAudioProcessorEditor::AuroraLookAndFeel::drawLinearSlider(
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float, float, juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical)
    {
        juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, 0, 0, style, slider);
        return;
    }

    auto b = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height);
    auto label = b.removeFromTop(18.0f).toNearestInt();
    g.setColour(juce::Colours::white.withAlpha(0.80f));
    g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
    g.drawFittedText(slider.getName(), label, juce::Justification::centred, 2);

    const float trackX = b.getCentreX();
    const float top = b.getY() + 8.0f;
    const float bottom = b.getBottom() - 22.0f;
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.fillRoundedRectangle(trackX - 3.0f, top, 6.0f, bottom - top, 3.0f);
    g.setColour(purple.withAlpha(0.55f));
    g.fillRoundedRectangle(trackX - 2.0f, sliderPos, 4.0f, bottom - sliderPos, 2.0f);
    g.setColour(magenta.withAlpha(0.95f));
    g.fillRoundedRectangle(trackX - 14.0f, sliderPos - 4.0f, 28.0f, 8.0f, 4.0f);
}

void VeloriaAudioProcessorEditor::MidiLearnSlider::mouseDown(const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu())
    {
        juce::Slider::mouseDown(e);
        return;
    }

    const auto currentCC = currentCCCallback ? currentCCCallback() : -1;
    juce::PopupMenu menu;
    menu.addItem(1, currentCC >= 0
        ? "Relearn MIDI CC (CC " + juce::String(currentCC) + ")"
        : "Learn MIDI CC");
    menu.addItem(2, "Clear MIDI CC", currentCC >= 0);

    juce::Component::SafePointer<MidiLearnSlider> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [safe](int r)
    {
        if (safe == nullptr) return;
        if (r == 1 && safe->learnCallback) safe->learnCallback();
        if (r == 2 && safe->clearCallback) safe->clearCallback();
    });
}

VeloriaAudioProcessorEditor::VeloriaAudioProcessorEditor(VeloriaAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(1400, 900);

    title.setText("V E L O R I A", juce::dontSendNotification);
    title.setJustificationType(juce::Justification::centred);
    title.setFont(juce::FontOptions(29.0f));
    title.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.92f));
    addAndMakeVisible(title);

    subtitle.setText("DYNAMIC STOCHASTIC SYNTHESIS", juce::dontSendNotification);
    subtitle.setJustificationType(juce::Justification::centred);
    subtitle.setFont(juce::FontOptions(7.8f));
    subtitle.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.25f));
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
        }
        else if (id >= firstUserPresetId)
        {
            audioProcessor.loadUserPreset(presetBox.getText());
        }
    };
    addAndMakeVisible(presetBox);
    refreshPresetBox();

    discoverButton.onClick = [this]
    {
        audioProcessor.discover();
        presetBox.setText("Discovered", juce::dontSendNotification);
    };
    newFieldButton.onClick = [this]
    {
        audioProcessor.newField();
        presetBox.setText("Field Variation", juce::dontSendNotification);
    };

    for (auto* b : { &discoverButton, &newFieldButton, &savePresetButton, &renamePresetButton })
    {
        b->setColour(juce::TextButton::buttonColourId, purple.withAlpha(0.24f));
        b->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        addAndMakeVisible(b);
    }
    addAndMakeVisible(deletePresetButton);

    monoButton.setClickingTogglesState(true);
    monoButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    monoButton.setColour(juce::ToggleButton::tickColourId, cyan);
    addAndMakeVisible(monoButton);

    orderButton.setColour(juce::TextButton::buttonColourId, purple.withAlpha(0.22f));
    orderButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.86f));
    orderButton.onClick = [this]
    {
        auto* raw = audioProcessor.parameters.getRawParameterValue("walkOrder");
        auto* parameter = audioProcessor.parameters.getParameter("walkOrder");
        if (raw == nullptr || parameter == nullptr) return;
        const auto current = raw->load() >= 1.5f ? 2.0f : 1.0f;
        const auto next = current == 1.0f ? 2.0f : 1.0f;
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(next));
        parameter->endChangeGesture();
        refreshOrderButton();
    };
    addAndMakeVisible(orderButton);
    refreshOrderButton();

    configureFieldSlider(ampWalk, "ampWalk", "AMP WALK");
    configureFieldSlider(timeWalk, "timeWalk", "TIME WALK");
    configureFieldSlider(ampMirror, "ampMirror", "AMP BARRIER");
    configureFieldSlider(timeMirror, "timeMirror", "TIME BARRIER");

    configureKnob(ampDist, "ampDist", "AMP DIST");
    configureKnob(timeDist, "timeDist", "TIME DIST");
    configureKnob(ampStep, "ampStep", "AMP STEP");
    configureKnob(timeStep, "timeStep", "TIME STEP");
    configureKnob(chaos, "chaos", "CHAOS");
    configureKnob(breakpoints, "breakpoints", "POINTS");
    configureKnob(pitchStability, "pitchStability", "PITCH LOCK");
    configureKnob(curve, "curve", "CURVE");

    configureKnob(attack, "attack", "ATTACK");
    configureKnob(decay, "decay", "DECAY");
    configureKnob(sustain, "sustain", "SUSTAIN");
    configureKnob(release, "release", "RELEASE");
    configureKnob(seed, "seed", "FIELD SEED");
    configureKnob(level, "level", "LEVEL");

    for (auto* s : { &ampWalk, &timeWalk, &ampMirror, &timeMirror,
                     &ampDist, &timeDist, &ampStep, &timeStep,
                     &chaos, &breakpoints, &pitchStability, &curve,
                     &attack, &decay, &sustain, &release, &seed, &level })
    {
        s->setLookAndFeel(&auroraLookAndFeel);
        addAndMakeVisible(s);
    }

    for (auto* s : { &ampWalk, &timeWalk, &ampMirror, &timeMirror,
                     &ampStep, &timeStep, &chaos, &pitchStability,
                     &curve, &attack, &decay, &release })
        s->setNumDecimalPlacesToDisplay(2);

    sustain.setNumDecimalPlacesToDisplay(2);
    ampDist.setNumDecimalPlacesToDisplay(0);
    timeDist.setNumDecimalPlacesToDisplay(0);
    breakpoints.setNumDecimalPlacesToDisplay(0);
    seed.setNumDecimalPlacesToDisplay(0);
    level.setNumDecimalPlacesToDisplay(1);
    level.setTextValueSuffix(" dB");

    ampWalkAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "ampWalk", ampWalk);
    timeWalkAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "timeWalk", timeWalk);
    ampMirrorAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "ampMirror", ampMirror);
    timeMirrorAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "timeMirror", timeMirror);
    ampDistAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "ampDist", ampDist);
    timeDistAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "timeDist", timeDist);
    ampStepAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "ampStep", ampStep);
    timeStepAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "timeStep", timeStep);
    chaosAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "chaos", chaos);
    breakpointsAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "breakpoints", breakpoints);
    pitchStabilityAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "pitchStability", pitchStability);
    curveAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "curve", curve);
    attackAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "attack", attack);
    decayAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "decay", decay);
    sustainAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "sustain", sustain);
    releaseAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "release", release);
    seedAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "seed", seed);
    levelAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "level", level);
    monoAttachment = std::make_unique<ButtonAttachment>(audioProcessor.parameters, "mono", monoButton);

    presetNameEditor.setTextToShowWhenEmpty("Name preset...", juce::Colours::white.withAlpha(0.28f));
    presetNameEditor.setColour(juce::TextEditor::backgroundColourId, panel2);
    presetNameEditor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    addAndMakeVisible(presetNameEditor);

    savePresetButton.onClick = [this]
    {
        if (audioProcessor.saveUserPreset(presetNameEditor.getText().trim()))
            refreshPresetBox(presetNameEditor.getText().trim());
    };
    renamePresetButton.onClick = [this]
    {
        if (selectedPresetIsUser()
            && audioProcessor.renameUserPreset(presetBox.getText(), presetNameEditor.getText().trim()))
            refreshPresetBox(presetNameEditor.getText().trim());
    };
    deletePresetButton.onClick = [this]
    {
        if (selectedPresetIsUser() && audioProcessor.deleteUserPreset(presetBox.getText()))
            refreshPresetBox();
    };

    for (auto* l : { &fieldStatus, &voiceStatus, &presetStatus, &footerStatus })
        addAndMakeVisible(l);

    voiceStatus.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.62f));
    fieldStatus.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.55f));
    presetStatus.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.40f));
    footerStatus.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.25f));
    footerStatus.setText("LIVE STOCHASTIC PLANET  //  5 MACRO ANCHORS  //  12 BREAKPOINTS  //  PARTICLE FIELD",
                         juce::dontSendNotification);
    footerStatus.setJustificationType(juce::Justification::centred);

    visualState = audioProcessor.getVisualState();
    startTimerHz(30);
}

VeloriaAudioProcessorEditor::~VeloriaAudioProcessorEditor()
{
    for (auto* s : { &ampWalk, &timeWalk, &ampMirror, &timeMirror,
                     &ampDist, &timeDist, &ampStep, &timeStep,
                     &chaos, &breakpoints, &pitchStability, &curve,
                     &attack, &decay, &sustain, &release, &seed, &level })
        s->setLookAndFeel(nullptr);
}

void VeloriaAudioProcessorEditor::configureKnob(MidiLearnSlider& s,
                                                 const juce::String& id,
                                                 const juce::String& name)
{
    s.setName(name);
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 58, 15);
    s.learnCallback = [this, id, name]
    {
        audioProcessor.beginMidiLearn(id);
        presetStatus.setText(name + " - MOVE A MIDI CC", juce::dontSendNotification);
    };
    s.clearCallback = [this, id] { audioProcessor.clearMidiMapping(id); };
    s.currentCCCallback = [this, id] { return audioProcessor.getMidiCCForParameter(id); };
}

void VeloriaAudioProcessorEditor::configureFieldSlider(MidiLearnSlider& s,
                                                        const juce::String& id,
                                                        const juce::String& name)
{
    s.setName(name);
    s.setSliderStyle(juce::Slider::LinearVertical);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 15);
    s.learnCallback = [this, id, name]
    {
        audioProcessor.beginMidiLearn(id);
        presetStatus.setText(name + " - MOVE A MIDI CC", juce::dontSendNotification);
    };
    s.clearCallback = [this, id] { audioProcessor.clearMidiMapping(id); };
    s.currentCCCallback = [this, id] { return audioProcessor.getMidiCCForParameter(id); };
}

void VeloriaAudioProcessorEditor::refreshOrderButton()
{
    const auto order = parameterValue(audioProcessor.parameters, "walkOrder") >= 1.5f ? 2 : 1;
    orderButton.setButtonText("ORDER " + juce::String(order));
}

void VeloriaAudioProcessorEditor::refreshPresetBox(const juce::String& select)
{
    presetBox.clear(juce::dontSendNotification);
    presetBox.addSectionHeading("FACTORY");
    const auto factory = audioProcessor.getFactoryPresetNames();
    for (int i = 0; i < factory.size(); ++i)
        presetBox.addItem(factory[i], i + 1);

    const auto user = audioProcessor.getUserPresetNames();
    if (! user.isEmpty())
    {
        presetBox.addSeparator();
        presetBox.addSectionHeading("USER PRESETS");
        for (int i = 0; i < user.size(); ++i)
            presetBox.addItem(user[i], firstUserPresetId + i);
    }

    if (select.isNotEmpty())
    {
        const auto index = user.indexOf(select);
        if (index >= 0)
            presetBox.setSelectedId(firstUserPresetId + index, juce::dontSendNotification);
    }
    else
    {
        const auto p = audioProcessor.getCurrentProgram();
        if (p >= 0 && p < factory.size())
            presetBox.setSelectedId(p + 1, juce::dontSendNotification);
    }
}

bool VeloriaAudioProcessorEditor::selectedPresetIsUser() const noexcept
{
    return presetBox.getSelectedId() >= firstUserPresetId;
}

void VeloriaAudioProcessorEditor::timerCallback()
{
    visualState = audioProcessor.getVisualState();
    rotationPhase += 0.0022f + visualState.energy * 0.0095f;
    if (rotationPhase > juce::MathConstants<float>::twoPi)
        rotationPhase -= juce::MathConstants<float>::twoPi;

    voiceStatus.setText("VOICE ENGINE   " + juce::String(visualState.activeVoices) + " / 8 ACTIVE",
                        juce::dontSendNotification);
    fieldStatus.setText(juce::String(visualState.activeVoices) + " VOICES  //  "
                        + juce::String((int) (visualState.energy * 100.0f)) + "% ENERGY",
                        juce::dontSendNotification);
    refreshOrderButton();
    repaint();
}

void VeloriaAudioProcessorEditor::drawPanel(juce::Graphics& g,
                                             juce::Rectangle<float> b,
                                             const juce::String& titleText)
{
    g.setColour(panel.withAlpha(0.92f));
    g.fillRoundedRectangle(b, 9.0f);
    g.setColour(juce::Colours::white.withAlpha(0.07f));
    g.drawRoundedRectangle(b, 9.0f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.62f));
    g.setFont(9.0f);
    g.drawText(titleText, b.toNearestInt().reduced(10, 7).removeFromTop(15),
               juce::Justification::centredLeft);
}

void VeloriaAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);
    juce::ColourGradient bg(purple.withAlpha(0.09f), 700, 280, background, 700, 900, true);
    g.setGradientFill(bg);
    g.fillRect(getLocalBounds());
    g.setColour(juce::Colours::white.withAlpha(0.06f));
    g.drawRoundedRectangle(getLocalBounds().reduced(6).toFloat(), 13.0f, 1.0f);
    g.setColour(purple.withAlpha(0.28f));
    g.drawLine(14, 62, 1386, 62, 1.0f);

    const juce::Font brandLight(juce::FontOptions(18.0f));
    const juce::Font brandBold(juce::FontOptions(18.0f, juce::Font::bold));
    g.setColour(juce::Colours::white.withAlpha(0.80f));
    g.setFont(brandLight);
    g.drawText("LATHAM", 22, 14, 70, 26, juce::Justification::centredLeft);
    g.setFont(brandBold);
    g.drawText("AUDIO", 86, 14, 72, 26, juce::Justification::centredLeft);

    drawPanel(g, { 14, 74, 330, 556 }, "STOCHASTIC FIELD / PERFORMANCE");
    drawPanel(g, { 354, 74, 774, 556 }, "LIVING PLANET / LIVE MATHEMATICAL STATE");
    drawPanel(g, { 1138, 74, 248, 268 }, "EVOLUTION / STRUCTURE");
    drawPanel(g, { 1138, 352, 248, 278 }, "VOICE / PROBABILITY STATE");
    drawPanel(g, { 14, 640, 500, 220 }, "AMPLITUDE ENVELOPE");
    drawPanel(g, { 524, 640, 610, 220 }, "PRESETS / FIELD MEMORY");
    drawPanel(g, { 1144, 640, 242, 220 }, "OUTPUT");

    drawStochasticGlobe(g, { 370, 92, 742, 520 });
    drawEvolutionGraph(g, { 1156, 116, 212, 142 });

    const auto bp = (int) parameterValue(audioProcessor.parameters, "breakpoints");
    const auto order = (int) parameterValue(audioProcessor.parameters, "walkOrder");
    const auto lock = parameterValue(audioProcessor.parameters, "pitchStability");
    const auto chaosValue = parameterValue(audioProcessor.parameters, "chaos");

    g.setColour(cyan.withAlpha(0.65f));
    g.setFont(9.0f);
    g.drawText("BREAKPOINTS  " + juce::String(bp), 1156, 424, 210, 18, juce::Justification::centredLeft);
    g.drawText("WALK ORDER   " + juce::String(order), 1156, 450, 210, 18, juce::Justification::centredLeft);
    g.drawText("PITCH LOCK   " + juce::String(lock * 100.0f, 0) + "%", 1156, 476, 210, 18, juce::Justification::centredLeft);
    g.setColour(gold.withAlpha(0.72f));
    g.drawText("CHAOS        " + juce::String(chaosValue * 100.0f, 0) + "%", 1156, 502, 210, 18, juce::Justification::centredLeft);
}

void VeloriaAudioProcessorEditor::drawEvolutionGraph(juce::Graphics& g, juce::Rectangle<float> b)
{
    juce::Path p;
    for (std::size_t i = 0; i < visualState.amplitudes.size(); ++i)
    {
        const auto x = b.getX() + b.getWidth() * (float) i / (float) (visualState.amplitudes.size() - 1);
        const auto y = b.getCentreY() - visualState.amplitudes[i] * b.getHeight() * 0.34f;
        if (i == 0) p.startNewSubPath(x, y); else p.lineTo(x, y);
    }
    g.setColour(purple.withAlpha(0.25f));
    g.strokePath(p, juce::PathStrokeType(6.0f));
    g.setColour(cyan.withAlpha(0.8f));
    g.strokePath(p, juce::PathStrokeType(1.4f));
}

void VeloriaAudioProcessorEditor::drawStochasticGlobe(juce::Graphics& g, juce::Rectangle<float> b)
{
    const auto aw = parameterValue(audioProcessor.parameters, "ampWalk");
    const auto tw = parameterValue(audioProcessor.parameters, "timeWalk");
    const auto ampBarrier = parameterValue(audioProcessor.parameters, "ampMirror");
    const auto timeBarrier = parameterValue(audioProcessor.parameters, "timeMirror");
    const auto chaosValue = parameterValue(audioProcessor.parameters, "chaos");
    const auto ampDistValue = parameterValue(audioProcessor.parameters, "ampDist");
    const auto timeDistValue = parameterValue(audioProcessor.parameters, "timeDist");
    const auto ampStepValue = parameterValue(audioProcessor.parameters, "ampStep");
    const auto timeStepValue = parameterValue(audioProcessor.parameters, "timeStep");
    const auto energy = juce::jlimit(0.0f, 1.0f, visualState.energy);
    const auto activity = juce::jlimit(0.0f, 1.0f,
        0.24f + aw * 0.18f + tw * 0.18f + energy * 0.14f + chaosValue * 0.26f);

    auto globe = b.withSizeKeepingCentre(506.0f, 506.0f);
    const auto c = globe.getCentre();
    const auto r = globe.getWidth() * 0.455f;

    // Global halo: luminous but transparent enough that internal detail dominates.
    juce::ColourGradient outerGlow(purple.withAlpha(0.25f + energy * 0.16f), c.x, c.y,
                                    juce::Colours::transparentBlack, c.x + r * 1.42f, c.y, true);
    outerGlow.addColour(0.34, magenta.withAlpha(0.16f + chaosValue * 0.07f));
    outerGlow.addColour(0.68, cyan.withAlpha(0.045f));
    g.setGradientFill(outerGlow);
    g.fillEllipse(globe.expanded(40.0f));

    // External orbit families and particle spill, visible around the limb.
    for (int ring = 0; ring < 24; ++ring)
    {
        const auto rr = r * (0.98f + ring * 0.018f);
        const auto flatten = 0.50f + 0.022f * (float) (ring % 10);
        const auto tilt = rotationPhase * (ring % 2 ? -0.34f : 0.27f) + ring * 0.19f;
        juce::Path orbit;
        orbit.addCentredArc(c.x, c.y, rr, rr * flatten, tilt,
                            0.04f, juce::MathConstants<float>::twoPi - 0.04f, true);
        const auto col = ring % 7 == 0 ? gold : (ring % 5 == 0 ? cyan : purple);
        g.setColour(col.withAlpha(0.035f + activity * 0.035f));
        g.strokePath(orbit, juce::PathStrokeType(0.55f + (ring % 9 == 0 ? 0.25f : 0.0f)));
    }

    for (int i = 0; i < 340; ++i)
    {
        const auto fi = (float) i;
        const auto a = fi * 2.39996323f + rotationPhase * (0.18f + (i % 13) * 0.012f);
        const auto rr = r * (1.00f + 0.18f * ((float) ((i * 37) % 337) / 337.0f));
        const auto yScale = 0.58f + 0.24f * std::sin(fi * 0.17f + rotationPhase);
        const auto x = c.x + std::cos(a) * rr;
        const auto y = c.y + std::sin(a) * rr * yScale;
        const auto size = 0.45f + (i % 11 == 0 ? 1.2f : 0.0f);
        const auto col = i % 19 == 0 ? gold : (i % 11 == 0 ? cyan : purple);
        g.setColour(col.withAlpha(0.05f + energy * 0.08f));
        g.fillEllipse(x - size * 0.5f, y - size * 0.5f, size, size);
    }

    // Atmospheric planet body. Intentionally translucent-looking rather than opaque.
    juce::ColourGradient body(juce::Colour::fromRGB(42, 12, 68).withAlpha(0.88f),
                              c.x - r * 0.64f, c.y - r * 0.64f,
                              juce::Colour::fromRGB(3, 3, 10).withAlpha(0.94f),
                              c.x + r * 0.80f, c.y + r * 0.80f, true);
    body.addColour(0.34, juce::Colour::fromRGB(52, 17, 94).withAlpha(0.72f));
    body.addColour(0.69, juce::Colour::fromRGB(12, 8, 30).withAlpha(0.82f));
    g.setGradientFill(body);
    g.fillEllipse(globe);

    juce::Graphics::ScopedSaveState clipped(g);
    juce::Path sphereClip;
    sphereClip.addEllipse(globe);
    g.reduceClipRegion(sphereClip);

    // Actual 12 DSP breakpoint positions: micro-structure only.
    float totalDuration = 0.0f;
    for (const auto d : visualState.durations)
        totalDuration += juce::jmax(0.001f, d);

    std::array<juce::Point<float>, VeloriaAudioProcessor::visualBreakpointCount> points {};
    float cumulative = 0.0f;
    for (std::size_t i = 0; i < points.size(); ++i)
    {
        const auto angle = rotationPhase - juce::MathConstants<float>::halfPi
                         + juce::MathConstants<float>::twoPi
                         * (cumulative / juce::jmax(0.001f, totalDuration));
        cumulative += juce::jmax(0.001f, visualState.durations[i]);
        const auto radial = r * (0.26f + visualState.amplitudes[i] * 0.12f);
        const auto depth = 0.72f + 0.20f * std::sin(angle * 2.0f + rotationPhase * 0.61f + i * 0.31f);
        points[i] = { c.x + std::cos(angle) * radial,
                      c.y + std::sin(angle) * radial * depth };
    }

    // FIVE MACRO ANCHORS. Their base positions are deliberately spread over the
    // planet like the reference image; grouped DSP statistics perturb them so they
    // remain genuinely related to the stochastic state without ever collapsing inward.
    const std::array<juce::Point<float>, 5> baseAnchorNorm {{
        { -0.48f, -0.32f },
        {  0.04f, -0.48f },
        {  0.50f, -0.18f },
        {  0.34f,  0.39f },
        { -0.40f,  0.36f }
    }};

    std::array<juce::Point<float>, 5> anchors {};
    std::array<float, 5> anchorEnergy {};
    for (int a = 0; a < 5; ++a)
    {
        float ampMean = 0.0f;
        float durMean = 0.0f;
        const int start = a * 2;
        const int count = a < 2 ? 3 : 2;
        for (int j = 0; j < count; ++j)
        {
            const auto idx = (std::size_t) ((start + j) % (int) points.size());
            ampMean += visualState.amplitudes[idx];
            durMean += visualState.durations[idx];
        }
        ampMean /= (float) count;
        durMean /= (float) count;
        const auto jitterX = ampMean * r * (0.055f + aw * 0.040f)
                           + std::sin(rotationPhase * (0.35f + a * 0.04f) + a * 1.31f)
                           * r * (0.012f + chaosValue * 0.022f);
        const auto jitterY = (durMean - 1.0f) * r * 0.020f
                           + std::cos(rotationPhase * (0.29f + a * 0.05f) + a * 0.77f)
                           * r * (0.010f + tw * 0.020f);
        anchors[(std::size_t) a] = {
            c.x + baseAnchorNorm[(std::size_t) a].x * r + jitterX,
            c.y + baseAnchorNorm[(std::size_t) a].y * r + jitterY
        };
        anchorEnergy[(std::size_t) a] = juce::jlimit(0.0f, 1.0f,
            0.32f + std::abs(ampMean) * 0.48f + energy * 0.20f);
    }

    // Low-alpha luminous cloud volumes around the five anchors.
    for (int a = 0; a < 5; ++a)
    {
        const auto& anchor = anchors[(std::size_t) a];
        const auto e = anchorEnergy[(std::size_t) a];
        const auto col = a == 0 || a == 3 ? gold : (a == 2 ? cyan : magenta);
        for (int cloud = 0; cloud < 18; ++cloud)
        {
            const auto cf = (float) cloud;
            const auto ang = cf * 2.39996323f + rotationPhase * (0.11f + a * 0.02f);
            const auto cr = r * (0.035f + 0.090f * std::sqrt((float) (cloud + 1) / 18.0f));
            const auto p = anchor + juce::Point<float>(std::cos(ang) * cr, std::sin(ang) * cr * 0.72f);
            const auto size = r * (0.035f + 0.030f * e);
            juce::ColourGradient fog(col.withAlpha(0.020f + e * 0.018f), p.x, p.y,
                                     juce::Colours::transparentBlack, p.x + size, p.y, true);
            g.setGradientFill(fog);
            g.fillEllipse(p.x - size, p.y - size, size * 2.0f, size * 2.0f);
        }
    }

    // Main full-sphere filament web: large curves organized by the five anchors.
    static constexpr int pairA[10] = { 0,0,0,0,1,1,1,2,2,3 };
    static constexpr int pairB[10] = { 1,2,3,4,2,3,4,3,4,4 };
    for (int family = 0; family < 10; ++family)
    {
        const auto aIndex = pairA[family];
        const auto bIndex = pairB[family];
        const auto p0 = anchors[(std::size_t) aIndex];
        const auto p3 = anchors[(std::size_t) bIndex];
        const auto midpoint = (p0 + p3) * 0.5f;
        auto radial = midpoint - c;
        auto normal = juce::Point<float>(-radial.y, radial.x);
        normal /= juce::jmax(1.0f, normal.getDistanceFromOrigin());

        for (int strand = 0; strand < 13; ++strand)
        {
            const auto sf = (float) strand - 6.0f;
            const auto bend = r * (0.07f + 0.016f * (float) (family % 4))
                            * (1.0f + chaosValue * 0.55f);
            const auto phase = rotationPhase * (0.24f + family * 0.011f)
                             + family * 0.41f + strand * 0.17f;
            const auto offset = sf * r * 0.0068f;
            const auto control1 = p0 * 0.68f + midpoint * 0.32f
                                + normal * (bend + offset + std::sin(phase) * r * 0.018f);
            const auto control2 = p3 * 0.68f + midpoint * 0.32f
                                - normal * (bend - offset + std::cos(phase * 0.91f) * r * 0.018f);

            juce::Path filament;
            filament.startNewSubPath(p0);
            filament.cubicTo(control1, control2, p3);
            const auto col = family % 4 == 0 ? gold
                           : (family % 3 == 0 ? cyan
                           : (family % 2 == 0 ? magenta : purple));
            g.setColour(col.withAlpha(0.028f + activity * 0.042f));
            g.strokePath(filament, juce::PathStrokeType(0.48f + (strand == 6 ? 0.38f : 0.0f),
                                                         juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
        }
    }

    // Additional sweeping orbit filaments crossing the entire sphere.
    for (int sweep = 0; sweep < 64; ++sweep)
    {
        const auto sf = (float) sweep;
        const auto rx = r * (0.52f + 0.42f * (0.5f + 0.5f * std::sin(sf * 0.71f)));
        const auto ry = r * (0.18f + 0.58f * (0.5f + 0.5f * std::cos(sf * 0.43f)));
        const auto rot = rotationPhase * (0.10f + (sweep % 9) * 0.006f) + sf * 0.193f;
        juce::Path orbit;
        orbit.addCentredArc(c.x, c.y, rx, ry, rot,
                            0.03f, juce::MathConstants<float>::twoPi - 0.03f, true);
        const auto col = sweep % 13 == 0 ? gold : (sweep % 7 == 0 ? cyan : purple);
        g.setColour(col.withAlpha(0.012f + activity * 0.026f));
        g.strokePath(orbit, juce::PathStrokeType(0.42f));
    }

    // Particle volume: deliberately bright enough to remain visible even at idle.
    const int particleCount = 3000;
    for (int i = 0; i < particleCount; ++i)
    {
        const auto fi = (float) i;
        const auto anchorIndex = i % 5;
        const auto anchor = anchors[(std::size_t) anchorIndex];
        const auto z = std::sin(fi * 0.619f
                              + rotationPhase * (0.19f + tw * 0.36f)
                              + std::sin(fi * 0.071f) * chaosValue);
        const auto near = (z + 1.0f) * 0.5f;
        const auto angle = fi * 2.39996323f
                         + rotationPhase * (0.13f + (i % 23) * 0.006f)
                         + anchorIndex * 0.83f;
        const auto localNorm = std::sqrt((float) ((i * 97) % 2999) / 2999.0f);
        const auto localR = r * (0.045f + localNorm * 0.42f);
        const auto anchorPull = 0.48f + 0.18f * std::sin(fi * 0.037f + rotationPhase);
        auto p = c * (1.0f - anchorPull) + anchor * anchorPull;
        p.x += std::cos(angle) * localR * (0.76f + 0.24f * near);
        p.y += std::sin(angle) * localR * (0.50f + 0.28f * near);

        const auto globalDrift = std::sin(fi * 0.113f + rotationPhase * (0.9f + activity * 1.4f));
        p += juce::Point<float>(globalDrift * r * 0.016f * ampStepValue,
                                std::cos(fi * 0.091f + rotationPhase) * r * 0.014f * timeStepValue);

        const auto size = 0.38f + near * 1.10f + (i % 137 == 0 ? 1.55f : 0.0f);
        auto col = i % 37 == 0 ? gold
                 : (i % 19 == 0 ? cyan
                 : (i % 11 == 0 ? magenta : purple));
        const auto distBias = juce::jlimit(0.0f, 1.0f, (ampDistValue + timeDistValue) / 10.0f);
        if (i % 53 == 0)
            col = col.interpolatedWith(gold, 0.24f + distBias * 0.42f);

        const auto alpha = 0.040f + near * 0.070f + activity * 0.055f;
        g.setColour(col.withAlpha(alpha));
        g.fillEllipse(p.x - size * 0.5f, p.y - size * 0.5f, size, size);
    }

    // Flowing spark streams between macro anchors.
    for (int family = 0; family < 10; ++family)
    {
        const auto p0 = anchors[(std::size_t) pairA[family]];
        const auto p1 = anchors[(std::size_t) pairB[family]];
        const auto delta = p1 - p0;
        auto normal = juce::Point<float>(-delta.y, delta.x);
        normal /= juce::jmax(1.0f, normal.getDistanceFromOrigin());
        for (int j = 0; j < 64; ++j)
        {
            const auto t = std::fmod((float) j / 64.0f
                                   + rotationPhase * (0.009f + tw * 0.014f)
                                   + family * 0.067f, 1.0f);
            auto p = p0 + delta * t;
            const auto wave = std::sin(t * juce::MathConstants<float>::twoPi * 2.0f
                                     + family * 0.61f + rotationPhase * 1.9f);
            p += normal * wave * r * (0.015f + chaosValue * 0.026f);
            const auto spark = j % 17 == 0;
            const auto size = spark ? 2.3f : 0.55f + (j % 4) * 0.12f;
            const auto col = spark ? gold : (family % 3 == 0 ? magenta : purple);
            g.setColour(col.withAlpha(spark ? 0.48f : 0.075f + activity * 0.075f));
            g.fillEllipse(p.x - size * 0.5f, p.y - size * 0.5f, size, size);
        }
    }

    // 12 true breakpoints: smaller than macro anchors, visually unmistakable as micro nodes.
    for (std::size_t i = 0; i < points.size(); ++i)
    {
        const auto amp = std::abs(visualState.amplitudes[i]);
        const auto nr = 1.7f + amp * 1.9f + energy * 0.35f;
        const auto col = i % 4 == 0 ? gold : (i % 3 == 0 ? cyan : magenta);
        g.setColour(col.withAlpha(0.08f + amp * 0.07f));
        g.fillEllipse(points[i].x - nr * 2.5f, points[i].y - nr * 2.5f, nr * 5.0f, nr * 5.0f);
        g.setColour(col.withAlpha(0.88f));
        g.fillEllipse(points[i].x - nr, points[i].y - nr, nr * 2.0f, nr * 2.0f);
    }

    // Five macro anchor storms: the dominant reference points from the target visual.
    for (int a = 0; a < 5; ++a)
    {
        const auto anchor = anchors[(std::size_t) a];
        const auto e = anchorEnergy[(std::size_t) a];
        const auto col = a == 0 || a == 3 ? gold : (a == 2 ? cyan : magenta);
        const auto core = r * (0.018f + 0.014f * e);

        for (int p = 0; p < 96; ++p)
        {
            const auto pf = (float) p;
            const auto angle = pf * 2.39996323f + rotationPhase * (0.42f + a * 0.06f);
            const auto rr = r * (0.012f + 0.13f * std::sqrt((float) (p + 1) / 96.0f))
                          * (0.58f + e * 0.62f);
            const auto q = anchor + juce::Point<float>(std::cos(angle) * rr,
                                                        std::sin(angle) * rr * 0.72f);
            const auto size = 0.45f + (p % 13 == 0 ? 1.4f : 0.0f);
            g.setColour(col.withAlpha(0.040f + e * 0.075f));
            g.fillEllipse(q.x - size * 0.5f, q.y - size * 0.5f, size, size);
        }

        juce::ColourGradient anchorGlow(col.withAlpha(0.28f + e * 0.20f), anchor.x, anchor.y,
                                        juce::Colours::transparentBlack, anchor.x + core * 4.6f, anchor.y, true);
        g.setGradientFill(anchorGlow);
        g.fillEllipse(anchor.x - core * 4.6f, anchor.y - core * 4.6f,
                      core * 9.2f, core * 9.2f);
        g.setColour(col.withAlpha(0.96f));
        g.fillEllipse(anchor.x - core, anchor.y - core, core * 2.0f, core * 2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.95f));
        g.fillEllipse(anchor.x - 1.4f, anchor.y - 1.4f, 2.8f, 2.8f);
    }

    // Gentle spherical shading only; never bury the particle system.
    juce::ColourGradient limb(juce::Colours::transparentBlack,
                              c.x - r * 0.46f, c.y - r * 0.32f,
                              juce::Colours::black.withAlpha(0.22f),
                              c.x + r * 0.95f, c.y + r * 0.88f, false);
    g.setGradientFill(limb);
    g.fillEllipse(globe);

    g.setColour(cyan.withAlpha(0.12f + energy * 0.05f));
    g.drawEllipse(globe, 0.9f);
    g.setColour(purple.withAlpha(0.16f));
    g.drawEllipse(globe.reduced(3.0f), 0.65f);
    g.setColour(gold.withAlpha(0.055f + chaosValue * 0.035f));
    g.drawEllipse(globe.reduced(7.0f), 0.50f);
}

void VeloriaAudioProcessorEditor::resized()
{
    title.setBounds(515, 8, 370, 34);
    subtitle.setBounds(535, 40, 330, 11);
    presetBox.setBounds(930, 15, 190, 30);
    discoverButton.setBounds(1128, 15, 78, 30);
    newFieldButton.setBounds(1213, 15, 84, 30);
    monoButton.setBounds(1305, 15, 68, 30);

    ampWalk.setBounds(28, 108, 64, 242);
    timeWalk.setBounds(104, 108, 64, 242);
    ampMirror.setBounds(180, 108, 64, 242);
    timeMirror.setBounds(256, 108, 64, 242);

    ampDist.setBounds(24, 374, 70, 104);
    timeDist.setBounds(100, 374, 70, 104);
    ampStep.setBounds(176, 374, 70, 104);
    timeStep.setBounds(252, 374, 70, 104);
    chaos.setBounds(24, 490, 70, 104);
    breakpoints.setBounds(100, 490, 70, 104);
    pitchStability.setBounds(176, 490, 70, 104);
    curve.setBounds(252, 490, 70, 104);
    orderButton.setBounds(25, 600, 68, 20);

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
