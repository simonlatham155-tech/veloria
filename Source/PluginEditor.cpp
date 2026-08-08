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
    if (auto* value = state.getRawParameterValue(id)) return value->load();
    return 0.0f;
}
}

void VeloriaAudioProcessorEditor::AuroraLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
    juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height);
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
    g.setGradientFill(glow); g.fillEllipse(dial.expanded(6.0f));
    g.setColour(juce::Colour::fromRGB(17,18,25)); g.fillEllipse(dial);
    g.setColour(juce::Colours::white.withAlpha(0.10f)); g.drawEllipse(dial, 1.0f);

    juce::Path arc;
    arc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, angle, true);
    juce::ColourGradient grad(purple, dial.getX(), centre.y, gold, dial.getRight(), centre.y, false);
    grad.addColour(0.55, magenta); g.setGradientFill(grad);
    g.strokePath(arc, juce::PathStrokeType(3.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path pointer; pointer.addRoundedRectangle(-1.0f, -radius * 0.72f, 2.0f, radius * 0.42f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.92f));
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
}

void VeloriaAudioProcessorEditor::AuroraLookAndFeel::drawLinearSlider(
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float, float, juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical)
    {
        juce::LookAndFeel_V4::drawLinearSlider(g,x,y,width,height,sliderPos,0,0,style,slider);
        return;
    }
    auto b = juce::Rectangle<float>((float)x,(float)y,(float)width,(float)height);
    auto label = b.removeFromTop(18.0f).toNearestInt();
    g.setColour(juce::Colours::white.withAlpha(0.80f));
    g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
    g.drawFittedText(slider.getName(), label, juce::Justification::centred, 2);

    const float trackX = b.getCentreX();
    const float top = b.getY()+8.0f, bottom=b.getBottom()-22.0f;
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.fillRoundedRectangle(trackX-3.0f, top, 6.0f, bottom-top, 3.0f);
    g.setColour(purple.withAlpha(0.55f));
    g.fillRoundedRectangle(trackX-2.0f, sliderPos, 4.0f, bottom-sliderPos, 2.0f);
    g.setColour(magenta.withAlpha(0.95f));
    g.fillRoundedRectangle(trackX-14.0f, sliderPos-4.0f, 28.0f, 8.0f, 4.0f);
}

