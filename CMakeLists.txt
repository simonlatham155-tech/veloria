cmake_minimum_required(VERSION 3.22)

project(Veloria VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.14
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Veloria
    COMPANY_NAME "LATHAMAUDIO"
    BUNDLE_ID com.lathamaudio.veloria
    PLUGIN_MANUFACTURER_CODE Latm
    PLUGIN_CODE Vlra
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Veloria"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(Veloria)

# The supplied history artwork is embedded only for the WHAT IF? story.
# It is not shown anywhere in the normal Veloria instrument interface.
juce_add_binary_data(VeloriaAssets
    SOURCES
        assets/what_if_history.jpg
)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginProcessor.cpp" VELORIA_PROCESSOR_SOURCE)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginEditor.cpp" VELORIA_EDITOR_SOURCE)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginEditor.h" VELORIA_EDITOR_HEADER)

# Give the generated editor access to the embedded history JPEG.
string(REPLACE
    "#include <JuceHeader.h>"
    "#include <JuceHeader.h>\n#include <BinaryData.h>"
    VELORIA_EDITOR_HEADER "${VELORIA_EDITOR_HEADER}")

# CHAOS no longer detunes the performed pitch centre. The shared note-event layer
# in PluginProcessor.h supplies the actual harmonic stochastic takeover in both
# VELORIA and BROWN IDSS operating models.
string(REPLACE
    "const auto effectivePitchStability = juce::jlimit(0.0f, 1.0f, pitchStability * (1.0f - chaosSquared * 0.58f));"
    "const auto effectivePitchStability = pitchStability;"
    VELORIA_PROCESSOR_SOURCE "${VELORIA_PROCESSOR_SOURCE}")

# Smooth stochastic controls. The six distribution laws remain genuine discrete
# algorithms internally, but the parameter is continuous and the oscillator
# cross-morphs adjacent statistical outputs. POINTS similarly grows the next
# breakpoint continuously out of the closing segment.
string(REPLACE
    "layout.push_back(std::make_unique<juce::AudioParameterInt>(\"ampDist\", \"Amplitude Distribution\", 0, 5, 0));"
    "layout.push_back(std::make_unique<juce::AudioParameterFloat>(\"ampDist\", \"Amplitude Distribution Morph\", juce::NormalisableRange<float>(0.0f, 5.0f), 0.0f));"
    VELORIA_PROCESSOR_SOURCE "${VELORIA_PROCESSOR_SOURCE}")
string(REPLACE
    "layout.push_back(std::make_unique<juce::AudioParameterInt>(\"timeDist\", \"Time Distribution\", 0, 5, 0));"
    "layout.push_back(std::make_unique<juce::AudioParameterFloat>(\"timeDist\", \"Time Distribution Morph\", juce::NormalisableRange<float>(0.0f, 5.0f), 0.0f));"
    VELORIA_PROCESSOR_SOURCE "${VELORIA_PROCESSOR_SOURCE}")
string(REPLACE
    "layout.push_back(std::make_unique<juce::AudioParameterInt>(\"breakpoints\", \"Breakpoints\", 4, 12, 12));"
    "layout.push_back(std::make_unique<juce::AudioParameterFloat>(\"breakpoints\", \"Breakpoint Morph\", juce::NormalisableRange<float>(4.0f, 12.0f), 12.0f));"
    VELORIA_PROCESSOR_SOURCE "${VELORIA_PROCESSOR_SOURCE}")

string(REPLACE
    "const auto ampDist = distributionFromParameter(parameters.getRawParameterValue(\"ampDist\")->load());"
    "const auto ampDist = parameters.getRawParameterValue(\"ampDist\")->load();"
    VELORIA_PROCESSOR_SOURCE "${VELORIA_PROCESSOR_SOURCE}")
string(REPLACE
    "const auto timeDist = distributionFromParameter(parameters.getRawParameterValue(\"timeDist\")->load());"
    "const auto timeDist = parameters.getRawParameterValue(\"timeDist\")->load();"
    VELORIA_PROCESSOR_SOURCE "${VELORIA_PROCESSOR_SOURCE}")
