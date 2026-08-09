#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class VeloriaAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit VeloriaAudioProcessorEditor(VeloriaAudioProcessor&);
    ~VeloriaAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    class AuroraLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                              float sliderPosProportional, float rotaryStartAngle,
                              float rotaryEndAngle, juce::Slider&) override;
        void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
                              float sliderPos, float minSliderPos, float maxSliderPos,
                              juce::Slider::SliderStyle, juce::Slider&) override;
    };

    class MidiLearnSlider final : public juce::Slider
    {
    public:
        std::function<void()> learnCallback;
        std::function<void()> clearCallback;
        std::function<int()> currentCCCallback;
        void mouseDown(const juce::MouseEvent&) override;
    };

    class EngineModeButton final : public juce::TextButton,
                                   private juce::Timer
    {
    public:
        EngineModeButton(VeloriaAudioProcessorEditor& editorToUse,
                         VeloriaAudioProcessor& processorToUse)
            : juce::TextButton("ENGINE: VELORIA"),
              editor(editorToUse), processor(processorToUse)
        {
            setTooltip("Switch between Veloria's modern DSS model and the Andrew R. Brown / Greg Jenkins IDSS-inspired operating model.");
            onClick = [this]
            {
                processor.setBrownIdssMode(! processor.isBrownIdssMode());
                editor.presetStatus.setText(processor.isBrownIdssMode()
                    ? "BROWN IDSS: INTERACTIVE DSS OPERATING MODEL"
                    : "VELORIA: MODERN DSS OPERATING MODEL",
                    juce::dontSendNotification);
                updateAppearance();
            };
            startTimerHz(12);
        }

    private:
        void timerCallback() override
        {
            if (getParentComponent() == nullptr)
                editor.addAndMakeVisible(*this);

            setBounds(103, 600, 215, 20);
            setVisible(! editor.whatIfOpen);
            updateAppearance();
        }

        void updateAppearance()
        {
            const auto brown = processor.isBrownIdssMode();
            setButtonText(brown ? "ENGINE: BROWN IDSS" : "ENGINE: VELORIA");
            const auto active = brown ? juce::Colour::fromRGB(255, 184, 86)
                                      : juce::Colour::fromRGB(176, 77, 255);
            setColour(juce::TextButton::buttonColourId, active.withAlpha(brown ? 0.24f : 0.16f));
            setColour(juce::TextButton::textColourOffId,
                      (brown ? juce::Colour::fromRGB(255, 184, 86) : juce::Colours::white).withAlpha(0.92f));
        }

        VeloriaAudioProcessorEditor& editor;
        VeloriaAudioProcessor& processor;
    };

    class RevisedWhatIfOverlay final : public juce::Component,
                                       private juce::Timer
    {
    public:
        explicit RevisedWhatIfOverlay(VeloriaAudioProcessorEditor& editorToUse)
            : editor(editorToUse)
        {
            setInterceptsMouseClicks(false, false);
            startTimerHz(12);
        }

        void paint(juce::Graphics& g) override
        {
            const auto bg = juce::Colour::fromRGB(5, 5, 10);
            const auto panel = juce::Colour::fromRGB(13, 13, 21);
            const auto gold = juce::Colour::fromRGB(255, 184, 86);
            const auto purple = juce::Colour::fromRGB(176, 77, 255);
            const auto magenta = juce::Colour::fromRGB(255, 72, 190);
            const auto cyan = juce::Colour::fromRGB(111, 226, 255);

            auto bounds = getLocalBounds().toFloat();
            g.setColour(bg.withAlpha(0.995f));
            g.fillRoundedRectangle(bounds, 12.0f);
            g.setColour(gold.withAlpha(0.22f));
            g.drawRoundedRectangle(bounds.reduced(0.5f), 12.0f, 1.0f);

            auto inner = bounds.reduced(24.0f);
            auto header = inner.removeFromTop(78.0f);
            g.setColour(gold.withAlpha(0.96f));
            g.setFont(juce::FontOptions(26.0f, juce::Font::bold));
            g.drawText("WHAT IF?  //  THE SYNTHESIS INDUSTRY THAT NEVER HAPPENED",
                       header.removeFromTop(36.0f).toNearestInt(), juce::Justification::centredLeft);
            g.setColour(juce::Colours::white.withAlpha(0.62f));
            g.setFont(juce::FontOptions(11.0f));
            g.drawFittedText("First, the history that DID happen. Then the question Veloria asks: what if stochastic synthesis had received the same decades of instrument design as analogue synthesis?",
                             header.toNearestInt(), juce::Justification::centredLeft, 2);

            auto sectionLabel = [&](const juce::String& text, juce::Colour colour)
            {
                auto r = inner.removeFromTop(24.0f);
                g.setColour(colour.withAlpha(0.90f));
                g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
                g.drawText(text, r.toNearestInt(), juce::Justification::centredLeft);
                inner.removeFromTop(7.0f);
            };

            auto historyCard = [&](juce::Rectangle<float> r, const juce::String& years,
                                   const juce::String& name, const juce::String& role,
                                   const juce::String& story, juce::Colour colour)
            {
                g.setColour(panel.withAlpha(0.96f));
                g.fillRoundedRectangle(r, 9.0f);
                g.setColour(colour.withAlpha(0.28f));
                g.drawRoundedRectangle(r, 9.0f, 1.0f);
                auto c = r.reduced(15.0f);
                g.setColour(colour.withAlpha(0.92f));
                g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
                g.drawText(years, c.removeFromTop(20.0f).toNearestInt(), juce::Justification::centredLeft);
                g.setColour(juce::Colours::white.withAlpha(0.95f));
                g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
                g.drawText(name, c.removeFromTop(28.0f).toNearestInt(), juce::Justification::centredLeft);
                g.setColour(colour.withAlpha(0.82f));
                g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
                g.drawText(role, c.removeFromTop(20.0f).toNearestInt(), juce::Justification::centredLeft);
                c.removeFromTop(6.0f);
                g.setColour(juce::Colours::white.withAlpha(0.64f));
                g.setFont(juce::FontOptions(11.0f));
                g.drawFittedText(story, c.toNearestInt(), juce::Justification::topLeft, 6);
            };

            sectionLabel("REAL HISTORY  //  THE TWO FOUNDATIONAL MOMENTS", cyan);
            auto realRow = inner.removeFromTop(214.0f);
            auto xenakis = realRow.removeFromLeft((realRow.getWidth() - 14.0f) * 0.5f);
            realRow.removeFromLeft(14.0f);
            auto brown = realRow;
            historyCard(xenakis, "1950s-1970s", "IANNIS XENAKIS", "THE FOUNDATIONAL MOMENT  //  INVENT THE LANGUAGE",
                        "Dynamic Stochastic Synthesis makes the waveform itself a probability system. Breakpoints, random walks, reflecting barriers and statistical distributions become a new grammar for generating sound.", gold);
            historyCard(brown, "2004-2005", "ANDREW R. BROWN + GREG JENKINS", "THE INSTRUMENT MOMENT  //  MAKE IT PLAYABLE",
                        "IDSS turns DSS toward real-time musical interaction: finer step control, pitch stabilisation, interpolation choices and stochastic percussion gestures. The research language becomes something a performer can deliberately play.", magenta);

            inner.removeFromTop(14.0f);
            auto hinge = inner.removeFromTop(50.0f);
            g.setColour(gold.withAlpha(0.13f));
            g.fillRoundedRectangle(hinge, 7.0f);
            g.setColour(gold.withAlpha(0.92f));
            g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
            g.drawText("WHAT IF THE SYNTH INDUSTRY HAD TAKEN THAT BALL AND RUN WITH IT?",
                       hinge.removeFromTop(25.0f).toNearestInt(), juce::Justification::centred);
            g.setColour(juce::Colours::white.withAlpha(0.48f));
            g.setFont(juce::FontOptions(9.5f));
            g.drawText("THIS IS WHERE THE HISTORY THAT NEVER HAPPENED BEGINS.", hinge.toNearestInt(), juce::Justification::centred);

            inner.removeFromTop(14.0f);
            sectionLabel("THE MISSING INDUSTRY  //  AN IMAGINED COMMERCIAL EVOLUTION", purple);

            struct FutureCard { const char* name; const char* tag; const char* story; juce::Colour colour; };
            const std::array<FutureCard, 4> futures {{
                { "THE STOCHASTIC MONOSYNTH", "THE PERFORMANCE SYNTH",
                  "Barrier Width becomes the big expressive control. Cauchy Step becomes the bite. Walk Order, distributions and pitch stabilisation are designed for hands-on playing rather than laboratory parameter entry.", magenta },
                { "THE STOCHASTIC ACID BOX", "THE BASS MACHINE",
                  "A compact sequenced instrument discovers its own club language: mutation replaces filter sweep, probability shapes slide, and eruptive heavy-tailed jumps create a new kind of squelch without a diode ladder.", purple },
                { "THE STOCHASTIC DRUM MACHINE", "THE CLUB MACHINE",
                  "Kick contraction, stochastic transients and dual-engine snares turn Brown's percussion insight into a complete rhythm instrument. The chaos is not added noise; it is the drum's waveform evolving in time.", cyan },
                { "VELORIA 2026", "THE MISSING FIFTY YEARS",
                  "Polyphony, family-aware presets, expressive pressure, recallable stochastic fields and a living mathematical interface. Not a recreation of one historical machine, but the instrument this synthesis lineage might have grown into.", gold }
            }};

            const float gap = 12.0f;
            const float cardW = (inner.getWidth() - gap) * 0.5f;
            const float cardH = (inner.getHeight() - gap) * 0.5f;
            for (int i = 0; i < 4; ++i)
            {
                const int row = i / 2, col = i % 2;
                auto r = juce::Rectangle<float>(inner.getX() + col * (cardW + gap),
                                                inner.getY() + row * (cardH + gap),
                                                cardW, cardH);
                const auto& f = futures[(std::size_t)i];
                g.setColour(panel.withAlpha(0.94f));
                g.fillRoundedRectangle(r, 8.0f);
                g.setColour(f.colour.withAlpha(0.25f));
                g.drawRoundedRectangle(r, 8.0f, 1.0f);
                auto c = r.reduced(14.0f);
                g.setColour(juce::Colours::white.withAlpha(0.94f));
                g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
                g.drawText(f.name, c.removeFromTop(24.0f).toNearestInt(), juce::Justification::centredLeft);
                g.setColour(f.colour.withAlpha(0.80f));
                g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
                g.drawText(f.tag, c.removeFromTop(18.0f).toNearestInt(), juce::Justification::centredLeft);
                c.removeFromTop(5.0f);
                g.setColour(juce::Colours::white.withAlpha(0.60f));
                g.setFont(juce::FontOptions(10.0f));
                g.drawFittedText(f.story, c.toNearestInt(), juce::Justification::topLeft, 5);
            }
        }

    private:
        void timerCallback() override
        {
            if (getParentComponent() == nullptr)
                editor.addAndMakeVisible(*this);
            setBounds(16, 74, editor.getWidth() - 32, editor.getHeight() - 90);
            setVisible(editor.whatIfOpen);
            if (editor.whatIfOpen)
                toFront(false);
        }

        VeloriaAudioProcessorEditor& editor;
    };

    void timerCallback() override;
    void drawStochasticGlobe(juce::Graphics&, juce::Rectangle<float> bounds);
    void drawEvolutionGraph(juce::Graphics&, juce::Rectangle<float> bounds);
    void drawPanel(juce::Graphics&, juce::Rectangle<float> bounds, const juce::String& title);
    void drawWhatIfOverlay(juce::Graphics&);
    void setWhatIfMode(bool shouldOpen);
    void configureKnob(MidiLearnSlider&, const juce::String& parameterId, const juce::String& displayName);
    void configureFieldSlider(MidiLearnSlider&, const juce::String& parameterId, const juce::String& displayName);
    void refreshPresetBox(const juce::String& selectUserPreset = {});
    bool selectedPresetIsUser() const noexcept;
    void refreshOrderButton();

    VeloriaAudioProcessor& audioProcessor;
    VeloriaAudioProcessor::VisualState visualState;
    AuroraLookAndFeel auroraLookAndFeel;

    MidiLearnSlider ampWalk, timeWalk, ampMirror, timeMirror;
    MidiLearnSlider ampDist, timeDist, ampStep, timeStep;
    MidiLearnSlider chaos, breakpoints, pitchStability, curve;
    MidiLearnSlider attack, decay, sustain, release;
    MidiLearnSlider seed, level;

    juce::ComboBox presetBox;
    juce::ToggleButton monoButton { "MONO" };
    juce::TextButton orderButton { "ORDER 2" };
    juce::TextButton discoverButton { "DISCOVER" };
    juce::TextButton newFieldButton { "NEW FIELD" };
    juce::TextButton whatIfButton { "WHAT IF?" };
    juce::TextButton savePresetButton { "SAVE" };
    juce::TextButton renamePresetButton { "RENAME" };
    juce::TextButton deletePresetButton { "DELETE" };
    juce::TextEditor presetNameEditor;
    juce::Label title, brand, subtitle, fieldStatus, voiceStatus, footerStatus, presetStatus;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> ampWalkAttachment, timeWalkAttachment,
        ampMirrorAttachment, timeMirrorAttachment;
    std::unique_ptr<SliderAttachment> ampDistAttachment, timeDistAttachment,
        ampStepAttachment, timeStepAttachment, chaosAttachment,
        breakpointsAttachment, pitchStabilityAttachment, curveAttachment;
    std::unique_ptr<SliderAttachment> attackAttachment, decayAttachment,
        sustainAttachment, releaseAttachment, seedAttachment, levelAttachment;
    std::unique_ptr<ButtonAttachment> monoAttachment;

    float rotationPhase { 0.0f };
    bool whatIfOpen { false };
    EngineModeButton engineButton { *this, audioProcessor };
    RevisedWhatIfOverlay revisedWhatIfOverlay { *this };
    static constexpr int firstUserPresetId = 1001;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VeloriaAudioProcessorEditor)
};
