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
    footerStatus.setText("LIVE STOCHASTIC PLANET  //  4 FIELD FADERS  //  8 STOCHASTIC DIMENSIONS  //  SEEDED MEMORY",juce::dontSendNotification); footerStatus.setJustificationType(juce::Justification::centred);
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

    // Fixed brand rule: LATHAMAUDIO is one word. Weight, not spacing, separates it.
    const juce::Font brandLight(juce::FontOptions(18.0f));
    const juce::Font brandBold(juce::FontOptions(18.0f,juce::Font::bold));
    const juce::String brandLeft("LATHAM");
    const juce::String brandRight("AUDIO");
    const auto leftWidth = static_cast<int>(std::ceil(brandLight.getStringWidthFloat(brandLeft)));
    g.setColour(juce::Colours::white.withAlpha(0.80f));
    g.setFont(brandLight); g.drawText(brandLeft,22,14,leftWidth+1,26,juce::Justification::centredLeft);
    g.setFont(brandBold); g.drawText(brandRight,22+leftWidth-1,14,72,26,juce::Justification::centredLeft);

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
    const auto aw=parameterValue(audioProcessor.parameters,"ampWalk"),tw=parameterValue(audioProcessor.parameters,"timeWalk"),am=parameterValue(audioProcessor.parameters,"ampMirror"),energy=juce::jlimit(0.0f,1.0f,visualState.energy); const auto chaosValue=parameterValue(audioProcessor.parameters,"chaos"); const auto motion=juce::jlimit(0.0f,1.0f,tw*0.7f+aw*0.2f+energy*0.1f+chaosValue*0.25f); const auto tension=juce::jlimit(0.0f,1.0f,aw*0.6f+am*0.25f+energy*0.15f+chaosValue*0.30f);
    auto globe=b.withSizeKeepingCentre(500.0f,500.0f);const auto c=globe.getCentre();const auto r=globe.getWidth()*0.455f;
    juce::ColourGradient halo(purple.withAlpha(0.25f+energy*0.22f),c.x,c.y,juce::Colours::transparentBlack,c.x,c.y+r*1.3f,true);halo.addColour(0.45,magenta.withAlpha(0.18f));g.setGradientFill(halo);g.fillEllipse(globe.expanded(25));
    juce::ColourGradient body(juce::Colour::fromRGB(35,14,57),c.x-r*.4f,c.y-r*.5f,juce::Colour::fromRGB(2,3,8),c.x+r*.8f,c.y+r*.8f,true);g.setGradientFill(body);g.fillEllipse(globe);

    for(int ring=0;ring<12;++ring){const auto rr=r*(0.95f+ring*0.035f);juce::Path p;p.addCentredArc(c.x,c.y,rr,rr*(0.67f+ring*.012f),rotationPhase*(ring%2?-.7f:1.0f),0.08f,juce::MathConstants<float>::twoPi-.08f,true);g.setColour((ring%4==0?gold:(ring%3==0?cyan:purple)).withAlpha(0.05f+motion*.06f));g.strokePath(p,juce::PathStrokeType(.7f));}

    for(int i=0;i<1350;++i){const auto fi=(float)i;const auto phase=fi*2.39996323f+rotationPhase*(0.10f+(i%17)*0.011f);const auto radial=std::sqrt((float)((i*73)%1349)/1349.0f);const auto depth=0.56f+0.42f*std::sin(fi*.23f+rotationPhase*(0.6f+motion));const auto pr=r*radial*(0.82f+0.34f*std::sin(fi*.071f));const auto x=c.x+std::cos(phase)*pr;const auto y=c.y+std::sin(phase)*pr*depth;const auto z=std::sin(phase*.73f+fi*.017f);const auto sz=.35f+(z+1.0f)*.45f+energy*.7f;const auto col=i%19==0?gold:(i%11==0?magenta:(i%5==0?cyan:purple));g.setColour(col.withAlpha(.025f+(z+1.0f)*.025f+energy*.12f+chaosValue*.03f));g.fillEllipse(x-sz*.5f,y-sz*.5f,sz,sz);}

    float total=0;for(auto d:visualState.durations)total+=juce::jmax(.001f,d);std::array<juce::Point<float>,VeloriaAudioProcessor::visualBreakpointCount> pts{};float cum=0;for(std::size_t i=0;i<pts.size();++i){const auto angle=rotationPhase-juce::MathConstants<float>::halfPi+juce::MathConstants<float>::twoPi*(cum/juce::jmax(.001f,total));cum+=juce::jmax(.001f,visualState.durations[i]);const auto rad=r*(.50f+visualState.amplitudes[i]*.31f);pts[i]={c.x+std::cos(angle)*rad,c.y+std::sin(angle)*rad*(.72f+.28f*std::sin(angle*2+rotationPhase*.55f))};}

    for(int h=0;h<28;++h){juce::Path trace;const auto off=(h-14)*.7f;for(std::size_t s=0;s<pts.size();++s){const auto n=(s+1)%pts.size();auto tangent=pts[n]-pts[s];juce::Point<float> normal(-tangent.y,tangent.x);const auto len=juce::jmax(1.0f,normal.getDistanceFromOrigin());normal/=len;const auto p=pts[s]+normal*(off+std::sin(rotationPhase*.6f+h*.17f+s*.65f)*(2.0f+motion*5.0f));if(s==0)trace.startNewSubPath(p);else trace.lineTo(p);}trace.closeSubPath();g.setColour((h%7==0?gold:(h%4==0?cyan:purple)).withAlpha(.022f+energy*.035f+chaosValue*.018f));g.strokePath(trace,juce::PathStrokeType(.55f));}

    for(std::size_t s=0;s<pts.size();++s){const auto n=(s+1)%pts.size();for(int j=0;j<42;++j){const auto t=std::fmod((float)j/42.0f+rotationPhase*(.018f+motion*.035f+s*.001f),1.0f);auto p=pts[s]+(pts[n]-pts[s])*t;auto tangent=pts[n]-pts[s];juce::Point<float> normal(-tangent.y,tangent.x);normal/=juce::jmax(1.0f,normal.getDistanceFromOrigin());p+=normal*std::sin(t*14+s+rotationPhase*2.7f)*(1.7f+energy*7.0f+chaosValue*6.0f);const auto sz=.5f+energy*1.2f+(j%3)*.18f;g.setColour((j%8==0?gold:purple).interpolatedWith(cyan,tw*.35f).withAlpha(.07f+energy*.28f+chaosValue*.06f));g.fillEllipse(p.x-sz*.5f,p.y-sz*.5f,sz,sz);}}

    for(std::size_t i=0;i<pts.size();++i){const auto a=std::abs(visualState.amplitudes[i]);const auto nr=3.0f+a*4.0f+energy;const auto col=(i%3==0?gold:purple).interpolatedWith(magenta,a*.4f);g.setColour(col.withAlpha(.14f));g.fillEllipse(pts[i].x-nr*3,pts[i].y-nr*3,nr*6,nr*6);g.setColour(col.withAlpha(.96f));g.fillEllipse(pts[i].x-nr,pts[i].y-nr,nr*2,nr*2);g.setColour(juce::Colours::white.withAlpha(.9f));g.fillEllipse(pts[i].x-1,pts[i].y-1,2,2);}
    juce::ColourGradient shadow(juce::Colours::transparentBlack,c.x-r*.25f,c.y,juce::Colours::black.withAlpha(.42f),c.x+r*.9f,c.y,false);g.setGradientFill(shadow);g.fillEllipse(globe);g.setColour(cyan.withAlpha(.12f+energy*.08f));g.drawEllipse(globe,1.2f);
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