void VeloriaAudioProcessorEditor::MidiLearnSlider::mouseDown(const juce::MouseEvent& e)
{
    if (!e.mods.isPopupMenu()) { juce::Slider::mouseDown(e); return; }
    const auto currentCC = currentCCCallback ? currentCCCallback() : -1;
    juce::PopupMenu menu;
    menu.addItem(1, currentCC >= 0 ? "Relearn MIDI CC (CC " + juce::String(currentCC) + ")" : "Learn MIDI CC");
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
    title.setFont(juce::FontOptions(29.0f)); title.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.92f)); addAndMakeVisible(title);
    subtitle.setText("DYNAMIC STOCHASTIC SYNTHESIS", juce::dontSendNotification);
    subtitle.setJustificationType(juce::Justification::centred); subtitle.setFont(juce::FontOptions(7.8f)); subtitle.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.25f)); addAndMakeVisible(subtitle);

    presetBox.setColour(juce::ComboBox::backgroundColourId, panel2); presetBox.setColour(juce::ComboBox::outlineColourId,purple.withAlpha(0.38f)); presetBox.setColour(juce::ComboBox::textColourId,juce::Colours::white);
    presetBox.onChange=[this]{const auto id=presetBox.getSelectedId(); const auto fc=audioProcessor.getFactoryPresetNames().size(); if(id>0&&id<=fc){audioProcessor.setCurrentProgram(id-1); presetNameEditor.setText(presetBox.getText()+" Copy",juce::dontSendNotification);} else if(id>=firstUserPresetId) audioProcessor.loadUserPreset(presetBox.getText());};
    addAndMakeVisible(presetBox); refreshPresetBox();

    discoverButton.onClick=[this]{audioProcessor.discover(); presetBox.setText("Discovered",juce::dontSendNotification);};
    newFieldButton.onClick=[this]{audioProcessor.newField(); presetBox.setText("Field Variation",juce::dontSendNotification);};
    for(auto* b:{&discoverButton,&newFieldButton,&savePresetButton,&renamePresetButton}){b->setColour(juce::TextButton::buttonColourId,purple.withAlpha(0.24f)); b->setColour(juce::TextButton::textColourOffId,juce::Colours::white); addAndMakeVisible(b);} addAndMakeVisible(deletePresetButton);
    monoButton.setClickingTogglesState(true); monoButton.setColour(juce::ToggleButton::textColourId,juce::Colours::white); monoButton.setColour(juce::ToggleButton::tickColourId,cyan); addAndMakeVisible(monoButton);

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

    configureFieldSlider(ampWalk,"ampWalk","AMP WALK"); configureFieldSlider(timeWalk,"timeWalk","TIME WALK");
    configureFieldSlider(ampMirror,"ampMirror","AMP BARRIER"); configureFieldSlider(timeMirror,"timeMirror","TIME BARRIER");
    configureKnob(ampDist,"ampDist","AMP DIST"); configureKnob(timeDist,"timeDist","TIME DIST"); configureKnob(ampStep,"ampStep","AMP STEP"); configureKnob(timeStep,"timeStep","TIME STEP");
    configureKnob(chaos,"chaos","CHAOS"); configureKnob(breakpoints,"breakpoints","POINTS"); configureKnob(pitchStability,"pitchStability","PITCH LOCK"); configureKnob(curve,"curve","CURVE");
    configureKnob(attack,"attack","ATTACK"); configureKnob(decay,"decay","DECAY"); configureKnob(sustain,"sustain","SUSTAIN"); configureKnob(release,"release","RELEASE"); configureKnob(seed,"seed","FIELD SEED"); configureKnob(level,"level","LEVEL");

    for(auto* s:{&ampWalk,&timeWalk,&ampMirror,&timeMirror,&ampDist,&timeDist,&ampStep,&timeStep,&chaos,&breakpoints,&pitchStability,&curve,&attack,&decay,&sustain,&release,&seed,&level}){s->setLookAndFeel(&auroraLookAndFeel); addAndMakeVisible(s);}
    for(auto* s:{&ampWalk,&timeWalk,&ampMirror,&timeMirror,&ampStep,&timeStep,&chaos,&pitchStability,&curve,&attack,&decay,&release}) s->setNumDecimalPlacesToDisplay(2);
    sustain.setNumDecimalPlacesToDisplay(2); ampDist.setNumDecimalPlacesToDisplay(0); timeDist.setNumDecimalPlacesToDisplay(0); breakpoints.setNumDecimalPlacesToDisplay(0); seed.setNumDecimalPlacesToDisplay(0); level.setNumDecimalPlacesToDisplay(1); level.setTextValueSuffix(" dB");

    ampWalkAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"ampWalk",ampWalk); timeWalkAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"timeWalk",timeWalk); ampMirrorAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"ampMirror",ampMirror); timeMirrorAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"timeMirror",timeMirror);
    ampDistAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"ampDist",ampDist); timeDistAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"timeDist",timeDist); ampStepAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"ampStep",ampStep); timeStepAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"timeStep",timeStep); chaosAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"chaos",chaos); breakpointsAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"breakpoints",breakpoints); pitchStabilityAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"pitchStability",pitchStability); curveAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"curve",curve);
    attackAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"attack",attack); decayAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"decay",decay); sustainAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"sustain",sustain); releaseAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"release",release); seedAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"seed",seed); levelAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"level",level); monoAttachment=std::make_unique<ButtonAttachment>(audioProcessor.parameters,"mono",monoButton);

    presetNameEditor.setTextToShowWhenEmpty("Name preset...",juce::Colours::white.withAlpha(0.28f)); presetNameEditor.setColour(juce::TextEditor::backgroundColourId,panel2); presetNameEditor.setColour(juce::TextEditor::textColourId,juce::Colours::white); addAndMakeVisible(presetNameEditor);
    savePresetButton.onClick=[this]{if(audioProcessor.saveUserPreset(presetNameEditor.getText().trim())) refreshPresetBox(presetNameEditor.getText().trim());};
    renamePresetButton.onClick=[this]{if(selectedPresetIsUser()&&audioProcessor.renameUserPreset(presetBox.getText(),presetNameEditor.getText().trim())) refreshPresetBox(presetNameEditor.getText().trim());};
    deletePresetButton.onClick=[this]{if(selectedPresetIsUser()&&audioProcessor.deleteUserPreset(presetBox.getText())) refreshPresetBox();};

    for(auto* l:{&fieldStatus,&voiceStatus,&presetStatus,&footerStatus}) addAndMakeVisible(l);
    voiceStatus.setColour(juce::Label::textColourId,juce::Colours::white.withAlpha(0.62f)); fieldStatus.setColour(juce::Label::textColourId,juce::Colours::white.withAlpha(0.55f)); presetStatus.setColour(juce::Label::textColourId,juce::Colours::white.withAlpha(0.40f)); footerStatus.setColour(juce::Label::textColourId,juce::Colours::white.withAlpha(0.25f));
    footerStatus.setText("LIVE STOCHASTIC PLANET  //  BREAKPOINT GEOMETRY  //  PARTICLE FIELD  //  SEEDED MEMORY",juce::dontSendNotification); footerStatus.setJustificationType(juce::Justification::centred);
    visualState=audioProcessor.getVisualState(); startTimerHz(30);
}

