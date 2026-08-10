#include "PluginEditor.h"
#include <array>
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
    const float gap = h * 0.16f;
    const float totalW = glyphW * 7.0f + gap * 6.0f;
    const float x0 = area.getCentreX() - totalW * 0.5f;
    const float top = area.getY() + h * 0.10f;
    const float mid = area.getY() + h * 0.50f;
    const float bot = area.getY() + h * 0.90f;

    g.setColour(juce::Colours::white.withAlpha(0.94f));
    juce::Path p;
    auto X = [=](int i) { return x0 + (glyphW + gap) * (float) i; };

    p.startNewSubPath(X(0), top); p.lineTo(X(0) + glyphW * 0.50f, bot); p.lineTo(X(0) + glyphW, top);
    p.startNewSubPath(X(1) + glyphW, top); p.lineTo(X(1), top); p.lineTo(X(1), bot); p.lineTo(X(1) + glyphW, bot);
    p.startNewSubPath(X(1), mid); p.lineTo(X(1) + glyphW * 0.72f, mid);
    p.startNewSubPath(X(2), top); p.lineTo(X(2), bot); p.lineTo(X(2) + glyphW, bot);
    p.startNewSubPath(X(3) + glyphW * 0.22f, top); p.lineTo(X(3) + glyphW * 0.78f, top); p.lineTo(X(3) + glyphW, top + h * 0.22f);
    p.lineTo(X(3) + glyphW, bot - h * 0.22f); p.lineTo(X(3) + glyphW * 0.78f, bot); p.lineTo(X(3) + glyphW * 0.22f, bot);
    p.lineTo(X(3), bot - h * 0.22f); p.lineTo(X(3), top + h * 0.22f); p.closeSubPath();
    p.startNewSubPath(X(4), bot); p.lineTo(X(4), top); p.lineTo(X(4) + glyphW * 0.68f, top); p.lineTo(X(4) + glyphW, top + h * 0.18f);
    p.lineTo(X(4) + glyphW, mid - h * 0.06f); p.lineTo(X(4) + glyphW * 0.68f, mid + h * 0.05f); p.lineTo(X(4), mid + h * 0.05f);
    p.startNewSubPath(X(4) + glyphW * 0.53f, mid + h * 0.05f); p.lineTo(X(4) + glyphW, bot);
    p.startNewSubPath(X(5) + glyphW * 0.5f, top); p.lineTo(X(5) + glyphW * 0.5f, bot);
    p.startNewSubPath(X(6), bot); p.lineTo(X(6) + glyphW * 0.50f, top); p.lineTo(X(6) + glyphW, bot);
    p.startNewSubPath(X(6) + glyphW * 0.24f, mid + h * 0.08f); p.lineTo(X(6) + glyphW * 0.76f, mid + h * 0.08f);

    g.strokePath(p, juce::PathStrokeType(stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void drawMiniRandomWalk(juce::Graphics& g, juce::Rectangle<float> area, juce::Colour colour, int seed)
{
    juce::Path p;
    float x = area.getX(), y = area.getCentreY();
    p.startNewSubPath(x, y);
    for (int i = 1; i <= 28; ++i)
    {
        x = area.getX() + area.getWidth() * (float)i / 28.0f;
        y += (hash01(seed + i * 17) - 0.5f) * area.getHeight() * 0.28f;
        y = juce::jlimit(area.getY() + 4.0f, area.getBottom() - 4.0f, y);
        p.lineTo(x, y);
    }
    g.setColour(colour.withAlpha(0.82f));
    g.strokePath(p, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
}

void VeloriaAudioProcessorEditor::AuroraLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
    float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>((float)x,(float)y,(float)width,(float)height);
    auto label = bounds.removeFromTop(17.0f).toNearestInt();
    g.setColour(juce::Colours::white.withAlpha(0.76f)); g.setFont(juce::FontOptions(8.2f,juce::Font::bold));
    g.drawFittedText(slider.getName(),label,juce::Justification::centred,1);
    bounds=bounds.reduced(6.0f,2.0f); const auto size=juce::jmin(bounds.getWidth(),bounds.getHeight()); auto dial=bounds.withSizeKeepingCentre(size,size);
    const auto centre=dial.getCentre(); const auto radius=size*0.43f; const auto angle=rotaryStartAngle+sliderPosProportional*(rotaryEndAngle-rotaryStartAngle);
    juce::ColourGradient glow(purple.withAlpha(0.28f),centre.x,centre.y,juce::Colours::transparentBlack,centre.x+radius*1.3f,centre.y,true); glow.addColour(0.5,magenta.withAlpha(0.18f));
    g.setGradientFill(glow); g.fillEllipse(dial.expanded(6.0f)); g.setColour(juce::Colour::fromRGB(17,18,25)); g.fillEllipse(dial); g.setColour(juce::Colours::white.withAlpha(0.10f)); g.drawEllipse(dial,1.0f);
    juce::Path arc; arc.addCentredArc(centre.x,centre.y,radius,radius,0.0f,rotaryStartAngle,angle,true);
    juce::ColourGradient grad(purple,dial.getX(),centre.y,gold,dial.getRight(),centre.y,false); grad.addColour(0.55,magenta); g.setGradientFill(grad);
    g.strokePath(arc,juce::PathStrokeType(3.4f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    juce::Path pointer; pointer.addRoundedRectangle(-1.0f,-radius*0.72f,2.0f,radius*0.42f,1.0f); g.setColour(juce::Colours::white.withAlpha(0.92f));
    g.fillPath(pointer,juce::AffineTransform::rotation(angle).translated(centre.x,centre.y));
}

void VeloriaAudioProcessorEditor::AuroraLookAndFeel::drawLinearSlider(juce::Graphics& g,int x,int y,int width,int height,float sliderPos,float,float,juce::Slider::SliderStyle style,juce::Slider& slider)
{
    if(style!=juce::Slider::LinearVertical){juce::LookAndFeel_V4::drawLinearSlider(g,x,y,width,height,sliderPos,0,0,style,slider);return;}
    auto b=juce::Rectangle<float>((float)x,(float)y,(float)width,(float)height); auto label=b.removeFromTop(18.0f).toNearestInt();
    g.setColour(juce::Colours::white.withAlpha(0.80f)); g.setFont(juce::FontOptions(8.0f,juce::Font::bold)); g.drawFittedText(slider.getName(),label,juce::Justification::centred,2);
    const float trackX=b.getCentreX(),top=b.getY()+8.0f,bottom=b.getBottom()-22.0f; g.setColour(juce::Colours::white.withAlpha(0.08f)); g.fillRoundedRectangle(trackX-3,top,6,bottom-top,3);
    g.setColour(purple.withAlpha(0.55f)); g.fillRoundedRectangle(trackX-2,sliderPos,4,bottom-sliderPos,2); g.setColour(magenta.withAlpha(0.95f)); g.fillRoundedRectangle(trackX-14,sliderPos-4,28,8,4);
}

void VeloriaAudioProcessorEditor::MidiLearnSlider::mouseDown(const juce::MouseEvent& e)
{
    if(!e.mods.isPopupMenu()){juce::Slider::mouseDown(e);return;} const auto currentCC=currentCCCallback?currentCCCallback():-1; juce::PopupMenu menu;
    menu.addItem(1,currentCC>=0?"Relearn MIDI CC (CC "+juce::String(currentCC)+")":"Learn MIDI CC"); menu.addItem(2,"Clear MIDI CC",currentCC>=0);
    juce::Component::SafePointer<MidiLearnSlider> safe(this); menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),[safe](int r){if(safe==nullptr)return;if(r==1&&safe->learnCallback)safe->learnCallback();if(r==2&&safe->clearCallback)safe->clearCallback();});
}

VeloriaAudioProcessorEditor::VeloriaAudioProcessorEditor(VeloriaAudioProcessor& p):AudioProcessorEditor(&p),audioProcessor(p)
{
    setSize(1400,900);
    title.setText({},juce::dontSendNotification); title.setColour(juce::Label::textColourId,juce::Colours::transparentBlack); addAndMakeVisible(title);
    subtitle.setText("DYNAMIC STOCHASTIC SYNTHESIS",juce::dontSendNotification); subtitle.setJustificationType(juce::Justification::centred); subtitle.setFont(juce::FontOptions(7.8f)); subtitle.setColour(juce::Label::textColourId,juce::Colours::white.withAlpha(0.38f)); addAndMakeVisible(subtitle);

    presetBox.setColour(juce::ComboBox::backgroundColourId,panel2); presetBox.setColour(juce::ComboBox::outlineColourId,purple.withAlpha(0.38f)); presetBox.setColour(juce::ComboBox::textColourId,juce::Colours::white);
    presetBox.onChange=[this]{const auto id=presetBox.getSelectedId();const auto fc=audioProcessor.getFactoryPresetNames().size();if(id>0&&id<=fc){audioProcessor.setCurrentProgram(id-1);presetNameEditor.setText(presetBox.getText()+" Copy",juce::dontSendNotification);}else if(id>=firstUserPresetId)audioProcessor.loadUserPreset(presetBox.getText());}; addAndMakeVisible(presetBox); refreshPresetBox();
    discoverButton.onClick=[this]{audioProcessor.discover();presetBox.setText("Discovered",juce::dontSendNotification);}; newFieldButton.onClick=[this]{audioProcessor.newField();presetBox.setText("Field Variation",juce::dontSendNotification);};
    whatIfButton.setColour(juce::TextButton::buttonColourId,gold.withAlpha(0.20f)); whatIfButton.setColour(juce::TextButton::textColourOffId,gold.withAlpha(0.95f)); whatIfButton.onClick=[this]{setWhatIfMode(!whatIfOpen);};
    for(auto* b:{&discoverButton,&newFieldButton,&savePresetButton,&renamePresetButton}){b->setColour(juce::TextButton::buttonColourId,purple.withAlpha(0.24f));b->setColour(juce::TextButton::textColourOffId,juce::Colours::white);addAndMakeVisible(b);} addAndMakeVisible(deletePresetButton); addAndMakeVisible(whatIfButton);
    monoButton.setClickingTogglesState(true); monoButton.setColour(juce::ToggleButton::textColourId,juce::Colours::white); monoButton.setColour(juce::ToggleButton::tickColourId,cyan); addAndMakeVisible(monoButton);
    orderButton.setColour(juce::TextButton::buttonColourId,purple.withAlpha(0.22f)); orderButton.setColour(juce::TextButton::textColourOffId,juce::Colours::white.withAlpha(0.86f));
    orderButton.onClick=[this]{auto* raw=audioProcessor.parameters.getRawParameterValue("walkOrder");auto* param=audioProcessor.parameters.getParameter("walkOrder");if(!raw||!param)return;const auto next=raw->load()>=1.5f?1.0f:2.0f;param->beginChangeGesture();param->setValueNotifyingHost(param->convertTo0to1(next));param->endChangeGesture();refreshOrderButton();}; addAndMakeVisible(orderButton); refreshOrderButton();

    configureFieldSlider(ampWalk,"ampWalk","AMP WALK"); configureFieldSlider(timeWalk,"timeWalk","TIME WALK"); configureFieldSlider(ampMirror,"ampMirror","AMP BARRIER"); configureFieldSlider(timeMirror,"timeMirror","TIME BARRIER");
    configureKnob(ampDist,"ampDist","AMP DIST"); configureKnob(timeDist,"timeDist","TIME DIST"); configureKnob(ampStep,"ampStep","AMP STEP"); configureKnob(timeStep,"timeStep","TIME STEP"); configureKnob(chaos,"chaos","CHAOS"); configureKnob(breakpoints,"breakpoints","POINTS");
    configureKnob(boundary,"boundary","BOUNDARY"); configureKnob(rate,"rate","RATE"); configureKnob(jump,"jump","JUMP"); configureKnob(correlation,"correlation","CORRELATION"); configureKnob(pitchStability,"pitchStability","PITCH LOCK"); configureKnob(curve,"curve","CURVE");
    configureKnob(attack,"attack","ATTACK"); configureKnob(decay,"decay","DECAY"); configureKnob(sustain,"sustain","SUSTAIN"); configureKnob(release,"release","RELEASE"); configureKnob(seed,"seed","FIELD SEED"); configureKnob(level,"level","LEVEL");
    for(auto* s:{&ampWalk,&timeWalk,&ampMirror,&timeMirror,&ampDist,&timeDist,&ampStep,&timeStep,&chaos,&breakpoints,&pitchStability,&curve,&boundary,&rate,&jump,&correlation,&attack,&decay,&sustain,&release,&seed,&level}){s->setLookAndFeel(&auroraLookAndFeel);addAndMakeVisible(s);}
    for(auto* s:{&ampWalk,&timeWalk,&ampMirror,&timeMirror,&ampStep,&timeStep,&chaos,&boundary,&rate,&jump,&correlation,&pitchStability,&curve,&attack,&decay,&release})s->setNumDecimalPlacesToDisplay(2);
    sustain.setNumDecimalPlacesToDisplay(2); ampDist.setNumDecimalPlacesToDisplay(0); timeDist.setNumDecimalPlacesToDisplay(0); breakpoints.setNumDecimalPlacesToDisplay(0); seed.setNumDecimalPlacesToDisplay(0); level.setNumDecimalPlacesToDisplay(1); level.setTextValueSuffix(" dB");

    ampWalkAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"ampWalk",ampWalk); timeWalkAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"timeWalk",timeWalk); ampMirrorAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"ampMirror",ampMirror); timeMirrorAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"timeMirror",timeMirror);
    ampDistAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"ampDist",ampDist); timeDistAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"timeDist",timeDist); ampStepAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"ampStep",ampStep); timeStepAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"timeStep",timeStep); chaosAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"chaos",chaos); boundaryAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"boundary",boundary); rateAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"rate",rate); jumpAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"jump",jump); correlationAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"correlation",correlation); breakpointsAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"breakpoints",breakpoints); pitchStabilityAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"pitchStability",pitchStability); curveAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"curve",curve);
    attackAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"attack",attack); decayAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"decay",decay); sustainAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"sustain",sustain); releaseAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"release",release); seedAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"seed",seed); levelAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,"level",level); monoAttachment=std::make_unique<ButtonAttachment>(audioProcessor.parameters,"mono",monoButton);

    presetNameEditor.setTextToShowWhenEmpty("Name preset...",juce::Colours::white.withAlpha(0.28f)); presetNameEditor.setColour(juce::TextEditor::backgroundColourId,panel2); presetNameEditor.setColour(juce::TextEditor::textColourId,juce::Colours::white); addAndMakeVisible(presetNameEditor);
    savePresetButton.onClick=[this]{if(audioProcessor.saveUserPreset(presetNameEditor.getText().trim()))refreshPresetBox(presetNameEditor.getText().trim());}; renamePresetButton.onClick=[this]{if(selectedPresetIsUser()&&audioProcessor.renameUserPreset(presetBox.getText(),presetNameEditor.getText().trim()))refreshPresetBox(presetNameEditor.getText().trim());}; deletePresetButton.onClick=[this]{if(selectedPresetIsUser()&&audioProcessor.deleteUserPreset(presetBox.getText()))refreshPresetBox();};
    for(auto* l:{&fieldStatus,&voiceStatus,&presetStatus,&footerStatus})addAndMakeVisible(l);
    voiceStatus.setColour(juce::Label::textColourId,juce::Colours::white.withAlpha(0.62f)); fieldStatus.setColour(juce::Label::textColourId,juce::Colours::white.withAlpha(0.55f)); presetStatus.setColour(juce::Label::textColourId,juce::Colours::white.withAlpha(0.40f)); footerStatus.setColour(juce::Label::textColourId,juce::Colours::white.withAlpha(0.25f)); footerStatus.setText("LIVE STOCHASTIC PLANET  //  BREAKPOINT GEOMETRY  //  ORBITS  //  DURATION FIELD  //  STORMS",juce::dontSendNotification); footerStatus.setJustificationType(juce::Justification::centred);
    visualState=audioProcessor.getVisualState(); startTimerHz(30);
}