string(REPLACE
    "const auto breakpoints = static_cast<int>(std::lround(parameters.getRawParameterValue(\"breakpoints\")->load()));"
    "const auto breakpoints = parameters.getRawParameterValue(\"breakpoints\")->load();"
    VELORIA_PROCESSOR_SOURCE "${VELORIA_PROCESSOR_SOURCE}")
string(REPLACE
    "voice.oscillator.setAmplitudeDistribution(ampDist);"
    "voice.oscillator.setAmplitudeDistributionMorph(ampDist);"
    VELORIA_PROCESSOR_SOURCE "${VELORIA_PROCESSOR_SOURCE}")
string(REPLACE
    "voice.oscillator.setTimeDistribution(timeDist);"
    "voice.oscillator.setTimeDistributionMorph(timeDist);"
    VELORIA_PROCESSOR_SOURCE "${VELORIA_PROCESSOR_SOURCE}")

# Let the three morph knobs show their in-between positions instead of visually
# pretending they are still hard integer selectors.
string(REPLACE
    "ampDist.setNumDecimalPlacesToDisplay(0); timeDist.setNumDecimalPlacesToDisplay(0); breakpoints.setNumDecimalPlacesToDisplay(0);"
    "ampDist.setNumDecimalPlacesToDisplay(2); timeDist.setNumDecimalPlacesToDisplay(2); breakpoints.setNumDecimalPlacesToDisplay(2);"
    VELORIA_EDITOR_SOURCE "${VELORIA_EDITOR_SOURCE}")

# Replace only the VELORIA wordmark. LATHAMAUDIO and the rest of the header remain untouched.
set(OLD_WORDMARK [[void drawVeloriaWordmark(juce::Graphics& g, juce::Rectangle<float> area)
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
]])