VeloriaAudioProcessorEditor::~VeloriaAudioProcessorEditor()
{
    for(auto* s:{&ampWalk,&timeWalk,&ampMirror,&timeMirror,&ampDist,&timeDist,&ampStep,&timeStep,&chaos,&breakpoints,&pitchStability,&curve,&attack,&decay,&sustain,&release,&seed,&level}) s->setLookAndFeel(nullptr);
}

void VeloriaAudioProcessorEditor::configureKnob(MidiLearnSlider& s,const juce::String& id,const juce::String& name)
{
    s.setName(name); s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag); s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,58,15);
    s.learnCallback=[this,id,name]{audioProcessor.beginMidiLearn(id); presetStatus.setText(name+" - MOVE A MIDI CC",juce::dontSendNotification);}; s.clearCallback=[this,id]{audioProcessor.clearMidiMapping(id);}; s.currentCCCallback=[this,id]{return audioProcessor.getMidiCCForParameter(id);};
}

void VeloriaAudioProcessorEditor::configureFieldSlider(MidiLearnSlider& s,const juce::String& id,const juce::String& name)
{
    s.setName(name); s.setSliderStyle(juce::Slider::LinearVertical); s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,56,15);
    s.learnCallback=[this,id,name]{audioProcessor.beginMidiLearn(id); presetStatus.setText(name+" - MOVE A MIDI CC",juce::dontSendNotification);}; s.clearCallback=[this,id]{audioProcessor.clearMidiMapping(id);}; s.currentCCCallback=[this,id]{return audioProcessor.getMidiCCForParameter(id);};
}

void VeloriaAudioProcessorEditor::refreshOrderButton()
{
    const auto order = parameterValue(audioProcessor.parameters, "walkOrder") >= 1.5f ? 2 : 1;
    orderButton.setButtonText("ORDER " + juce::String(order));
}