VeloriaAudioProcessorEditor::~VeloriaAudioProcessorEditor(){for(auto* s:{&ampWalk,&timeWalk,&ampMirror,&timeMirror,&ampDist,&timeDist,&ampStep,&timeStep,&chaos,&breakpoints,&pitchStability,&curve,&boundary,&rate,&jump,&correlation,&attack,&decay,&sustain,&release,&seed,&level})s->setLookAndFeel(nullptr);}

void VeloriaAudioProcessorEditor::setWhatIfMode(bool shouldOpen)
{
    whatIfOpen=shouldOpen; whatIfButton.setButtonText(whatIfOpen?"BACK":"WHAT IF?");
    for(auto* c:{static_cast<juce::Component*>(&presetBox),static_cast<juce::Component*>(&monoButton),static_cast<juce::Component*>(&orderButton),static_cast<juce::Component*>(&discoverButton),static_cast<juce::Component*>(&newFieldButton),static_cast<juce::Component*>(&savePresetButton),static_cast<juce::Component*>(&renamePresetButton),static_cast<juce::Component*>(&deletePresetButton),static_cast<juce::Component*>(&presetNameEditor),static_cast<juce::Component*>(&fieldStatus),static_cast<juce::Component*>(&voiceStatus),static_cast<juce::Component*>(&footerStatus),static_cast<juce::Component*>(&presetStatus),static_cast<juce::Component*>(&ampWalk),static_cast<juce::Component*>(&timeWalk),static_cast<juce::Component*>(&ampMirror),static_cast<juce::Component*>(&timeMirror),static_cast<juce::Component*>(&ampDist),static_cast<juce::Component*>(&timeDist),static_cast<juce::Component*>(&ampStep),static_cast<juce::Component*>(&timeStep),static_cast<juce::Component*>(&chaos),static_cast<juce::Component*>(&boundary),static_cast<juce::Component*>(&rate),static_cast<juce::Component*>(&jump),static_cast<juce::Component*>(&correlation),static_cast<juce::Component*>(&breakpoints),static_cast<juce::Component*>(&pitchStability),static_cast<juce::Component*>(&curve),static_cast<juce::Component*>(&attack),static_cast<juce::Component*>(&decay),static_cast<juce::Component*>(&sustain),static_cast<juce::Component*>(&release),static_cast<juce::Component*>(&seed),static_cast<juce::Component*>(&level)})c->setVisible(!whatIfOpen); repaint();
}