set(NEW_WORDMARK [[void drawVeloriaWordmark(juce::Graphics& g, juce::Rectangle<float> area)
{
    // Ailerons-inspired custom vector wordmark: clean, open, geometric, and portable.
    const auto h = area.getHeight();
    const float top = area.getY() + h * 0.12f;
    const float bot = area.getY() + h * 0.88f;
    const float mid = (top + bot) * 0.5f;
    const float glyphH = bot - top;
    const float stroke = juce::jmax(1.35f, h * 0.058f);

    const std::array<float, 7> widths {{ 34.0f, 39.0f, 33.0f, 40.0f, 40.0f, 12.0f, 35.0f }};
    const float gap = 8.0f;
    float totalW = gap * 6.0f;
    for (auto w : widths) totalW += w;
    const float scale = juce::jmin(1.0f, area.getWidth() / totalW);
    const float x0 = area.getCentreX() - totalW * scale * 0.5f;

    juce::ColourGradient metal(juce::Colour::fromRGB(255, 247, 216), area.getX(), area.getCentreY(),
                               juce::Colour::fromRGB(214, 165, 96), area.getRight(), area.getCentreY(), false);
    metal.addColour(0.50, juce::Colour::fromRGB(255, 231, 172));
    g.setGradientFill(metal);

    juce::Path p;
    float x = x0;
    auto sx = [scale](float v) { return v * scale; };
    auto next = [&](int i) { const auto here = x; x += sx(widths[(std::size_t)i] + (i < 6 ? gap : 0.0f)); return here; };

    // V
    {
        const auto a = next(0), w = sx(widths[0]);
        p.startNewSubPath(a, top); p.lineTo(a + w * 0.50f, bot); p.lineTo(a + w, top);
    }
    // E - open rounded right side, like the Ailerons reference
    {
        const auto a = next(1), w = sx(widths[1]);
        p.startNewSubPath(a + w, top); p.lineTo(a + w * 0.18f, top);
        p.cubicTo(a + w * 0.05f, top, a, top + glyphH * 0.08f, a, top + glyphH * 0.22f);
        p.lineTo(a, bot - glyphH * 0.22f);
        p.cubicTo(a, bot - glyphH * 0.08f, a + w * 0.05f, bot, a + w * 0.18f, bot);
        p.lineTo(a + w, bot);
        p.startNewSubPath(a, mid); p.lineTo(a + w * 0.72f, mid);
    }
    // L
    {
        const auto a = next(2), w = sx(widths[2]);
        p.startNewSubPath(a, top); p.lineTo(a, bot); p.lineTo(a + w, bot);
    }
    // O - rounded rectangle
    {
        const auto a = next(3), w = sx(widths[3]);
        const float r = glyphH * 0.22f;
        p.startNewSubPath(a + r, top);
        p.lineTo(a + w - r, top);
        p.cubicTo(a + w - r * 0.35f, top, a + w, top + r * 0.35f, a + w, top + r);
        p.lineTo(a + w, bot - r);
        p.cubicTo(a + w, bot - r * 0.35f, a + w - r * 0.35f, bot, a + w - r, bot);
        p.lineTo(a + r, bot);
        p.cubicTo(a + r * 0.35f, bot, a, bot - r * 0.35f, a, bot - r);
        p.lineTo(a, top + r);
        p.cubicTo(a, top + r * 0.35f, a + r * 0.35f, top, a + r, top);
    }
    // R - rounded bowl with clean diagonal leg
    {
        const auto a = next(4), w = sx(widths[4]);
        p.startNewSubPath(a, bot); p.lineTo(a, top);
        p.lineTo(a + w * 0.62f, top);
        p.cubicTo(a + w * 0.88f, top, a + w, top + glyphH * 0.10f, a + w, top + glyphH * 0.25f);
        p.cubicTo(a + w, top + glyphH * 0.42f, a + w * 0.88f, mid, a + w * 0.62f, mid);
        p.lineTo(a, mid);
        p.startNewSubPath(a + w * 0.58f, mid); p.lineTo(a + w, bot);
    }
    // I
    {
        const auto a = next(5), w = sx(widths[5]);
        p.startNewSubPath(a + w * 0.5f, top); p.lineTo(a + w * 0.5f, bot);
    }
    // A - open apex, no crossbar, matching the reference character
    {
        const auto a = next(6), w = sx(widths[6]);
        p.startNewSubPath(a, bot); p.lineTo(a + w * 0.50f, top); p.lineTo(a + w, bot);
    }

    g.strokePath(p, juce::PathStrokeType(stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
]])

string(REPLACE "${OLD_WORDMARK}" "${NEW_WORDMARK}" VELORIA_EDITOR_SOURCE "${VELORIA_EDITOR_SOURCE}")

# WHAT IF? keeps the written history first. Remove the old top hero so nothing
# flashes before the story begins.
set(WHATIF_TOP_VISUAL [[                inner.removeFromTop(14.0f);
                drawHeroIllustration(g, inner.removeFromTop(290.0f), panel, gold, purple, magenta, cyan);
                inner.removeFromTop(24.0f);
]])
set(WHATIF_TOP_REPLACEMENT [[                inner.removeFromTop(14.0f);
]])
string(REPLACE "${WHATIF_TOP_VISUAL}" "${WHATIF_TOP_REPLACEMENT}" VELORIA_EDITOR_HEADER "${VELORIA_EDITOR_HEADER}")

# Put the supplied history artwork after the written history. Do not force it into
# a fixed 900x1272 box: use the embedded image's real aspect ratio and never scale
# above its native pixel width. That prevents the 'blown-up thumbnail' look.
set(WHATIF_FOOTER [[                auto footer = inner.removeFromTop(54.0f);
]])
set(WHATIF_BOTTOM_PICTURE [[                inner.removeFromTop(26.0f);
                sectionLabel("THE EVOLUTION OF STOCHASTIC SYNTHESIS  //  THE HISTORY PICTURE", gold);
                {
                    static const auto historyImage = juce::ImageFileFormat::loadFrom(
                        BinaryData::what_if_history_jpg,
                        BinaryData::what_if_history_jpgSize);

                    if (historyImage.isValid())
                    {
                        const auto nativeW = (float) historyImage.getWidth();
                        const auto nativeH = (float) historyImage.getHeight();
                        const auto availableW = juce::jmax(1.0f, inner.getWidth() - 32.0f);
                        const auto pictureW = juce::jmin(nativeW, availableW);
                        const auto pictureH = pictureW * nativeH / juce::jmax(1.0f, nativeW);
                        auto imageBand = inner.removeFromTop(pictureH + 24.0f);
                        auto picture = juce::Rectangle<float>(0.0f, 0.0f, pictureW, pictureH)
                                           .withCentre(imageBand.getCentre());

                        g.setColour(panel.withAlpha(0.97f));
                        g.fillRoundedRectangle(picture.expanded(10.0f), 10.0f);
                        g.setColour(gold.withAlpha(0.30f));
                        g.drawRoundedRectangle(picture.expanded(10.0f), 10.0f, 1.2f);

                        juce::Graphics::ScopedSaveState imageState(g);
                        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
                        g.drawImage(historyImage,
                                    picture.getX(), picture.getY(), picture.getWidth(), picture.getHeight(),
                                    0.0f, 0.0f, nativeW, nativeH,
                                    false);
                    }
                }
                inner.removeFromTop(28.0f);

                auto footer = inner.removeFromTop(54.0f);
]])
string(REPLACE "${WHATIF_FOOTER}" "${WHATIF_BOTTOM_PICTURE}" VELORIA_EDITOR_HEADER "${VELORIA_EDITOR_HEADER}")

# Allow enough scroll room for the full portrait at its real aspect ratio.
string(REPLACE "static constexpr float contentHeight = 1920.0f;"
               "static constexpr float contentHeight = 3370.0f;"
               VELORIA_EDITOR_HEADER "${VELORIA_EDITOR_HEADER}")

# The old editor paint path used to draw a separate WHAT IF overlay immediately,
# then the RevisedWhatIfOverlay component appeared on its timer. That is the image
# flash seen before the text page. Disable the old renderer completely.
string(REPLACE
    "if(whatIfOpen){drawWhatIfOverlay(g);return;}"
    "if(whatIfOpen){return;}"
    VELORIA_EDITOR_SOURCE "${VELORIA_EDITOR_SOURCE}")

# Put the revised overlay in the component hierarchy from construction time and
# switch it synchronously when WHAT IF is pressed, rather than waiting for its timer.
string(REPLACE
    "visualState=audioProcessor.getVisualState(); startTimerHz(30);"
    "addAndMakeVisible(revisedWhatIfOverlay); revisedWhatIfOverlay.setVisible(false); visualState=audioProcessor.getVisualState(); startTimerHz(30);"
    VELORIA_EDITOR_SOURCE "${VELORIA_EDITOR_SOURCE}")
string(REPLACE
    "c->setVisible(!whatIfOpen); repaint();"
    "c->setVisible(!whatIfOpen); revisedWhatIfOverlay.setVisible(whatIfOpen); if(whatIfOpen) revisedWhatIfOverlay.toFront(false); repaint();"
    VELORIA_EDITOR_SOURCE "${VELORIA_EDITOR_SOURCE}")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/VeloriaV1Processor.cpp" "${VELORIA_PROCESSOR_SOURCE}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/VeloriaV1Editor.cpp" "${VELORIA_EDITOR_SOURCE}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/PluginEditor.h" "${VELORIA_EDITOR_HEADER}")

target_sources(Veloria
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/VeloriaV1Processor.cpp"
        Source/PluginProcessor.h
        "${CMAKE_CURRENT_BINARY_DIR}/VeloriaV1Editor.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/PluginEditor.h"
        Source/dsp/StochasticOscillator.h
)

target_include_directories(Veloria PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source"
)

target_compile_definitions(Veloria
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Veloria
    PRIVATE
        VeloriaAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