void VeloriaAudioProcessorEditor::refreshPresetBox(const juce::String& select)
{
    presetBox.clear(juce::dontSendNotification); presetBox.addSectionHeading("FACTORY"); const auto f=audioProcessor.getFactoryPresetNames(); for(int i=0;i<f.size();++i)presetBox.addItem(f[i],i+1); const auto u=audioProcessor.getUserPresetNames(); if(!u.isEmpty()){presetBox.addSeparator();presetBox.addSectionHeading("USER PRESETS");for(int i=0;i<u.size();++i)presetBox.addItem(u[i],firstUserPresetId+i);} if(select.isNotEmpty()){const auto i=u.indexOf(select);if(i>=0)presetBox.setSelectedId(firstUserPresetId+i,juce::dontSendNotification);} else {const auto p=audioProcessor.getCurrentProgram();if(p>=0&&p<f.size())presetBox.setSelectedId(p+1,juce::dontSendNotification);}
}

bool VeloriaAudioProcessorEditor::selectedPresetIsUser() const noexcept{return presetBox.getSelectedId()>=firstUserPresetId;}

void VeloriaAudioProcessorEditor::timerCallback()
{
    visualState=audioProcessor.getVisualState(); rotationPhase+=0.0022f+visualState.energy*0.0095f; if(rotationPhase>juce::MathConstants<float>::twoPi)rotationPhase-=juce::MathConstants<float>::twoPi; voiceStatus.setText("VOICE ENGINE   "+juce::String(visualState.activeVoices)+" / 8 ACTIVE",juce::dontSendNotification); fieldStatus.setText(juce::String(visualState.activeVoices)+" VOICES  //  "+juce::String((int)(visualState.energy*100.0f))+"% ENERGY",juce::dontSendNotification); refreshOrderButton(); repaint();
}

void VeloriaAudioProcessorEditor::drawPanel(juce::Graphics& g,juce::Rectangle<float> b,const juce::String& t)
{
    g.setColour(panel.withAlpha(0.92f));g.fillRoundedRectangle(b,9.0f);g.setColour(juce::Colours::white.withAlpha(0.07f));g.drawRoundedRectangle(b,9.0f,1.0f);g.setColour(juce::Colours::white.withAlpha(0.62f));g.setFont(9.0f);g.drawText(t,b.toNearestInt().reduced(10,7).removeFromTop(15),juce::Justification::centredLeft);
}

void VeloriaAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(background); juce::ColourGradient bg(purple.withAlpha(0.09f),700,280,background,700,900,true);g.setGradientFill(bg);g.fillRect(getLocalBounds());
    g.setColour(juce::Colours::white.withAlpha(0.06f));g.drawRoundedRectangle(getLocalBounds().reduced(6).toFloat(),13.0f,1.0f);g.setColour(purple.withAlpha(0.28f));g.drawLine(14,62,1386,62,1.0f);

    // LATHAMAUDIO: one word, LATHAM light / AUDIO bold.
    const juce::Font brandLight(juce::FontOptions(18.0f));
    const juce::Font brandBold(juce::FontOptions(18.0f,juce::Font::bold));
    g.setColour(juce::Colours::white.withAlpha(0.80f));
    g.setFont(brandLight); g.drawText("LATHAM",22,14,70,26,juce::Justification::centredLeft);
    g.setFont(brandBold); g.drawText("AUDIO",86,14,72,26,juce::Justification::centredLeft);

    drawPanel(g,{14,74,330,556},"STOCHASTIC FIELD / PERFORMANCE"); drawPanel(g,{354,74,774,556},"LIVING PLANET / LIVE MATHEMATICAL STATE"); drawPanel(g,{1138,74,248,268},"EVOLUTION / STRUCTURE"); drawPanel(g,{1138,352,248,278},"VOICE / PROBABILITY STATE"); drawPanel(g,{14,640,500,220},"AMPLITUDE ENVELOPE"); drawPanel(g,{524,640,610,220},"PRESETS / FIELD MEMORY"); drawPanel(g,{1144,640,242,220},"OUTPUT");
    drawStochasticGlobe(g,{370,92,742,520}); drawEvolutionGraph(g,{1156,116,212,142});

    const auto bp=(int)parameterValue(audioProcessor.parameters,"breakpoints"); const auto order=(int)parameterValue(audioProcessor.parameters,"walkOrder"); const auto lock=parameterValue(audioProcessor.parameters,"pitchStability"); const auto chaosValue=parameterValue(audioProcessor.parameters,"chaos");
    g.setColour(cyan.withAlpha(0.65f));g.setFont(9.0f);g.drawText("BREAKPOINTS  "+juce::String(bp),1156,424,210,18,juce::Justification::centredLeft);g.drawText("WALK ORDER   "+juce::String(order),1156,450,210,18,juce::Justification::centredLeft);g.drawText("PITCH LOCK   "+juce::String(lock*100.0f,0)+"%",1156,476,210,18,juce::Justification::centredLeft);g.setColour(gold.withAlpha(0.72f));g.drawText("CHAOS        "+juce::String(chaosValue*100.0f,0)+"%",1156,502,210,18,juce::Justification::centredLeft);
}

