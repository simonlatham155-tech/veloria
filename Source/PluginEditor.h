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
            setInterceptsMouseClicks(true, false);
            startTimerHz(12);
        }

        void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
        {
            scrollBy(-wheel.deltaY * 260.0f);
        }

        void mouseDown(const juce::MouseEvent& e) override
        {
            lastDragY = e.position.y;
        }

        void mouseDrag(const juce::MouseEvent& e) override
        {
            const auto delta = lastDragY - e.position.y;
            lastDragY = e.position.y;
            scrollBy(delta);
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

            {
                juce::Graphics::ScopedSaveState scrollState(g);
                g.reduceClipRegion(getLocalBounds().reduced(2));
                g.addTransform(juce::AffineTransform::translation(0.0f, -scrollOffset));

                auto inner = juce::Rectangle<float>(24.0f, 24.0f, bounds.getWidth() - 62.0f, contentHeight - 48.0f);
                auto header = inner.removeFromTop(112.0f);
                g.setColour(gold.withAlpha(0.96f));
                g.setFont(juce::FontOptions(31.0f, juce::Font::bold));
                g.drawText("WHAT IF?  //  THE SYNTHESIS INDUSTRY THAT NEVER HAPPENED",
                           header.removeFromTop(44.0f).toNearestInt(), juce::Justification::centredLeft);
                g.setColour(juce::Colours::white.withAlpha(0.72f));
                g.setFont(juce::FontOptions(15.0f));
                g.drawFittedText("First, the history that DID happen. Then the question Veloria asks: what if stochastic synthesis had received decades of dedicated instrument design and musical refinement?",
                                 header.toNearestInt(), juce::Justification::centredLeft, 3);

                inner.removeFromTop(14.0f);
                drawHeroIllustration(g, inner.removeFromTop(290.0f), panel, gold, purple, magenta, cyan);
                inner.removeFromTop(24.0f);

                auto sectionLabel = [&](const juce::String& text, juce::Colour colour)
                {
                    auto r = inner.removeFromTop(34.0f);
                    g.setColour(colour.withAlpha(0.94f));
                    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
                    g.drawText(text, r.toNearestInt(), juce::Justification::centredLeft);
                    inner.removeFromTop(10.0f);
                };

                auto historyCard = [&](juce::Rectangle<float> r, const juce::String& years,
                                       const juce::String& name, const juce::String& role,
                                       const juce::String& story, juce::Colour colour)
                {
                    g.setColour(panel.withAlpha(0.97f));
                    g.fillRoundedRectangle(r, 10.0f);
                    g.setColour(colour.withAlpha(0.34f));
                    g.drawRoundedRectangle(r, 10.0f, 1.2f);
                    auto c = r.reduced(20.0f);
                    g.setColour(colour.withAlpha(0.94f));
                    g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
                    g.drawText(years, c.removeFromTop(24.0f).toNearestInt(), juce::Justification::centredLeft);
                    g.setColour(juce::Colours::white.withAlpha(0.97f));
                    g.setFont(juce::FontOptions(23.0f, juce::Font::bold));
                    g.drawText(name, c.removeFromTop(34.0f).toNearestInt(), juce::Justification::centredLeft);
                    g.setColour(colour.withAlpha(0.86f));
                    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
                    g.drawText(role, c.removeFromTop(24.0f).toNearestInt(), juce::Justification::centredLeft);
                    c.removeFromTop(8.0f);
                    g.setColour(juce::Colours::white.withAlpha(0.72f));
                    g.setFont(juce::FontOptions(14.0f));
                    g.drawFittedText(story, c.toNearestInt(), juce::Justification::topLeft, 7);
                };

                sectionLabel("REAL HISTORY  //  THE TWO FOUNDATIONAL MOMENTS", cyan);
                auto realRow = inner.removeFromTop(270.0f);
                auto xenakis = realRow.removeFromLeft((realRow.getWidth() - 18.0f) * 0.5f);
                realRow.removeFromLeft(18.0f);
                historyCard(xenakis, "1950s-1970s", "IANNIS XENAKIS", "THE FOUNDATIONAL MOMENT  //  INVENT THE LANGUAGE",
                            "Dynamic Stochastic Synthesis makes the waveform itself a probability system. Breakpoints, random walks, reflecting barriers and statistical distributions become a new grammar for generating sound.", gold);
                historyCard(realRow, "2004-2005", "ANDREW R. BROWN + GREG JENKINS", "THE INSTRUMENT MOMENT  //  MAKE IT PLAYABLE",
                            "IDSS turns DSS toward real-time musical interaction: finer step control, pitch stabilisation, interpolation choices and stochastic percussion gestures. The research language becomes something a performer can deliberately play.", magenta);

                inner.removeFromTop(24.0f);
                auto hinge = inner.removeFromTop(82.0f);
                g.setColour(gold.withAlpha(0.14f));
                g.fillRoundedRectangle(hinge, 8.0f);
                g.setColour(gold.withAlpha(0.96f));
                g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
                g.drawText("WHAT IF THE SYNTH INDUSTRY HAD TAKEN THAT BALL AND RUN WITH IT?",
                           hinge.removeFromTop(42.0f).toNearestInt(), juce::Justification::centred);
                g.setColour(juce::Colours::white.withAlpha(0.58f));
                g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
                g.drawText("THIS IS WHERE THE HISTORY THAT NEVER HAPPENED BEGINS.", hinge.toNearestInt(), juce::Justification::centred);

                inner.removeFromTop(24.0f);
                sectionLabel("THE MISSING INDUSTRY  //  AN IMAGINED COMMERCIAL EVOLUTION", purple);

                struct FutureCard { const char* name; const char* tag; const char* story; juce::Colour colour; int visual; };
                const std::array<FutureCard, 4> futures {{
                    { "THE STOCHASTIC MONOSYNTH", "THE PERFORMANCE SYNTH",
                      "Barrier Width becomes the big expressive control. Cauchy Step becomes the bite. Walk Order, distributions and pitch stabilisation are designed for hands-on playing rather than laboratory parameter entry.", magenta, 0 },
                    { "THE STOCHASTIC ACID BOX", "THE BASS MACHINE",
                      "A compact sequenced instrument discovers its own club language: mutation replaces filter sweep, probability shapes slide, and eruptive heavy-tailed jumps create a new kind of squelch from waveform evolution itself.", purple, 1 },
                    { "THE STOCHASTIC DRUM MACHINE", "THE CLUB MACHINE",
                      "Kick contraction, stochastic transients and dual-engine snares turn Brown's percussion insight into a complete rhythm instrument. The chaos is not added noise; it is the drum's waveform evolving in time.", cyan, 2 },
                    { "VELORIA 2026", "THE MISSING FIFTY YEARS",
                      "Polyphony, family-aware presets, expressive pressure, recallable stochastic fields and a living mathematical interface. Not a recreation of one historical machine, but the instrument this synthesis lineage might have grown into.", gold, 3 }
                }};

                for (const auto& f : futures)
                {
                    auto r = inner.removeFromTop(210.0f);
                    g.setColour(panel.withAlpha(0.96f));
                    g.fillRoundedRectangle(r, 9.0f);
                    g.setColour(f.colour.withAlpha(0.28f));
                    g.drawRoundedRectangle(r, 9.0f, 1.1f);

                    auto c = r.reduced(18.0f);
                    auto visual = c.removeFromRight(360.0f);
                    c.removeFromRight(20.0f);
                    g.setColour(juce::Colours::white.withAlpha(0.96f));
                    g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
                    g.drawText(f.name, c.removeFromTop(32.0f).toNearestInt(), juce::Justification::centredLeft);
                    g.setColour(f.colour.withAlpha(0.86f));
                    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
                    g.drawText(f.tag, c.removeFromTop(24.0f).toNearestInt(), juce::Justification::centredLeft);
                    c.removeFromTop(7.0f);
                    g.setColour(juce::Colours::white.withAlpha(0.70f));
                    g.setFont(juce::FontOptions(14.0f));
                    g.drawFittedText(f.story, c.toNearestInt(), juce::Justification::topLeft, 7);
                    drawFutureVisual(g, visual, f.colour, f.visual);
                    inner.removeFromTop(14.0f);
                }

                auto footer = inner.removeFromTop(54.0f);
                g.setColour(gold.withAlpha(0.82f));
                g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
                g.drawText("VELORIA  //  THE INSTRUMENT FROM THE SYNTHESIS HISTORY THAT NEVER HAPPENED",
                           footer.toNearestInt(), juce::Justification::centred);
            }

            drawScrollbar(g, bounds);
        }

    private:
        void scrollBy(float amount)
        {
            const auto maxScroll = juce::jmax(0.0f, contentHeight - (float)getHeight());
            scrollOffset = juce::jlimit(0.0f, maxScroll, scrollOffset + amount);
            repaint();
        }

        void drawScrollbar(juce::Graphics& g, juce::Rectangle<float> bounds)
        {
            const auto maxScroll = juce::jmax(0.0f, contentHeight - bounds.getHeight());
            if (maxScroll <= 0.0f)
                return;

            auto track = juce::Rectangle<float>(bounds.getRight() - 11.0f, bounds.getY() + 14.0f, 4.0f, bounds.getHeight() - 28.0f);
            g.setColour(juce::Colours::white.withAlpha(0.08f));
            g.fillRoundedRectangle(track, 2.0f);
            const auto thumbH = juce::jmax(44.0f, track.getHeight() * bounds.getHeight() / contentHeight);
            const auto travel = track.getHeight() - thumbH;
            const auto thumbY = track.getY() + travel * (scrollOffset / maxScroll);
            g.setColour(juce::Colour::fromRGB(255, 184, 86).withAlpha(0.60f));
            g.fillRoundedRectangle(track.getX() - 1.0f, thumbY, 6.0f, thumbH, 3.0f);
        }

        static void drawRandomWalk(juce::Graphics& g, juce::Rectangle<float> area, juce::Colour colour, int seed)
        {
            juce::Random random(seed);
            juce::Path path;
            float y = area.getCentreY();
            path.startNewSubPath(area.getX(), y);
            for (int i = 1; i <= 34; ++i)
            {
                const auto x = area.getX() + area.getWidth() * (float)i / 34.0f;
                y += (random.nextFloat() - 0.5f) * area.getHeight() * 0.24f;
                y = juce::jlimit(area.getY() + 5.0f, area.getBottom() - 5.0f, y);
                path.lineTo(x, y);
            }
            g.setColour(colour.withAlpha(0.92f));
            g.strokePath(path, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        static void drawHeroIllustration(juce::Graphics& g, juce::Rectangle<float> r,
                                         juce::Colour panel, juce::Colour gold,
                                         juce::Colour purple, juce::Colour magenta,
                                         juce::Colour cyan)
        {
            g.setColour(panel.withAlpha(0.97f));
            g.fillRoundedRectangle(r, 11.0f);
            g.setColour(gold.withAlpha(0.30f));
            g.drawRoundedRectangle(r, 11.0f, 1.2f);
            auto c = r.reduced(20.0f);
            g.setColour(juce::Colours::white.withAlpha(0.92f));
            g.setFont(juce::FontOptions(17.0f, juce::Font::bold));
            g.drawText("FROM THEORY TO INSTRUMENT  //  THE VISUAL STORY", c.removeFromTop(28.0f).toNearestInt(), juce::Justification::centredLeft);
            c.removeFromTop(8.0f);

            const auto gap = 16.0f;
            const auto w = (c.getWidth() - gap * 2.0f) / 3.0f;
            auto theory = c.removeFromLeft(w); c.removeFromLeft(gap);
            auto instrument = c.removeFromLeft(w); c.removeFromLeft(gap);
            auto industry = c;

            for (auto box : { theory, instrument, industry })
            {
                g.setColour(juce::Colour::fromRGB(3, 4, 8).withAlpha(0.96f));
                g.fillRoundedRectangle(box, 8.0f);
                g.setColour(juce::Colours::white.withAlpha(0.08f));
                g.drawRoundedRectangle(box, 8.0f, 1.0f);
            }

            auto t = theory.reduced(15.0f);
            g.setColour(gold.withAlpha(0.88f)); g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
            g.drawText("STOCHASTIC WAVEFORM", t.removeFromTop(22.0f).toNearestInt(), juce::Justification::centredLeft);
            auto graph = t.reduced(0.0f, 8.0f);
            g.setColour(gold.withAlpha(0.22f));
            for (int i = 1; i < 4; ++i)
            {
                const auto y = graph.getY() + graph.getHeight() * (float)i / 4.0f;
                g.drawHorizontalLine((int)y, graph.getX(), graph.getRight());
            }
            drawRandomWalk(g, graph, gold, 77);

            auto s = instrument.reduced(15.0f);
            g.setColour(magenta.withAlpha(0.88f)); g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
            g.drawText("PLAYABLE DSS", s.removeFromTop(22.0f).toNearestInt(), juce::Justification::centredLeft);
            auto panelArea = s.removeFromTop(90.0f).reduced(5.0f);
            g.setColour(juce::Colour::fromRGB(57, 45, 50)); g.fillRoundedRectangle(panelArea, 5.0f);
            for (int k = 0; k < 5; ++k)
            {
                const auto x = panelArea.getX() + 24.0f + k * 50.0f;
                g.setColour((k % 2 == 0 ? magenta : purple).withAlpha(0.84f));
                g.fillEllipse(x, panelArea.getY() + 20.0f, 18.0f, 18.0f);
            }
            auto keys = s.removeFromBottom(60.0f);
            for (int k = 0; k < 16; ++k)
            {
                auto key = juce::Rectangle<float>(keys.getX() + k * keys.getWidth() / 16.0f, keys.getY(), keys.getWidth() / 16.0f - 1.0f, keys.getHeight());
                g.setColour(k % 7 == 1 || k % 7 == 4 ? juce::Colours::black : juce::Colours::ivory);
                g.fillRect(key);
            }

            auto d = industry.reduced(15.0f);
            g.setColour(cyan.withAlpha(0.88f)); g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
            g.drawText("THE MISSING INDUSTRY", d.removeFromTop(22.0f).toNearestInt(), juce::Justification::centredLeft);
            auto machine = d.reduced(2.0f, 10.0f);
            g.setColour(juce::Colour::fromRGB(42, 42, 48)); g.fillRoundedRectangle(machine, 6.0f);
            for (int k = 0; k < 16; ++k)
            {
                const auto x = machine.getX() + 9.0f + k * (machine.getWidth() - 18.0f) / 16.0f;
                g.setColour((k % 4 == 0 ? gold : cyan).withAlpha(0.80f));
                g.fillRoundedRectangle(x, machine.getBottom() - 26.0f, 9.0f, 14.0f, 2.0f);
            }
            drawRandomWalk(g, machine.reduced(20.0f, 42.0f), cyan, 210);
        }

        static void drawFutureVisual(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour colour, int kind)
        {
            g.setColour(juce::Colour::fromRGB(3, 4, 8).withAlpha(0.96f));
            g.fillRoundedRectangle(r, 7.0f);
            g.setColour(colour.withAlpha(0.20f));
            g.drawRoundedRectangle(r, 7.0f, 1.0f);
            auto v = r.reduced(14.0f);

            if (kind == 0)
            {
                auto keys = v.removeFromBottom(46.0f);
                g.setColour(juce::Colour::fromRGB(66, 46, 48)); g.fillRoundedRectangle(v, 5.0f);
                for (int k = 0; k < 5; ++k)
                {
                    const auto x = v.getX() + 24.0f + k * 56.0f;
                    g.setColour(colour.withAlpha(0.88f)); g.fillEllipse(x, v.getY() + 26.0f, 20.0f, 20.0f);
                }
                for (int k = 0; k < 14; ++k)
                {
                    auto key = juce::Rectangle<float>(keys.getX() + k * keys.getWidth() / 14.0f, keys.getY(), keys.getWidth() / 14.0f - 1.0f, keys.getHeight());
                    g.setColour(k % 7 == 1 || k % 7 == 4 ? juce::Colours::black : juce::Colours::ivory); g.fillRect(key);
                }
            }
            else if (kind == 1)
            {
                g.setColour(juce::Colour::fromRGB(170, 170, 168)); g.fillRoundedRectangle(v, 5.0f);
                for (int k = 0; k < 8; ++k)
                {
                    const auto x = v.getX() + 16.0f + k * 38.0f;
                    g.setColour(juce::Colour::fromRGB(45, 45, 48)); g.fillRoundedRectangle(x, v.getBottom() - 42.0f, 20.0f, 28.0f, 2.0f);
                    g.setColour(colour.withAlpha(0.90f)); g.fillRect(x + 8.0f, v.getY() + 22.0f, 4.0f, 45.0f + (float)(k % 3) * 12.0f);
                }
            }
            else if (kind == 2)
            {
                g.setColour(juce::Colour::fromRGB(202, 193, 174)); g.fillRoundedRectangle(v, 5.0f);
                for (int k = 0; k < 16; ++k)
                {
                    const auto x = v.getX() + 8.0f + k * (v.getWidth() - 16.0f) / 16.0f;
                    g.setColour((k % 4 == 0 ? colour : juce::Colour::fromRGB(170, 88, 40)).withAlpha(0.92f));
                    g.fillRoundedRectangle(x, v.getBottom() - 26.0f, 10.0f, 14.0f, 2.0f);
                }
                drawRandomWalk(g, v.reduced(18.0f, 48.0f), colour, 311);
            }
            else
            {
                const auto centre = v.getCentre();
                const auto radius = juce::jmin(v.getWidth(), v.getHeight()) * 0.36f;
                g.setColour(colour.withAlpha(0.10f)); g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
                for (int i = 0; i < 90; ++i)
                {
                    const auto angle = (float)i * 2.39996323f;
                    const auto rr = radius * std::sqrt((float)(i + 1) / 90.0f);
                    const auto p = centre + juce::Point<float>(std::cos(angle) * rr, std::sin(angle) * rr * 0.72f);
                    g.setColour((i % 11 == 0 ? juce::Colours::white : colour).withAlpha(0.65f));
                    g.fillEllipse(p.x - 1.0f, p.y - 1.0f, 2.0f, 2.0f);
                }
                drawRandomWalk(g, v.reduced(28.0f, 50.0f), colour, 510);
            }
        }

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
        float scrollOffset { 0.0f };
        float lastDragY { 0.0f };
        static constexpr float contentHeight = 1920.0f;
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