void VeloriaAudioProcessorEditor::configureKnob(MidiLearnSlider& s,const juce::String& id,const juce::String& name){s.setName(name);s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,58,15);s.learnCallback=[this,id,name]{audioProcessor.beginMidiLearn(id);presetStatus.setText(name+" - MOVE A MIDI CC",juce::dontSendNotification);};s.clearCallback=[this,id]{audioProcessor.clearMidiMapping(id);};s.currentCCCallback=[this,id]{return audioProcessor.getMidiCCForParameter(id);};}
void VeloriaAudioProcessorEditor::configureFieldSlider(MidiLearnSlider& s,const juce::String& id,const juce::String& name){s.setName(name);s.setSliderStyle(juce::Slider::LinearVertical);s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,56,15);s.learnCallback=[this,id,name]{audioProcessor.beginMidiLearn(id);presetStatus.setText(name+" - MOVE A MIDI CC",juce::dontSendNotification);};s.clearCallback=[this,id]{audioProcessor.clearMidiMapping(id);};s.currentCCCallback=[this,id]{return audioProcessor.getMidiCCForParameter(id);};}
void VeloriaAudioProcessorEditor::refreshOrderButton(){const auto order=parameterValue(audioProcessor.parameters,"walkOrder")>=1.5f?2:1;orderButton.setButtonText("ORDER "+juce::String(order));}
void VeloriaAudioProcessorEditor::refreshPresetBox(const juce::String& select){presetBox.clear(juce::dontSendNotification);presetBox.addSectionHeading("FACTORY");const auto factory=audioProcessor.getFactoryPresetNames();for(int i=0;i<factory.size();++i)presetBox.addItem(factory[i],i+1);const auto user=audioProcessor.getUserPresetNames();if(!user.isEmpty()){presetBox.addSeparator();presetBox.addSectionHeading("USER PRESETS");for(int i=0;i<user.size();++i)presetBox.addItem(user[i],firstUserPresetId+i);}if(select.isNotEmpty()){const auto index=user.indexOf(select);if(index>=0)presetBox.setSelectedId(firstUserPresetId+index,juce::dontSendNotification);}else{const auto p=audioProcessor.getCurrentProgram();if(p>=0&&p<factory.size())presetBox.setSelectedId(p+1,juce::dontSendNotification);}}
bool VeloriaAudioProcessorEditor::selectedPresetIsUser() const noexcept{return presetBox.getSelectedId()>=firstUserPresetId;}