void VeloriaAudioProcessorEditor::drawEvolutionGraph(juce::Graphics& g,juce::Rectangle<float> b)
{
    juce::Path p;for(std::size_t i=0;i<visualState.amplitudes.size();++i){const auto x=b.getX()+b.getWidth()*(float)i/(float)(visualState.amplitudes.size()-1);const auto y=b.getCentreY()-visualState.amplitudes[i]*b.getHeight()*0.34f;if(i==0)p.startNewSubPath(x,y);else p.lineTo(x,y);} g.setColour(purple.withAlpha(0.25f));g.strokePath(p,juce::PathStrokeType(6.0f));g.setColour(cyan.withAlpha(0.8f));g.strokePath(p,juce::PathStrokeType(1.4f));
}

void VeloriaAudioProcessorEditor::drawStochasticGlobe(juce::Graphics& g,juce::Rectangle<float> b)
{
    const auto aw = parameterValue(audioProcessor.parameters,"ampWalk");
    const auto tw = parameterValue(audioProcessor.parameters,"timeWalk");
    const auto ampBarrier = parameterValue(audioProcessor.parameters,"ampMirror");
    const auto timeBarrier = parameterValue(audioProcessor.parameters,"timeMirror");
    const auto chaosValue = parameterValue(audioProcessor.parameters,"chaos");
    const auto ampDistValue = parameterValue(audioProcessor.parameters,"ampDist");
    const auto timeDistValue = parameterValue(audioProcessor.parameters,"timeDist");
    const auto energy = juce::jlimit(0.0f,1.0f,visualState.energy);
    const auto activity = juce::jlimit(0.0f,1.0f,0.10f+aw*0.24f+tw*0.22f+energy*0.18f+chaosValue*0.42f);

    auto globe=b.withSizeKeepingCentre(506.0f,506.0f);
    const auto c=globe.getCentre();
    const auto r=globe.getWidth()*0.455f;

    // Atmospheric bloom behind the sphere.
    juce::ColourGradient outerGlow(purple.withAlpha(0.20f+energy*0.18f),c.x,c.y,
                                    juce::Colours::transparentBlack,c.x+r*1.38f,c.y,true);
    outerGlow.addColour(0.34,magenta.withAlpha(0.14f+chaosValue*0.07f));
    outerGlow.addColour(0.68,cyan.withAlpha(0.035f));
    g.setGradientFill(outerGlow); g.fillEllipse(globe.expanded(36.0f));

    // Fine orbital cages outside the planet, deliberately subtle.
    for(int ring=0;ring<18;++ring)
    {
        const auto rr=r*(1.01f+ring*0.022f);
        const auto flatten=0.54f+0.025f*(float)(ring%8);
        const auto tilt=rotationPhase*(ring%2?-.31f:.23f)+ring*0.21f;
        juce::Path orbit;
        orbit.addCentredArc(c.x,c.y,rr,rr*flatten,tilt,0.05f,juce::MathConstants<float>::twoPi-0.05f,true);
        const auto col=(ring%7==0?gold:(ring%5==0?cyan:purple));
        g.setColour(col.withAlpha(0.025f+activity*0.035f));
        g.strokePath(orbit,juce::PathStrokeType(0.55f));
    }

    // Base planet body and clipped internal volume.
    juce::ColourGradient body(juce::Colour::fromRGB(38,12,62),c.x-r*0.65f,c.y-r*0.65f,
                              juce::Colour::fromRGB(2,3,9),c.x+r*0.78f,c.y+r*0.78f,true);
    body.addColour(0.34,juce::Colour::fromRGB(45,17,82));
    body.addColour(0.70,juce::Colour::fromRGB(10,8,28));
    g.setGradientFill(body); g.fillEllipse(globe);

    juce::Graphics::ScopedSaveState clipped(g);
    juce::Path sphereClip; sphereClip.addEllipse(globe);
    g.reduceClipRegion(sphereClip);

    // Luminous internal atmosphere: many overlapping low-alpha cloudlets.
    for(int cloud=0;cloud<90;++cloud)
    {
        const auto f=(float)cloud;
        const auto a=f*2.39996323f+rotationPhase*(0.11f+(cloud%9)*0.012f);
        const auto radial=0.18f+0.76f*std::sqrt((float)((cloud*37)%89)/89.0f);
        const auto px=c.x+std::cos(a)*r*radial;
        const auto py=c.y+std::sin(a)*r*radial*(0.58f+0.30f*std::sin(f*0.31f+rotationPhase));
        const auto size=r*(0.08f+0.12f*(0.5f+0.5f*std::sin(f*1.73f)));
        const auto cloudCol=(cloud%11==0?gold:(cloud%5==0?magenta:purple));
        juce::ColourGradient fog(cloudCol.withAlpha(0.012f+activity*0.010f),px,py,
                                 juce::Colours::transparentBlack,px+size,py,true);
        g.setGradientFill(fog); g.fillEllipse(px-size,py-size,size*2.0f,size*2.0f);
    }

    // Construct the actual breakpoint anchors from live amplitude/duration data.
    float total=0.0f;
    for(auto d:visualState.durations) total+=juce::jmax(0.001f,d);
    std::array<juce::Point<float>,VeloriaAudioProcessor::visualBreakpointCount> pts{};
    float cumulative=0.0f;
    for(std::size_t i=0;i<pts.size();++i)
    {
        const auto angle=rotationPhase-juce::MathConstants<float>::halfPi
                         +juce::MathConstants<float>::twoPi*(cumulative/juce::jmax(0.001f,total));
        cumulative+=juce::jmax(0.001f,visualState.durations[i]);
        const auto radial=r*(0.45f+visualState.amplitudes[i]*0.24f);
        const auto depth=0.70f+0.24f*std::sin(angle*2.0f+rotationPhase*0.61f+i*0.31f);
        pts[i]={c.x+std::cos(angle)*radial,c.y+std::sin(angle)*radial*depth};
    }

    // Hundreds of thin, curved stochastic filaments. They share the breakpoint field
    // but diverge into separate trajectories, which creates the reference-like web.
    for(int strand=0;strand<150;++strand)
    {
        const float sf=(float)strand;
        juce::Path filament;
        const int samples=42;
        for(int j=0;j<samples;++j)
        {
            const float u=(float)j/(float)(samples-1);
            const auto anchorIndex=(std::size_t)((strand*7+j/4)%pts.size());
            const auto nextIndex=(anchorIndex+1)%pts.size();
            const auto blend=std::fmod(u*3.0f+sf*0.037f,1.0f);
            auto base=pts[anchorIndex]+(pts[nextIndex]-pts[anchorIndex])*blend;
            const auto orbitAngle=u*juce::MathConstants<float>::twoPi*(1.0f+(strand%5)*0.21f)+sf*0.17f+rotationPhase*(0.35f+tw*0.8f);
            const auto swirl=r*(0.025f+0.105f*(0.5f+0.5f*std::sin(sf*0.43f))) * (0.45f+activity);
            base.x+=std::cos(orbitAngle)*swirl + std::sin(sf*1.13f+u*9.0f)*r*0.012f*ampBarrier;
            base.y+=std::sin(orbitAngle)*swirl*0.72f + std::cos(sf*0.77f+u*11.0f)*r*0.014f*timeBarrier;
            if(j==0) filament.startNewSubPath(base); else filament.lineTo(base);
        }
        const auto colour=(strand%17==0?gold:(strand%9==0?cyan:(strand%5==0?magenta:purple)));
        g.setColour(colour.withAlpha(0.018f+activity*0.028f));
        g.strokePath(filament,juce::PathStrokeType(0.38f+(strand%13==0?0.32f:0.0f),juce::PathStrokeType::curved));
    }

    // Deep particle volume. Particle projection changes with a pseudo-Z value so the
    // field reads as 3D rather than as dots painted on a disc.
    const int particleCount=2800;
    for(int i=0;i<particleCount;++i)
    {
        const float fi=(float)i;
        const auto z=std::sin(fi*0.619f+rotationPhase*(0.19f+tw*0.36f)+std::sin(fi*0.071f)*chaosValue);
        const auto radial=std::sqrt((float)((i*97)%2797)/2797.0f);
        const auto angle=fi*2.39996323f+rotationPhase*(0.11f+(i%23)*0.006f)
                        +std::sin(fi*0.031f+rotationPhase)*chaosValue*0.5f;
        const auto depthScale=0.64f+0.36f*(z*0.5f+0.5f);
        const auto turbulence=1.0f+0.12f*std::sin(fi*0.113f+rotationPhase*(1.0f+activity*2.0f));
        const auto pr=r*radial*depthScale*turbulence;
        const auto x=c.x+std::cos(angle)*pr;
        const auto y=c.y+std::sin(angle)*pr*(0.74f+0.20f*z);
        const auto near=(z+1.0f)*0.5f;
        const auto size=0.32f+near*1.15f+energy*0.65f+(i%131==0?1.4f:0.0f);
        auto col=(i%31==0?gold:(i%17==0?magenta:(i%9==0?cyan:purple)));
        const auto distBias=juce::jlimit(0.0f,1.0f,(ampDistValue+timeDistValue)/10.0f);
        if(i%47==0) col=col.interpolatedWith(gold,0.25f+0.45f*distBias);
        const auto alpha=0.018f+near*0.070f+energy*0.060f+chaosValue*0.025f;
        g.setColour(col.withAlpha(alpha));
        g.fillEllipse(x-size*0.5f,y-size*0.5f,size,size);
    }

    // Breakpoint-linked star streams: not a polygon, but particles flowing between anchors.
    for(std::size_t s=0;s<pts.size();++s)
    {
        const auto n=(s+1)%pts.size();
        const auto delta=pts[n]-pts[s];
        juce::Point<float> normal(-delta.y,delta.x);
        normal/=juce::jmax(1.0f,normal.getDistanceFromOrigin());
        for(int j=0;j<70;++j)
        {
            const auto phase=std::fmod((float)j/70.0f+rotationPhase*(0.009f+tw*0.020f)+s*0.071f,1.0f);
            auto p=pts[s]+delta*phase;
            const auto curl=std::sin(phase*juce::MathConstants<float>::twoPi*2.0f+s*0.8f+rotationPhase*2.2f);
            p+=normal*curl*(2.0f+r*0.045f*(activity+chaosValue));
            const auto spark=(j%19==0);
            const auto sz=spark?2.1f:0.55f+(j%4)*0.12f;
            const auto col=spark?gold:(s%3==0?magenta:purple);
            g.setColour(col.withAlpha(spark?0.34f:0.045f+activity*0.09f));
            g.fillEllipse(p.x-sz*0.5f,p.y-sz*0.5f,sz,sz);
        }
    }

    // Small local storms around the true breakpoint positions.
    for(std::size_t i=0;i<pts.size();++i)
    {
        const auto amp=std::abs(visualState.amplitudes[i]);
        const auto stormRadius=r*(0.030f+0.050f*amp+0.030f*chaosValue);
        const auto stormColour=(i%4==0?gold:(i%3==0?cyan:magenta));
        for(int p=0;p<26;++p)
        {
            const auto a=(float)p*2.39996323f+rotationPhase*(0.7f+0.09f*i);
            const auto rr=stormRadius*std::sqrt((float)(p+1)/26.0f);
            const auto q=pts[i]+juce::Point<float>(std::cos(a)*rr,std::sin(a)*rr*0.72f);
            const auto sz=0.55f+(p%7==0?1.2f:0.0f);
            g.setColour(stormColour.withAlpha(0.035f+amp*0.09f+energy*0.05f));
            g.fillEllipse(q.x-sz*0.5f,q.y-sz*0.5f,sz,sz);
        }
    }

    // Primary mathematical anchors remain readable but no longer dominate the planet.
    for(std::size_t i=0;i<pts.size();++i)
    {
        const auto amp=std::abs(visualState.amplitudes[i]);
        const auto nr=2.1f+amp*2.2f+energy*0.4f;
        const auto col=(i%4==0?gold:(i%3==0?cyan:magenta));
        g.setColour(col.withAlpha(0.06f+amp*0.06f));
        g.fillEllipse(pts[i].x-nr*3.0f,pts[i].y-nr*3.0f,nr*6.0f,nr*6.0f);
        g.setColour(col.withAlpha(0.86f));
        g.fillEllipse(pts[i].x-nr,pts[i].y-nr,nr*2.0f,nr*2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.82f));
        g.fillEllipse(pts[i].x-0.8f,pts[i].y-0.8f,1.6f,1.6f);
    }

    // Spherical lighting: gentle limb darkness, never enough to erase the internal field.
    juce::ColourGradient limb(juce::Colours::transparentBlack,c.x-r*0.42f,c.y-r*0.28f,
                              juce::Colours::black.withAlpha(0.34f),c.x+r*0.92f,c.y+r*0.82f,false);
    g.setGradientFill(limb); g.fillEllipse(globe);

    // Close the sphere with several hairline atmospheric rims.
    g.setColour(cyan.withAlpha(0.10f+energy*0.05f)); g.drawEllipse(globe,0.9f);
    g.setColour(purple.withAlpha(0.15f)); g.drawEllipse(globe.reduced(3.0f),0.65f);
    g.setColour(gold.withAlpha(0.055f+chaosValue*0.035f)); g.drawEllipse(globe.reduced(7.0f),0.50f);
}

