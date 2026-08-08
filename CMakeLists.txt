cmake_minimum_required(VERSION 3.22)

project(Veloria VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Veloria
    COMPANY_NAME "Latham Audio"
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

target_sources(Veloria
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/dsp/StochasticOscillator.h
)

target_compile_definitions(Veloria
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# JUCE 8 deprecates Font::getStringWidthFloat, while the current editor still
# uses it only to butt the two brand weights together. The unused-variable
# warning is from a retained visual-state calculation during the CHAOS UI pass.
# Keep build output clean until those tiny editor internals are refactored.
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    target_compile_options(Veloria PRIVATE
        -Wno-deprecated-declarations
        -Wno-unused-variable
    )
endif()

target_link_libraries(Veloria
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