void VeloriaAudioProcessorEditor::timerCallback()
{
    visualState=audioProcessor.getVisualState(); rotationPhase+=0.0022f+visualState.energy*0.0095f; if(rotationPhase>juce::MathConstants<float>::twoPi)rotationPhase-=juce::MathConstants<float>::twoPi;
    voiceStatus.setText("VOICE ENGINE   "+juce::String(visualState.activeVoices)+" / 8 ACTIVE",juce::dontSendNotification); fieldStatus.setText(juce::String(visualState.activeVoices)+" VOICES  //  "+juce::String((int)(visualState.energy*100.0f))+"% ENERGY",juce::dontSendNotification);
    const auto drums=audioProcessor.isDrumMode(); for(auto* e:{&attack,&decay,&sustain,&release}){e->setEnabled(!drums);e->setAlpha(drums?0.30f:1.0f);} if(drums)presetStatus.setText("DRUM MODE: STOCHASTIC ONE-SHOT ENVELOPES",juce::dontSendNotification); else if(presetStatus.getText().startsWith("DRUM MODE"))presetStatus.setText({},juce::dontSendNotification);
    refreshOrderButton(); repaint();
}

void VeloriaAudioProcessorEditor::drawPanel(juce::Graphics& g,juce::Rectangle<float> b,const juce::String& titleText){g.setColour(panel.withAlpha(0.92f));g.fillRoundedRectangle(b,9.0f);g.setColour(juce::Colours::white.withAlpha(0.07f));g.drawRoundedRectangle(b,9.0f,1.0f);g.setColour(juce::Colours::white.withAlpha(0.62f));g.setFont(9.0f);g.drawText(titleText,b.toNearestInt().reduced(10,7).removeFromTop(15),juce::Justification::centredLeft);}