void VeloriaAudioProcessorEditor::resized()
{
    title.setBounds(515,8,370,34);subtitle.setBounds(535,40,330,11);presetBox.setBounds(930,15,190,30);discoverButton.setBounds(1128,15,78,30);newFieldButton.setBounds(1213,15,84,30);monoButton.setBounds(1305,15,68,30);
    ampWalk.setBounds(28,108,64,242);timeWalk.setBounds(104,108,64,242);ampMirror.setBounds(180,108,64,242);timeMirror.setBounds(256,108,64,242);
    ampDist.setBounds(24,374,70,104);timeDist.setBounds(100,374,70,104);ampStep.setBounds(176,374,70,104);timeStep.setBounds(252,374,70,104);
    chaos.setBounds(24,490,70,104);breakpoints.setBounds(100,490,70,104);pitchStability.setBounds(176,490,70,104);curve.setBounds(252,490,70,104);orderButton.setBounds(25,600,68,20);
    attack.setBounds(24,680,116,150);decay.setBounds(142,680,116,150);sustain.setBounds(260,680,116,150);release.setBounds(378,680,116,150);
    seed.setBounds(548,700,86,92);fieldStatus.setBounds(646,716,148,35);presetNameEditor.setBounds(808,674,300,29);savePresetButton.setBounds(808,712,78,29);renamePresetButton.setBounds(894,712,82,29);deletePresetButton.setBounds(984,712,82,29);presetStatus.setBounds(808,754,300,47);voiceStatus.setBounds(1156,366,205,22);level.setBounds(1188,674,154,150);footerStatus.setBounds(415,872,570,14);
}
