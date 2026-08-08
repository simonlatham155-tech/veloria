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

float hash01(int n) noexcept
{
    const auto x = std::sin((float) n * 12.9898f + 78.233f) * 43758.5453f;
    return x - std::floor(x);
}

void drawVeloriaWordmark(juce::Graphics& g, juce::Rectangle<float> area)
{
    const auto h = area.getHeight();
    const auto stroke = juce::jmax(1.0f, h * 0.045f);
    const float glyphW = h * 0.78f;
    const float gap = h * 0.34f;
    const float totalW = glyphW * 7.0f + gap * 6.0f;
    const float x0 = area.getCentreX() - totalW * 0.5f;
    const float y = area.getY();
    const float top = y + h * 0.10f;
    const float mid = y + h * 0.50f;
    const float bot = y + h * 0.90f;

    g.setColour(juce::Colours::white.withAlpha(0.94f));
    juce::Path p;
    auto X = [=](int i) { return x0 + (glyphW + gap) * (float) i; };

    // V
    p.startNewSubPath(X(0), top);
    p.lineTo(X(0) + glyphW * 0.50f, bot);
    p.lineTo(X(0) + glyphW, top);

    // E - open, geometric three-stroke form.
    p.startNewSubPath(X(1) + glyphW, top); p.lineTo(X(1), top);
    p.lineTo(X(1), bot); p.lineTo(X(1) + glyphW, bot);
    p.startNewSubPath(X(1), mid); p.lineTo(X(1) + glyphW * 0.72f, mid);

    // L
    p.startNewSubPath(X(2), top); p.lineTo(X(2), bot); p.lineTo(X(2) + glyphW, bot);

    // O - squared rounded-looking octagon.
    p.startNewSubPath(X(3) + glyphW * 0.22f, top);
    p.lineTo(X(3) + glyphW * 0.78f, top);
    p.lineTo(X(3) + glyphW, top + h * 0.22f);
    p.lineTo(X(3) + glyphW, bot - h * 0.22f);
    p.lineTo(X(3) + glyphW * 0.78f, bot);
    p.lineTo(X(3) + glyphW * 0.22f, bot);
    p.lineTo(X(3), bot - h * 0.22f);
    p.lineTo(X(3), top + h * 0.22f);
    p.closeSubPath();

    // R - futuristic open bowl + diagonal leg.
    p.startNewSubPath(X(4), bot); p.lineTo(X(4), top);
    p.lineTo(X(4) + glyphW * 0.68f, top);
    p.lineTo(X(4) + glyphW, top + h * 0.18f);
    p.lineTo(X(4) + glyphW, mid - h * 0.06f);
    p.lineTo(X(4) + glyphW * 0.68f, mid + h * 0.05f);
    p.lineTo(X(4), mid + h * 0.05f);
    p.startNewSubPath(X(4) + glyphW * 0.53f, mid + h * 0.05f);
    p.lineTo(X(4) + glyphW, bot);

    // I
    p.startNewSubPath(X(5) + glyphW * 0.5f, top); p.lineTo(X(5) + glyphW * 0.5f, bot);

    // A - open apex with crossbar.
    p.startNewSubPath(X(6), bot);
    p.lineTo(X(6) + glyphW * 0.50f, top);
    p.lineTo(X(6) + glyphW, bot);
    p.startNewSubPath(X(6) + glyphW * 0.24f, mid + h * 0.08f);
    p.lineTo(X(6) + glyphW * 0.76f, mid + h * 0.08f);

    g.strokePath(p, juce::PathStrokeType(stroke, juce::PathStrokeType::curved,
                                          juce::PathStrokeType::rounded));
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

    title.setText({}, juce::dontSendNotification);
    title.setJustificationType(juce::Justification::centred);
    title.setColour(juce::Label::textColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(title);

    subtitle.setText("DYNAMIC STOCHASTIC SYNTHESIS", juce::dontSendNotification);
    subtitle.setJustificationType(juce::Justification::centred);
    subtitle.setFont(juce::FontOptions(7.8f));
    subtitle.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.38f));
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
    footerStatus.setText("LIVE STOCHASTIC PLANET  //  BREAKPOINT GEOMETRY  //  ORBITS  //  DURATION FIELD  //  STORMS",
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

    drawVeloriaWordmark(g, { 520.0f, 9.0f, 360.0f, 31.0f });

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
        0.38f + aw * 0.15f + tw * 0.14f + energy * 0.16f + chaosValue * 0.18f);

    auto globe = b.withSizeKeepingCentre(510.0f, 510.0f);
    const auto c = globe.getCentre();
    const auto r = globe.getWidth() * 0.455f;

    // The reference sphere is mostly defined by light and activity, not an opaque ball.
    juce::ColourGradient outerGlow(purple.withAlpha(0.16f + energy * 0.10f), c.x, c.y,
                                    juce::Colours::transparentBlack, c.x + r * 1.50f, c.y, true);
    outerGlow.addColour(0.30, magenta.withAlpha(0.10f));
    outerGlow.addColour(0.62, cyan.withAlpha(0.025f));
    g.setGradientFill(outerGlow);
    g.fillEllipse(globe.expanded(54.0f));

    // Outside orbit families and spill particles establish the planetary silhouette.
    for (int ring = 0; ring < 42; ++ring)
    {
        const auto rr = r * (0.94f + ring * 0.010f);
        const auto flatten = 0.52f + 0.010f * (float) (ring % 15);
        const auto tilt = rotationPhase * (ring % 2 ? -0.22f : 0.18f) + ring * 0.137f;
        juce::Path orbit;
        orbit.addCentredArc(c.x, c.y, rr, rr * flatten, tilt,
                            0.03f, juce::MathConstants<float>::twoPi - 0.03f, true);
        const auto col = ring % 11 == 0 ? gold : (ring % 7 == 0 ? cyan : purple);
        g.setColour(col.withAlpha(0.020f + activity * 0.025f));
        g.strokePath(orbit, juce::PathStrokeType(0.40f + (ring % 13 == 0 ? 0.28f : 0.0f)));
    }

    for (int i = 0; i < 620; ++i)
    {
        const auto fi = (float) i;
        const auto a = fi * 2.39996323f + rotationPhase * (0.12f + (i % 17) * 0.006f);
        const auto rr = r * (0.91f + 0.29f * hash01(i * 19 + 7));
        const auto flatten = 0.63f + 0.24f * std::sin(fi * 0.087f + rotationPhase * 0.4f);
        const auto x = c.x + std::cos(a) * rr;
        const auto y = c.y + std::sin(a) * rr * flatten;
        const auto bright = i % 53 == 0;
        const auto size = bright ? 2.0f : 0.42f + hash01(i * 7) * 0.65f;
        const auto col = i % 31 == 0 ? gold : (i % 17 == 0 ? magenta : (i % 13 == 0 ? cyan : purple));
        g.setColour(col.withAlpha(bright ? 0.40f : 0.045f + activity * 0.025f));
        g.fillEllipse(x - size * 0.5f, y - size * 0.5f, size, size);
    }

    // Very dark transparent atmosphere. The network must remain the dominant object.
    juce::ColourGradient body(juce::Colour::fromRGB(35, 10, 60).withAlpha(0.46f),
                              c.x - r * 0.55f, c.y - r * 0.55f,
                              juce::Colour::fromRGB(2, 3, 9).withAlpha(0.74f),
                              c.x + r * 0.84f, c.y + r * 0.82f, true);
    body.addColour(0.33, juce::Colour::fromRGB(46, 14, 82).withAlpha(0.38f));
    body.addColour(0.72, juce::Colour::fromRGB(9, 7, 26).withAlpha(0.58f));
    g.setGradientFill(body);
    g.fillEllipse(globe);

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
        const auto radial = r * (0.43f + visualState.amplitudes[i] * 0.22f);
        const auto depth = 0.73f + 0.21f * std::sin(angle * 1.9f + rotationPhase * 0.61f + i * 0.31f);
        points[i] = { c.x + std::cos(angle) * radial,
                      c.y + std::sin(angle) * radial * depth };
    }

    const std::array<juce::Point<float>, 5> baseAnchorNorm {{
        { -0.43f, -0.31f },
        {  0.05f, -0.49f },
        {  0.48f, -0.15f },
        {  0.31f,  0.40f },
        { -0.40f,  0.35f }
    }};

    std::array<juce::Point<float>, 5> anchors {};
    std::array<float, 5> anchorEnergy {};
    for (int a = 0; a < 5; ++a)
    {
        const auto i0 = (std::size_t) ((a * 2) % 12);
        const auto i1 = (std::size_t) ((a * 2 + 1) % 12);
        const auto i2 = (std::size_t) ((a * 2 + 5) % 12);
        const auto ampMean = (visualState.amplitudes[i0] + visualState.amplitudes[i1] + visualState.amplitudes[i2]) / 3.0f;
        const auto durMean = (visualState.durations[i0] + visualState.durations[i1] + visualState.durations[i2]) / 3.0f;
        const auto jitterX = ampMean * r * (0.038f + aw * 0.030f)
                           + std::sin(rotationPhase * (0.30f + a * 0.035f) + a * 1.31f)
                           * r * (0.009f + chaosValue * 0.018f);
        const auto jitterY = (durMean - 1.0f) * r * 0.015f
                           + std::cos(rotationPhase * (0.26f + a * 0.041f) + a * 0.77f)
                           * r * (0.008f + tw * 0.016f);
        anchors[(std::size_t) a] = {
            c.x + baseAnchorNorm[(std::size_t) a].x * r + jitterX,
            c.y + baseAnchorNorm[(std::size_t) a].y * r + jitterY
        };
        anchorEnergy[(std::size_t) a] = juce::jlimit(0.0f, 1.0f,
            0.36f + std::abs(ampMean) * 0.40f + energy * 0.24f);
    }

    // Secondary luminous hubs are what stop the five anchors reading as a pentagon.
    std::array<juce::Point<float>, 26> hubs {};
    for (int h = 0; h < (int) hubs.size(); ++h)
    {
        const auto hf = (float) h;
        const auto bp = points[(std::size_t) (h % 12)];
        const auto anchor = anchors[(std::size_t) ((h * 3 + 1) % 5)];
        const auto blend = 0.24f + 0.50f * hash01(h * 29 + 3);
        auto p = bp * blend + anchor * (1.0f - blend);
        const auto ang = hf * 2.39996323f + rotationPhase * (0.10f + (h % 7) * 0.013f);
        const auto rr = r * (0.05f + 0.16f * hash01(h * 31 + 9));
        p += { std::cos(ang) * rr, std::sin(ang) * rr * (0.62f + hash01(h * 17) * 0.25f) };
        hubs[(std::size_t) h] = p;
    }

    {
        juce::Graphics::ScopedSaveState clipped(g);
        juce::Path sphereClip;
        sphereClip.addEllipse(globe);
        g.reduceClipRegion(sphereClip);

        // Dense deep particle volume across the whole sphere, not clustered around anchors.
        for (int i = 0; i < 3200; ++i)
        {
            const auto fi = (float) i;
            const auto z = std::sin(fi * 0.613f + rotationPhase * (0.13f + tw * 0.23f));
            const auto near = (z + 1.0f) * 0.5f;
            const auto radial = std::sqrt(hash01(i * 37 + 11));
            const auto angle = fi * 2.39996323f
                             + rotationPhase * (0.08f + (i % 29) * 0.004f)
                             + std::sin(fi * 0.031f) * chaosValue * 0.33f;
            const auto turbulence = 1.0f + std::sin(fi * 0.109f + rotationPhase * (0.7f + activity)) * 0.08f;
            const auto rr = r * radial * turbulence;
            const auto x = c.x + std::cos(angle) * rr;
            const auto y = c.y + std::sin(angle) * rr * (0.73f + 0.23f * z);
            const auto bright = i % 149 == 0;
            const auto size = bright ? 2.2f : 0.34f + near * 0.95f;
            auto col = i % 43 == 0 ? gold : (i % 23 == 0 ? magenta : (i % 17 == 0 ? cyan : purple));
            if (i % 67 == 0)
                col = col.interpolatedWith(gold, juce::jlimit(0.15f, 0.65f, (ampDistValue + timeDistValue) * 0.08f));
            g.setColour(col.withAlpha(bright ? 0.55f : 0.034f + near * 0.072f + energy * 0.034f));
            g.fillEllipse(x - size * 0.5f, y - size * 0.5f, size, size);
        }

        // Hundreds of curved trajectories weaving hub-to-hub and anchor-to-hub.
        for (int strand = 0; strand < 220; ++strand)
        {
            const auto a = strand % 5;
            const auto h0 = (strand * 7 + 3) % (int) hubs.size();
            const auto h1 = (strand * 13 + 11) % (int) hubs.size();
            const auto p0 = strand % 3 == 0 ? anchors[(std::size_t) a] : hubs[(std::size_t) h0];
            const auto p3 = hubs[(std::size_t) h1];
            const auto mid = (p0 + p3) * 0.5f;
            auto tangent = p3 - p0;
            juce::Point<float> normal(-tangent.y, tangent.x);
            normal /= juce::jmax(1.0f, normal.getDistanceFromOrigin());
            auto radial = mid - c;
            radial /= juce::jmax(1.0f, radial.getDistanceFromOrigin());
            const auto bend = r * (0.030f + 0.14f * hash01(strand * 19 + 5)) * (0.72f + chaosValue * 0.65f);
            const auto phase = rotationPhase * (0.11f + (strand % 17) * 0.004f) + strand * 0.31f;
            const auto cp1 = p0 * 0.66f + mid * 0.34f
                           + normal * bend + radial * std::sin(phase) * r * 0.035f;
            const auto cp2 = p3 * 0.66f + mid * 0.34f
                           - normal * bend + radial * std::cos(phase * 0.87f) * r * 0.035f;
            juce::Path filament;
            filament.startNewSubPath(p0);
            filament.cubicTo(cp1, cp2, p3);
            const auto col = strand % 19 == 0 ? gold
                           : (strand % 13 == 0 ? cyan
                           : (strand % 7 == 0 ? magenta : purple));
            const auto major = strand % 23 == 0;
            g.setColour(col.withAlpha((major ? 0.105f : 0.026f) + activity * (major ? 0.045f : 0.022f)));
            g.strokePath(filament, juce::PathStrokeType(major ? 0.92f : 0.42f,
                                                         juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
        }

        // Orbit arcs at many scales. These imply the sphere's curvature.
        for (int sweep = 0; sweep < 110; ++sweep)
        {
            const auto sf = (float) sweep;
            const auto rx = r * (0.40f + 0.55f * hash01(sweep * 11 + 1));
            const auto ry = r * (0.13f + 0.67f * hash01(sweep * 13 + 4));
            const auto rot = rotationPhase * (0.07f + (sweep % 11) * 0.005f) + sf * 0.173f;
            const auto ox = c.x + (hash01(sweep * 17 + 2) - 0.5f) * r * 0.22f;
            const auto oy = c.y + (hash01(sweep * 23 + 8) - 0.5f) * r * 0.20f;
            juce::Path orbit;
            orbit.addCentredArc(ox, oy, rx, ry, rot,
                                0.04f, juce::MathConstants<float>::twoPi - 0.04f, true);
            const auto col = sweep % 17 == 0 ? gold : (sweep % 11 == 0 ? magenta : purple);
            g.setColour(col.withAlpha(0.014f + activity * 0.020f));
            g.strokePath(orbit, juce::PathStrokeType(0.38f + (sweep % 29 == 0 ? 0.42f : 0.0f)));
        }

        // Flowing sparks travel along the broad network rather than exposing its skeleton.
        for (int family = 0; family < 36; ++family)
        {
            const auto p0 = hubs[(std::size_t) ((family * 5 + 1) % (int) hubs.size())];
            const auto p1 = hubs[(std::size_t) ((family * 11 + 7) % (int) hubs.size())];
            const auto delta = p1 - p0;
            juce::Point<float> normal(-delta.y, delta.x);
            normal /= juce::jmax(1.0f, normal.getDistanceFromOrigin());
            for (int j = 0; j < 34; ++j)
            {
                const auto t = std::fmod((float) j / 34.0f
                                       + rotationPhase * (0.006f + tw * 0.012f)
                                       + family * 0.043f, 1.0f);
                auto p = p0 + delta * t;
                p += normal * std::sin(t * juce::MathConstants<float>::twoPi * 2.0f
                                     + family * 0.71f + rotationPhase * 1.7f)
                   * r * (0.008f + chaosValue * 0.017f);
                const auto bright = j % 15 == 0;
                const auto size = bright ? 1.9f : 0.45f + (j % 3) * 0.12f;
                const auto col = bright ? gold : (family % 4 == 0 ? magenta : purple);
                g.setColour(col.withAlpha(bright ? 0.52f : 0.055f + activity * 0.055f));
                g.fillEllipse(p.x - size * 0.5f, p.y - size * 0.5f, size, size);
            }
        }

        // Secondary hubs: many bright intersections like the reference.
        for (int h = 0; h < (int) hubs.size(); ++h)
        {
            const auto p = hubs[(std::size_t) h];
            const auto col = h % 7 == 0 ? gold : (h % 5 == 0 ? cyan : (h % 3 == 0 ? magenta : purple));
            const auto sz = 1.5f + 1.6f * hash01(h * 19 + 7) + energy * 0.8f;
            juce::ColourGradient glow(col.withAlpha(0.18f), p.x, p.y,
                                      juce::Colours::transparentBlack, p.x + sz * 5.0f, p.y, true);
            g.setGradientFill(glow);
            g.fillEllipse(p.x - sz * 5.0f, p.y - sz * 5.0f, sz * 10.0f, sz * 10.0f);
            g.setColour(col.withAlpha(0.82f));
            g.fillEllipse(p.x - sz * 0.5f, p.y - sz * 0.5f, sz, sz);
        }

        // True DSP breakpoints remain recognizable but are only one layer of the field.
        for (std::size_t i = 0; i < points.size(); ++i)
        {
            const auto amp = std::abs(visualState.amplitudes[i]);
            const auto nr = 1.6f + amp * 1.8f + energy * 0.3f;
            const auto col = i % 4 == 0 ? gold : (i % 3 == 0 ? cyan : magenta);
            g.setColour(col.withAlpha(0.10f + amp * 0.06f));
            g.fillEllipse(points[i].x - nr * 3.0f, points[i].y - nr * 3.0f, nr * 6.0f, nr * 6.0f);
            g.setColour(col.withAlpha(0.92f));
            g.fillEllipse(points[i].x - nr, points[i].y - nr, nr * 2.0f, nr * 2.0f);
        }

        // Five macro anchors: luminous event centres, no visible pentagon geometry.
        for (int a = 0; a < 5; ++a)
        {
            const auto anchor = anchors[(std::size_t) a];
            const auto e = anchorEnergy[(std::size_t) a];
            const auto col = a == 0 || a == 3 ? gold : (a == 2 ? cyan : magenta);
            const auto core = r * (0.010f + 0.009f * e);

            for (int p = 0; p < 120; ++p)
            {
                const auto pf = (float) p;
                const auto angle = pf * 2.39996323f + rotationPhase * (0.28f + a * 0.045f);
                const auto rr = r * (0.009f + 0.105f * std::sqrt((float) (p + 1) / 120.0f))
                              * (0.55f + e * 0.70f);
                const auto q = anchor + juce::Point<float>(std::cos(angle) * rr,
                                                            std::sin(angle) * rr * 0.72f);
                const auto size = p % 21 == 0 ? 1.7f : 0.40f + hash01(p * 13 + a) * 0.50f;
                g.setColour(col.withAlpha(0.035f + e * 0.060f));
                g.fillEllipse(q.x - size * 0.5f, q.y - size * 0.5f, size, size);
            }

            juce::ColourGradient anchorGlow(col.withAlpha(0.24f + e * 0.16f), anchor.x, anchor.y,
                                            juce::Colours::transparentBlack, anchor.x + core * 6.0f, anchor.y, true);
            g.setGradientFill(anchorGlow);
            g.fillEllipse(anchor.x - core * 6.0f, anchor.y - core * 6.0f,
                          core * 12.0f, core * 12.0f);
            g.setColour(col.withAlpha(0.96f));
            g.fillEllipse(anchor.x - core, anchor.y - core, core * 2.0f, core * 2.0f);
            g.setColour(juce::Colours::white.withAlpha(0.94f));
            g.fillEllipse(anchor.x - 1.2f, anchor.y - 1.2f, 2.4f, 2.4f);
        }

        // Very light limb shading only.
        juce::ColourGradient limb(juce::Colours::transparentBlack,
                                  c.x - r * 0.50f, c.y - r * 0.35f,
                                  juce::Colours::black.withAlpha(0.12f),
                                  c.x + r * 0.98f, c.y + r * 0.90f, false);
        g.setGradientFill(limb);
        g.fillEllipse(globe);
    }

    // Foreground particles and arcs cross the limb, adding the reference's depth.
    for (int i = 0; i < 460; ++i)
    {
        const auto fi = (float) i;
        const auto a = fi * 2.39996323f + rotationPhase * (0.16f + (i % 19) * 0.006f);
        const auto rr = r * (0.72f + 0.42f * hash01(i * 23 + 5));
        const auto x = c.x + std::cos(a) * rr;
        const auto y = c.y + std::sin(a) * rr * (0.64f + 0.26f * std::sin(fi * 0.071f));
        const auto bright = i % 61 == 0;
        const auto size = bright ? 2.4f : 0.50f + hash01(i * 11 + 2) * 0.80f;
        const auto col = i % 29 == 0 ? gold : (i % 17 == 0 ? cyan : (i % 13 == 0 ? magenta : purple));
        g.setColour(col.withAlpha(bright ? 0.64f : 0.055f + energy * 0.045f));
        g.fillEllipse(x - size * 0.5f, y - size * 0.5f, size, size);
    }

    // Multiple faint rims, not a solid circumference.
    g.setColour(cyan.withAlpha(0.075f + energy * 0.035f));
    g.drawEllipse(globe, 0.65f);
    g.setColour(purple.withAlpha(0.11f));
    g.drawEllipse(globe.reduced(2.5f), 0.55f);
    g.setColour(gold.withAlpha(0.035f + chaosValue * 0.025f));
    g.drawEllipse(globe.reduced(6.0f), 0.45f);
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
