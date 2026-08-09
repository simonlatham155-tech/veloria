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

# V1 editor build. The supplied artwork is a visual reference only: no image
# assets are embedded or displayed in Veloria.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginEditor.cpp" VELORIA_EDITOR_SOURCE)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginEditor.h" VELORIA_EDITOR_HEADER)

# Match the compact geometric VELORIA wordmark from the approved reference.
# LATHAMAUDIO and every other header element remain unchanged.
string(REPLACE
    "const auto stroke = juce::jmax(1.0f, h * 0.045f);\n    const float glyphW = h * 0.78f;\n    const float gap = h * 0.16f;"
    "const auto stroke = juce::jmax(1.2f, h * 0.060f);\n    const float glyphW = h * 0.70f;\n    const float gap = h * 0.055f;"
    VELORIA_EDITOR_SOURCE "${VELORIA_EDITOR_SOURCE}")

string(REPLACE
    "g.setColour(juce::Colours::white.withAlpha(0.94f));\n    juce::Path p;"
    "juce::ColourGradient veloriaGold(juce::Colour::fromRGB(255, 246, 213), area.getX(), area.getCentreY(),\n                                      juce::Colour::fromRGB(218, 164, 92), area.getRight(), area.getCentreY(), false);\n    veloriaGold.addColour(0.52, juce::Colour::fromRGB(255, 232, 174));\n    g.setGradientFill(veloriaGold);\n    juce::Path p;"
    VELORIA_EDITOR_SOURCE "${VELORIA_EDITOR_SOURCE}")

# What If is a scrollable written/diagrammatic feature. The former large visual
# story panel is removed completely; there is no poster/photo at top or bottom.
set(WHATIF_TOP_VISUAL [[                inner.removeFromTop(14.0f);
                drawHeroIllustration(g, inner.removeFromTop(290.0f), panel, gold, purple, magenta, cyan);
                inner.removeFromTop(24.0f);
]])
set(WHATIF_TOP_REPLACEMENT [[                inner.removeFromTop(14.0f);
]])
string(REPLACE "${WHATIF_TOP_VISUAL}" "${WHATIF_TOP_REPLACEMENT}" VELORIA_EDITOR_HEADER "${VELORIA_EDITOR_HEADER}")

# Reduce the scroll document to the actual history/evolution content now that
# the large picture panel is gone.
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
