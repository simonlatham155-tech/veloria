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

# V1 editor build. No picture/image assets are embedded or displayed.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginEditor.cpp" VELORIA_EDITOR_SOURCE)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginEditor.h" VELORIA_EDITOR_HEADER)

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

# What If remains text/diagram based. No picture panel is shown.
set(WHATIF_TOP_VISUAL [[                inner.removeFromTop(14.0f);
                drawHeroIllustration(g, inner.removeFromTop(290.0f), panel, gold, purple, magenta, cyan);
                inner.removeFromTop(24.0f);
]])
set(WHATIF_TOP_REPLACEMENT [[                inner.removeFromTop(14.0f);
]])
string(REPLACE "${WHATIF_TOP_VISUAL}" "${WHATIF_TOP_REPLACEMENT}" VELORIA_EDITOR_HEADER "${VELORIA_EDITOR_HEADER}")
string(REPLACE "static constexpr float contentHeight = 1920.0f;"
               "static constexpr float contentHeight = 1660.0f;"
               VELORIA_EDITOR_HEADER "${VELORIA_EDITOR_HEADER}")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/VeloriaV1Editor.cpp" "${VELORIA_EDITOR_SOURCE}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/PluginEditor.h" "${VELORIA_EDITOR_HEADER}")

target_sources(Veloria
    PRIVATE
        Source/PluginProcessor.cpp
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
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