void VeloriaAudioProcessorEditor::drawWhatIfOverlay(juce::Graphics& g)
{
    auto content=getLocalBounds().toFloat().reduced(16.0f); content.removeFromTop(58.0f); g.setColour(juce::Colour::fromRGB(7,6,11).withAlpha(0.98f));g.fillRoundedRectangle(content,12.0f);g.setColour(gold.withAlpha(0.22f));g.drawRoundedRectangle(content,12.0f,1.0f);
    auto inner=content.reduced(22.0f); auto heading=inner.removeFromTop(76.0f); g.setColour(gold.withAlpha(0.92f));g.setFont(juce::FontOptions(25.0f,juce::Font::bold));g.drawText("THE EVOLUTION OF STOCHASTIC SYNTHESIS  /  1970-1990",heading.removeFromTop(34.0f).toNearestInt(),juce::Justification::centredLeft);
    g.setColour(juce::Colours::white.withAlpha(0.62f));g.setFont(juce::FontOptions(11.0f));g.drawFittedText("WHAT IF XENAKIS' DYNAMIC STOCHASTIC SYNTHESIS HAD BECOME A COMMERCIAL INSTRUMENT FAMILY - EVOLVING BESIDE MOOG, ROLAND AND THE MACHINES THAT CREATED CLUB CULTURE?",heading.toNearestInt(),juce::Justification::centredLeft,2);
    struct Era{const char* year;const char* name;const char* tag;const char* story;juce::Colour colour;};
    const std::array<Era,4> eras{{
        {"1970s","THE XENAKIS SYSTEM 55","MODULAR BEHEMOTH","A wall-sized stochastic modular replaces oscillators and filters with 2D random walks. Breakpoints, Barrier Ceilings and probability distributions become the sound-shaping language.",gold},
        {"1980","THE GENDY-D LEAD","PORTABLE MONOSYNTH","The laboratory becomes playable. Barrier Width replaces cutoff; Cauchy Step Size becomes the edge control; Walk Order switches between first-order instability and second-order pitch-centred motion.",magenta},
        {"1982","THE STOCHASTIC-303","ACID BOX","Amplitude Barrier and Duration Mutation replace familiar filter controls. Slide becomes a probability rule while heavy-tailed mutations tear the waveform into stochastic acid.",purple},
        {"1984","THE STOCHASTIC-909","CLUB DRUM MACHINE","Kick contraction and heavy-tailed beater bursts meet a two-engine snare: stable body plus eruptive wire layer. Club percussion emerges from controlled statistical impact.",cyan}
    }};
    const float gap=12.0f,eraH=(inner.getHeight()-gap*3.0f)/4.0f;
    for(int i=0;i<4;++i){auto row=inner.removeFromTop(eraH);if(i<3)inner.removeFromTop(gap);const auto& e=eras[(std::size_t)i];g.setColour(panel2.withAlpha(0.92f));g.fillRoundedRectangle(row,8.0f);g.setColour(e.colour.withAlpha(0.24f));g.drawRoundedRectangle(row,8.0f,1.0f);auto r=row.reduced(14.0f);auto yearBox=r.removeFromLeft(105.0f);g.setColour(e.colour.withAlpha(0.14f));g.fillRoundedRectangle(yearBox,6.0f);g.setColour(e.colour.withAlpha(0.95f));g.setFont(juce::FontOptions(20.0f,juce::Font::bold));g.drawText(e.year,yearBox.toNearestInt(),juce::Justification::centred);r.removeFromLeft(14.0f);auto visual=r.removeFromRight(310.0f);r.removeFromRight(14.0f);auto nameLine=r.removeFromTop(26.0f);g.setColour(juce::Colours::white.withAlpha(0.92f));g.setFont(juce::FontOptions(16.0f,juce::Font::bold));g.drawText(e.name,nameLine.toNearestInt(),juce::Justification::centredLeft);auto tagLine=r.removeFromTop(18.0f);g.setColour(e.colour.withAlpha(0.78f));g.setFont(juce::FontOptions(9.5f,juce::Font::bold));g.drawText(e.tag,tagLine.toNearestInt(),juce::Justification::centredLeft);r.removeFromTop(4.0f);g.setColour(juce::Colours::white.withAlpha(0.62f));g.setFont(juce::FontOptions(10.5f));g.drawFittedText(e.story,r.toNearestInt(),juce::Justification::topLeft,4);
        auto v=visual.reduced(6.0f);g.setColour(juce::Colour::fromRGB(3,4,8).withAlpha(0.9f));g.fillRoundedRectangle(v,6.0f);g.setColour(e.colour.withAlpha(0.18f));g.drawRoundedRectangle(v,6.0f,1.0f);
        if(i==0){for(int m=0;m<7;++m){auto module=juce::Rectangle<float>(v.getX()+8.0f+m*40.0f,v.getY()+9.0f,33.0f,v.getHeight()-18.0f);g.setColour(juce::Colour::fromRGB(70,55,42));g.fillRoundedRectangle(module,2.0f);g.setColour(gold.withAlpha(0.35f));g.drawRoundedRectangle(module,2.0f,0.7f);for(int k=0;k<4;++k){g.setColour((k%2?purple:gold).withAlpha(0.55f));g.fillEllipse(module.getX()+6+(k%2)*14,module.getY()+12+(k/2)*25,3,3);}}drawMiniRandomWalk(g,v.reduced(18.0f,26.0f),gold,100);}
        else if(i==1){auto synth=v.reduced(12.0f,16.0f);auto keys=synth.removeFromBottom(28.0f);g.setColour(juce::Colour::fromRGB(88,63,42));g.fillRoundedRectangle(synth,4.0f);for(int k=0;k<14;++k){auto key=juce::Rectangle<float>(keys.getX()+k*keys.getWidth()/14.0f,keys.getY(),keys.getWidth()/14.0f-1.0f,keys.getHeight());g.setColour(k%7==1||k%7==4?juce::Colours::black:juce::Colours::ivory);g.fillRect(key);}auto knob=synth.withSizeKeepingCentre(54.0f,54.0f);g.setColour(magenta.withAlpha(.2f));g.fillEllipse(knob.expanded(7));g.setColour(juce::Colour::fromRGB(34,28,30));g.fillEllipse(knob);g.setColour(magenta);g.drawEllipse(knob,3.0f);}
        else if(i==2){auto box=v.reduced(12.0f,18.0f);g.setColour(juce::Colour::fromRGB(178,176,166));g.fillRoundedRectangle(box,5.0f);for(int k=0;k<8;++k){auto x=box.getX()+15+k*32.0f;g.setColour(juce::Colour::fromRGB(50,48,46));g.fillRoundedRectangle(x,box.getBottom()-35,18,24,2);g.setColour(purple.withAlpha(.85f));g.fillRect(x+7.0f,box.getY()+12.0f,4.0f,35.0f+hash01(k)*18.0f);}}
        else{auto drum=v.reduced(10.0f,14.0f);g.setColour(juce::Colour::fromRGB(205,193,165));g.fillRoundedRectangle(drum,4.0f);for(int k=0;k<16;++k){auto x=drum.getX()+7+k*(drum.getWidth()-14)/16.0f;g.setColour(k%4==0?orange:juce::Colour::fromRGB(168,86,36));g.fillRect(x,drum.getBottom()-20.0f,8.0f,11.0f);}drawMiniRandomWalk(g,drum.reduced(15.0f).removeFromTop(42.0f),cyan,320);}
    }
    g.setColour(juce::Colours::white.withAlpha(0.34f));g.setFont(juce::FontOptions(8.5f,juce::Font::bold));g.drawText("2026  //  VELORIA - THE INSTRUMENT FROM THIS SYNTHESIS HISTORY THAT NEVER HAPPENED.",(int)content.getX()+20,(int)content.getBottom()-24,(int)content.getWidth()-40,16,juce::Justification::centred);
}

void VeloriaAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);juce::ColourGradient bg(purple.withAlpha(0.09f),700,280,background,700,900,true);g.setGradientFill(bg);g.fillRect(getLocalBounds());g.setColour(juce::Colours::white.withAlpha(0.06f));g.drawRoundedRectangle(getLocalBounds().reduced(6).toFloat(),13,1);g.setColour(purple.withAlpha(0.28f));g.drawLine(14,62,1386,62,1);
    const juce::Font brandLight(juce::FontOptions(18.0f));const juce::Font brandBold(juce::FontOptions(18.0f,juce::Font::bold));g.setColour(juce::Colours::white.withAlpha(0.80f));g.setFont(brandLight);g.drawText("LATHAM",22,14,70,26,juce::Justification::centredLeft);g.setFont(brandBold);g.drawText("AUDIO",86,14,72,26,juce::Justification::centredLeft);drawVeloriaWordmark(g,{520,9,360,31});
    if(whatIfOpen){drawWhatIfOverlay(g);return;}
    drawPanel(g,{14,74,330,556},"STOCHASTIC FIELD / PERFORMANCE");drawPanel(g,{354,74,774,556},"LIVING PLANET / LIVE MATHEMATICAL STATE");drawPanel(g,{1138,74,248,268},"EVOLUTION / STRUCTURE");drawPanel(g,{1138,352,248,278},"VOICE / PROBABILITY STATE");drawPanel(g,{14,640,500,220},audioProcessor.isDrumMode()?"PERCUSSION ENVELOPE / INTERNAL":"AMPLITUDE ENVELOPE");drawPanel(g,{524,640,610,220},"PRESETS / FIELD MEMORY");drawPanel(g,{1144,640,242,220},"OUTPUT");drawStochasticGlobe(g,{370,92,742,520});drawEvolutionGraph(g,{1156,116,212,142});
    const auto bp=(int)parameterValue(audioProcessor.parameters,"breakpoints"),order=(int)parameterValue(audioProcessor.parameters,"walkOrder");const auto lock=parameterValue(audioProcessor.parameters,"pitchStability"),chaosValue=parameterValue(audioProcessor.parameters,"chaos");g.setColour(cyan.withAlpha(0.65f));g.setFont(9);g.drawText("BREAKPOINTS  "+juce::String(bp),1156,424,210,18,juce::Justification::centredLeft);g.drawText("WALK ORDER   "+juce::String(order),1156,450,210,18,juce::Justification::centredLeft);g.drawText("PITCH LOCK   "+juce::String(lock*100,0)+"%",1156,476,210,18,juce::Justification::centredLeft);g.setColour(gold.withAlpha(0.72f));g.drawText("CHAOS        "+juce::String(chaosValue*100,0)+"%",1156,502,210,18,juce::Justification::centredLeft);
}

