cmake_minimum_required(VERSION 3.22)

project(Veloria VERSION 0.1.1 LANGUAGES C CXX)

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

# Build the clean V1 editor from generated copies so the What If story can be
# laid out correctly without carrying the temporary MIDI diagnostic UI forward.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginProcessor.cpp" VELORIA_PROCESSOR_SOURCE)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginEditor.cpp" VELORIA_EDITOR_SOURCE)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginEditor.h" VELORIA_EDITOR_HEADER)

# The visual story belongs at the end of the What If narrative, not at the top.
set(WHATIF_TOP_VISUAL [[                inner.removeFromTop(14.0f);
                drawHeroIllustration(g, inner.removeFromTop(290.0f), panel, gold, purple, magenta, cyan);
                inner.removeFromTop(24.0f);
]])
set(WHATIF_TOP_REPLACEMENT [[                inner.removeFromTop(14.0f);
]])
string(REPLACE "${WHATIF_TOP_VISUAL}" "${WHATIF_TOP_REPLACEMENT}" VELORIA_EDITOR_HEADER "${VELORIA_EDITOR_HEADER}")

set(WHATIF_FOOTER [[                auto footer = inner.removeFromTop(54.0f);
]])
set(WHATIF_BOTTOM_VISUAL [[                inner.removeFromTop(26.0f);
                sectionLabel("THE ALTERNATIVE HISTORY  //  THE VISUAL PAYOFF", gold);
                drawHeroIllustration(g, inner.removeFromTop(360.0f), panel, gold, purple, magenta, cyan);
                inner.removeFromTop(28.0f);

                auto footer = inner.removeFromTop(54.0f);
]])
string(REPLACE "${WHATIF_FOOTER}" "${WHATIF_BOTTOM_VISUAL}" VELORIA_EDITOR_HEADER "${VELORIA_EDITOR_HEADER}")
string(REPLACE "static constexpr float contentHeight = 1920.0f;"
               "static constexpr float contentHeight = 2140.0f;"
               VELORIA_EDITOR_HEADER "${VELORIA_EDITOR_HEADER}")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/PluginEditor.h" "${VELORIA_EDITOR_HEADER}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/VeloriaProcessor.cpp" "${VELORIA_PROCESSOR_SOURCE}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/VeloriaEditor.cpp" "${VELORIA_EDITOR_SOURCE}")

target_sources(Veloria
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/VeloriaProcessor.cpp"
        Source/PluginProcessor.h
        "${CMAKE_CURRENT_BINARY_DIR}/VeloriaEditor.cpp"
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