void VeloriaAudioProcessorEditor::drawEvolutionGraph(juce::Graphics& g,juce::Rectangle<float> b)
{
    const auto active=juce::jlimit(4,12,(int)parameterValue(audioProcessor.parameters,"breakpoints")); juce::Path p;
    for(int i=0;i<active;++i){const auto x=b.getX()+b.getWidth()*(float)i/(float)juce::jmax(1,active-1);const auto y=b.getCentreY()-visualState.amplitudes[(std::size_t)i]*b.getHeight()*0.34f;if(i==0)p.startNewSubPath(x,y);else p.lineTo(x,y);}g.setColour(purple.withAlpha(0.25f));g.strokePath(p,juce::PathStrokeType(6));g.setColour(cyan.withAlpha(0.8f));g.strokePath(p,juce::PathStrokeType(1.4f));
}

void VeloriaAudioProcessorEditor::drawStochasticGlobe(juce::Graphics& g,juce::Rectangle<float> b)
{
    const auto aw=parameterValue(audioProcessor.parameters,"ampWalk"),tw=parameterValue(audioProcessor.parameters,"timeWalk");
    const auto ampBarrier=parameterValue(audioProcessor.parameters,"ampMirror"),timeBarrier=parameterValue(audioProcessor.parameters,"timeMirror");
    const auto chaosValue=parameterValue(audioProcessor.parameters,"chaos"),ampStepValue=parameterValue(audioProcessor.parameters,"ampStep"),timeStepValue=parameterValue(audioProcessor.parameters,"timeStep");
    const auto ampDistValue=parameterValue(audioProcessor.parameters,"ampDist"),timeDistValue=parameterValue(audioProcessor.parameters,"timeDist");
    const auto pitchLockValue=parameterValue(audioProcessor.parameters,"pitchStability"),curveValue=parameterValue(audioProcessor.parameters,"curve"),orderValue=parameterValue(audioProcessor.parameters,"walkOrder");
    const auto activePoints=juce::jlimit(4,12,(int)parameterValue(audioProcessor.parameters,"breakpoints"));
    const auto barrierSpan=juce::jlimit(0.0f,1.0f,(ampBarrier+timeBarrier)*0.5f);
    const auto stepEnergy=juce::jlimit(0.0f,1.0f,((ampStepValue+timeStepValue)*0.5f-0.10f)/1.90f);
    const auto distributionEnergy=juce::jlimit(0.0f,1.0f,(ampDistValue+timeDistValue)/10.0f);
    const auto energy=juce::jlimit(0.0f,1.0f,visualState.energy);
    const auto activity=juce::jlimit(0.0f,1.0f,0.25f+aw*0.12f+tw*0.11f+energy*0.14f+chaosValue*0.13f+stepEnergy*0.10f+barrierSpan*0.08f+distributionEnergy*0.05f);

    auto globe=b.withSizeKeepingCentre(510.0f,510.0f);const auto c=globe.getCentre();const auto r=globe.getWidth()*0.455f;
    juce::ColourGradient halo(purple.withAlpha(0.08f+energy*0.07f),c.x,c.y,juce::Colours::transparentBlack,c.x+r*1.55f,c.y,true);halo.addColour(0.32,magenta.withAlpha(0.06f));g.setGradientFill(halo);g.fillEllipse(globe.expanded(58));
    juce::ColourGradient body(juce::Colour::fromRGB(32,8,53).withAlpha(0.16f),c.x-r*.55f,c.y-r*.55f,juce::Colour::fromRGB(2,3,8).withAlpha(0.38f),c.x+r*.88f,c.y+r*.82f,true);body.addColour(.35,juce::Colour::fromRGB(48,12,82).withAlpha(.12f));g.setGradientFill(body);g.fillEllipse(globe);

    float total=0.0f;for(int i=0;i<activePoints;++i)total+=juce::jmax(.001f,visualState.durations[(std::size_t)i]);
    std::array<juce::Point<float>,VeloriaAudioProcessor::visualBreakpointCount> pts{};float cumulative=0.0f;
    for(std::size_t i=0;i<pts.size();++i)
    {
        if((int)i>=activePoints){pts[i]=c;continue;}
        const auto a=rotationPhase-juce::MathConstants<float>::halfPi+juce::MathConstants<float>::twoPi*(cumulative/juce::jmax(.001f,total));
        cumulative+=juce::jmax(.001f,visualState.durations[i]);
        const auto radial=r*((.38f+.20f*barrierSpan)+visualState.amplitudes[i]*(.14f+.13f*ampBarrier));
        const auto depth=.62f+.27f*timeBarrier+.10f*std::sin(a*(1.55f+.55f*orderValue)+rotationPhase*(.42f+.26f*(1.0f-pitchLockValue))+i*.31f);
        pts[i]={c.x+std::cos(a)*radial,c.y+std::sin(a)*radial*depth};
    }

    const std::array<juce::Point<float>,5> base{{{-0.58f,-0.37f},{0.02f,-0.63f},{0.61f,-0.19f},{0.42f,0.52f},{-0.52f,0.49f}}};
    std::array<juce::Point<float>,5> anchors{};std::array<float,5> aEnergy{};
    for(int a=0;a<5;++a)
    {
        const auto i0=(std::size_t)((a*2)%activePoints),i1=(std::size_t)((a*2+1)%activePoints),i2=(std::size_t)((a*2+3)%activePoints);
        const auto amp=(visualState.amplitudes[i0]+visualState.amplitudes[i1]+visualState.amplitudes[i2])/3.0f;const auto dur=(visualState.durations[i0]+visualState.durations[i1]+visualState.durations[i2])/3.0f;
        anchors[(std::size_t)a]={c.x+base[(std::size_t)a].x*r+amp*r*(.025f+aw*.035f)+std::sin(rotationPhase*(.22f+a*.03f)+a*1.31f)*r*(.006f+chaosValue*.018f),c.y+base[(std::size_t)a].y*r+(dur-1)*r*(.010f+.014f*timeBarrier)+std::cos(rotationPhase*(.20f+a*.04f)+a*.77f)*r*(.006f+tw*.016f)};
        aEnergy[(std::size_t)a]=juce::jlimit(0.0f,1.0f,.32f+std::abs(amp)*.40f+energy*.20f+stepEnergy*.08f);
    }

    std::array<juce::Point<float>,52> hubs{};
    for(int h=0;h<(int)hubs.size();++h){const auto direction=orderValue<1.5f?-1.0f:1.0f;const auto ang=(float)h*2.39996323f+rotationPhase*direction*(.05f+(h%9)*.006f);const auto shell=.18f+(.52f+.25f*barrierSpan)*std::sqrt(hash01(h*47+3));const auto z=std::sin((float)h*(.45f+.12f*distributionEnergy)+rotationPhase*.19f);auto p=juce::Point<float>(c.x+std::cos(ang)*r*shell,c.y+std::sin(ang)*r*shell*(.68f+.20f*timeBarrier+.12f*z));const auto bp=pts[(std::size_t)(h%activePoints)];const auto attract=.06f+.19f*hash01(h*29+4)*pitchLockValue;p=p*(1.0f-attract)+bp*attract;hubs[(std::size_t)h]=p;}

    {
        juce::Graphics::ScopedSaveState clipped(g);juce::Path clip;clip.addEllipse(globe);g.reduceClipRegion(clip);
        for(int i=0;i<5200;++i){const auto fi=(float)i;const auto z=std::sin(fi*.613f+rotationPhase*(.12f+tw*.21f));const auto near=(z+1)*.5f;const auto radial=std::pow(hash01(i*37+11),.30f+(1.0f-barrierSpan)*.30f);const auto direction=orderValue<1.5f?-1.0f:1.0f;const auto angle=fi*2.39996323f+rotationPhase*direction*(.055f+(i%31)*.0035f)+std::sin(fi*.031f)*(chaosValue*.28f+distributionEnergy*.12f);const auto rr=r*radial*(.94f+.06f*barrierSpan+.04f*std::sin(fi*.109f+rotationPhase*(.6f+activity)));const auto x=c.x+std::cos(angle)*rr;const auto y=c.y+std::sin(angle)*rr*(.66f+.17f*timeBarrier+.17f*z);const auto hotModulo=juce::jmax(71,181-(int)(stepEnergy*78.0f));const bool hot=i%hotModulo==0;const auto size=hot?2.4f+stepEnergy*.8f:.30f+near*.92f;auto col=i%47==0?gold:(i%29==0?magenta:(i%19==0?cyan:purple));g.setColour(col.withAlpha(hot?.58f:.025f+near*.06f+energy*.028f));g.fillEllipse(x-size*.5f,y-size*.5f,size,size);}
        for(int strand=0;strand<430;++strand){const auto h0=(strand*7+3)%(int)hubs.size(),h1=(strand*19+17)%(int)hubs.size();auto p0=hubs[(std::size_t)h0],p3=hubs[(std::size_t)h1];if(strand%11==0)p0=anchors[(std::size_t)(strand%5)];if(strand%17==0)p3=pts[(std::size_t)(strand%activePoints)];const auto mid=(p0+p3)*.5f;auto tangent=p3-p0;juce::Point<float> normal(-tangent.y,tangent.x);normal/=juce::jmax(1.0f,normal.getDistanceFromOrigin());auto radial=mid-c;radial/=juce::jmax(1.0f,radial.getDistanceFromOrigin());const auto bend=r*(.018f+(.08f+.10f*stepEnergy)*hash01(strand*19+5))*(.68f+chaosValue*.55f+distributionEnergy*.22f)*(1.10f-curveValue*.30f);const auto phase=rotationPhase*((orderValue<1.5f?-.09f:.09f)+(strand%23)*.003f)+strand*.31f;const auto cp1=p0*.64f+mid*.36f+normal*bend+radial*std::sin(phase)*r*(.020f+.020f*(1.0f-pitchLockValue));const auto cp2=p3*.64f+mid*.36f-normal*bend+radial*std::cos(phase*.87f)*r*(.020f+.020f*(1.0f-pitchLockValue));juce::Path path;path.startNewSubPath(p0);path.cubicTo(cp1,cp2,p3);const auto col=strand%23==0?gold:(strand%17==0?cyan:(strand%9==0?magenta:purple));const bool major=strand%31==0;g.setColour(col.withAlpha((major?.11f:.018f)+activity*(major?.045f:.018f)));g.strokePath(path,juce::PathStrokeType(major?.88f:.34f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));}
        for(int sweep=0;sweep<180;++sweep){const auto rx=r*(.30f+(.45f+.20f*barrierSpan)*hash01(sweep*11+1)),ry=r*(.10f+(.48f+.25f*timeBarrier)*hash01(sweep*13+4));const auto direction=orderValue<1.5f?-1.0f:1.0f;const auto rot=rotationPhase*direction*(.045f+(sweep%13)*.004f)+(float)sweep*(.13f+.05f*distributionEnergy);const auto ox=c.x+(hash01(sweep*17+2)-.5f)*r*.34f,oy=c.y+(hash01(sweep*23+8)-.5f)*r*.32f;juce::Path orbit;orbit.addCentredArc(ox,oy,rx,ry,rot,.04f,juce::MathConstants<float>::twoPi-.04f,true);const auto col=sweep%23==0?gold:(sweep%17==0?magenta:(sweep%13==0?cyan:purple));g.setColour(col.withAlpha(.009f+activity*.015f));g.strokePath(orbit,juce::PathStrokeType(.30f+(sweep%41==0?.38f:0)));}
        for(int h=0;h<(int)hubs.size();++h){const auto p=hubs[(std::size_t)h];const auto col=h%11==0?gold:(h%7==0?cyan:(h%5==0?magenta:purple));const auto sz=1.1f+1.7f*hash01(h*19+7)+energy*.7f+stepEnergy*.3f;juce::ColourGradient glow(col.withAlpha(.14f),p.x,p.y,juce::Colours::transparentBlack,p.x+sz*4.8f,p.y,true);g.setGradientFill(glow);g.fillEllipse(p.x-sz*4.8f,p.y-sz*4.8f,sz*9.6f,sz*9.6f);g.setColour(col.withAlpha(.84f));g.fillEllipse(p.x-sz*.5f,p.y-sz*.5f,sz,sz);}
        for(int i=0;i<activePoints;++i){const auto amp=std::abs(visualState.amplitudes[(std::size_t)i]);const auto nr=1.5f+amp*1.8f+energy*.3f;const auto col=i%4==0?gold:(i%3==0?cyan:magenta);g.setColour(col.withAlpha(.12f+amp*.06f));g.fillEllipse(pts[(std::size_t)i].x-nr*3,pts[(std::size_t)i].y-nr*3,nr*6,nr*6);g.setColour(col.withAlpha(.94f));g.fillEllipse(pts[(std::size_t)i].x-nr,pts[(std::size_t)i].y-nr,nr*2,nr*2);}
        for(int a=0;a<5;++a){const auto anchor=anchors[(std::size_t)a]; const auto e=aEnergy[(std::size_t)a];const auto col=a==0||a==3?gold:(a==2?cyan:magenta);const auto core=r*(.009f+.008f*e);for(int p=0;p<150;++p){const auto angle=(float)p*2.39996323f+rotationPhase*(.20f+a*.04f);const auto rr=r*(.008f+.12f*std::sqrt((float)(p+1)/150.0f))*(.55f+e*.72f);const auto q=anchor+juce::Point<float>(std::cos(angle)*rr,std::sin(angle)*rr*.72f);const auto size=p%23==0?1.7f:.35f+hash01(p*13+a)*.48f;g.setColour(col.withAlpha(.032f+e*.06f));g.fillEllipse(q.x-size*.5f,q.y-size*.5f,size,size);}juce::ColourGradient glow(col.withAlpha(.22f+e*.15f),anchor.x,anchor.y,juce::Colours::transparentBlack,anchor.x+core*6.2f,anchor.y,true);g.setGradientFill(glow);g.fillEllipse(anchor.x-core*6.2f,anchor.y-core*6.2f,core*12.4f,core*12.4f);g.setColour(col.withAlpha(.97f));g.fillEllipse(anchor.x-core,anchor.y-core,core*2,core*2);g.setColour(juce::Colours::white.withAlpha(.95f));g.fillEllipse(anchor.x-1.1f,anchor.y-1.1f,2.2f,2.2f);}
        juce::ColourGradient limb(juce::Colours::transparentBlack,c.x-r*.5f,c.y-r*.35f,juce::Colours::black.withAlpha(.045f),c.x+r*.98f,c.y+r*.90f,false);g.setGradientFill(limb);g.fillEllipse(globe);
    }

    for(int ring=0;ring<70;++ring){const auto rr=r*(.84f+ring*.0058f*(.78f+.22f*barrierSpan));const auto flatten=.42f+.006f*(ring%25)+timeBarrier*.10f;const auto direction=orderValue<1.5f?-1.0f:1.0f;const auto tilt=rotationPhase*direction*(ring%2?-.16f:.14f)+ring*.111f;juce::Path orbit;orbit.addCentredArc(c.x,c.y,rr,rr*flatten,tilt,.03f,juce::MathConstants<float>::twoPi-.03f,true);const auto col=ring%17==0?gold:(ring%13==0?cyan:purple);g.setColour(col.withAlpha(.012f+activity*.016f));g.strokePath(orbit,juce::PathStrokeType(.3f+(ring%29==0?.32f:0)));}
    for(int i=0;i<900;++i){const auto fi=(float)i;const auto direction=orderValue<1.5f?-1.0f:1.0f;const auto angle=fi*2.39996323f+rotationPhase*direction*(.11f+(i%23)*.005f);const auto rr=r*(.74f+(.30f+.15f*barrierSpan)*hash01(i*23+5));const auto z=std::sin(fi*.071f+rotationPhase*.4f);const auto x=c.x+std::cos(angle)*rr,y=c.y+std::sin(angle)*rr*(.62f+.13f*timeBarrier+.20f*z);const bool hot=i%79==0;const auto size=hot?2.4f+stepEnergy*.5f:.45f+hash01(i*11+2)*.78f;const auto col=i%37==0?gold:(i%23==0?cyan:(i%17==0?magenta:purple));g.setColour(col.withAlpha(hot?.66f:.040f+energy*.04f));g.fillEllipse(x-size*.5f,y-size*.5f,size,size);}
    g.setColour(cyan.withAlpha(.045f+energy*.025f));g.drawEllipse(globe,.50f);g.setColour(purple.withAlpha(.055f));g.drawEllipse(globe.reduced(2.5f),.42f);g.setColour(gold.withAlpha(.020f+chaosValue*.02f));g.drawEllipse(globe.reduced(6),.36f);
}

void VeloriaAudioProcessorEditor::resized()
{
    title.setBounds(515,8,370,34);subtitle.setBounds(535,40,330,11);whatIfButton.setBounds(842,15,80,30);presetBox.setBounds(930,15,190,30);discoverButton.setBounds(1128,15,78,30);newFieldButton.setBounds(1213,15,84,30);monoButton.setBounds(1305,15,68,30);
    ampWalk.setBounds(28,108,64,242);timeWalk.setBounds(104,108,64,242);ampMirror.setBounds(180,108,64,242);timeMirror.setBounds(256,108,64,242);
    ampDist.setBounds(24,374,70,104);timeDist.setBounds(100,374,70,104);ampStep.setBounds(176,374,70,104);timeStep.setBounds(252,374,70,104);chaos.setBounds(24,490,70,104);breakpoints.setBounds(100,490,70,104);pitchStability.setBounds(176,490,70,104);curve.setBounds(252,490,70,104);boundary.setBounds(24,594,70,86);rate.setBounds(100,594,70,86);jump.setBounds(176,594,70,86);correlation.setBounds(252,594,70,86);orderButton.setBounds(25,686,68,20);
    attack.setBounds(30,724,104,112);decay.setBounds(150,724,104,112);sustain.setBounds(270,724,104,112);release.setBounds(390,724,104,112);
    seed.setBounds(548,700,86,92);fieldStatus.setBounds(646,716,148,35);presetNameEditor.setBounds(808,674,300,29);savePresetButton.setBounds(808,712,78,29);renamePresetButton.setBounds(894,712,82,29);deletePresetButton.setBounds(984,712,82,29);presetStatus.setBounds(808,754,300,47);voiceStatus.setBounds(1156,366,205,22);level.setBounds(1188,674,154,150);footerStatus.setBounds(415,872,570,14);
}
