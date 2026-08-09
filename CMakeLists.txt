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

# -----------------------------------------------------------------------------
# 0.1.1 TEST diagnostic instrumentation
# Generate temporary instrumented .cpp files at configure time. The normal source
# remains readable and easy to restore after the MIDI Note Off fault is isolated.
# -----------------------------------------------------------------------------
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginProcessor.cpp" VELORIA_PROCESSOR_SOURCE)

set(OLD_PREPARE [[    currentSampleRate = juce::jmax(1.0, sampleRate);
]])
set(NEW_PREPARE [[    currentSampleRate = juce::jmax(1.0, sampleRate);
    midiNoteOnCount.store(0, std::memory_order_relaxed);
    midiNoteOffCount.store(0, std::memory_order_relaxed);
    midiNoteOffMatchCount.store(0, std::memory_order_relaxed);
]])
string(REPLACE "${OLD_PREPARE}" "${NEW_PREPARE}" VELORIA_PROCESSOR_SOURCE "${VELORIA_PROCESSOR_SOURCE}")

set(OLD_MIDI [[        if (message.isNoteOn(false))
            startNote(message.getNoteNumber(), message.getFloatVelocity());
        else if (message.isNoteOff(true))
            stopNote(message.getNoteNumber());
        else if (message.isAllNotesOff() || message.isAllSoundOff())
            stopAllVoices(false);
]])
set(NEW_MIDI [[        if (message.isNoteOn(false))
        {
            midiNoteOnCount.fetch_add(1, std::memory_order_relaxed);
            startNote(message.getNoteNumber(), message.getFloatVelocity());
        }
        else if (message.isNoteOff(true))
        {
            midiNoteOffCount.fetch_add(1, std::memory_order_relaxed);
            stopNote(message.getNoteNumber());
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
            stopAllVoices(false);
]])
string(REPLACE "${OLD_MIDI}" "${NEW_MIDI}" VELORIA_PROCESSOR_SOURCE "${VELORIA_PROCESSOR_SOURCE}")

set(OLD_STOPNOTE [[void VeloriaAudioProcessor::stopNote(int midiNote)
{
    if (drumMode)
        return;

    for (auto& voice : voices)
    {
        if (voice.active && ! voice.percussion && voice.midiNote == midiNote)
        {
            voice.held = false;
            voice.envelope.noteOff();
        }
    }
}
]])
set(NEW_STOPNOTE [[void VeloriaAudioProcessor::stopNote(int midiNote)
{
    if (drumMode)
        return;

    bool matched = false;
    for (auto& voice : voices)
    {
        if (voice.active && ! voice.percussion && voice.midiNote == midiNote)
        {
            matched = true;
            voice.held = false;
            voice.envelope.noteOff();
        }
    }

    if (matched)
        midiNoteOffMatchCount.fetch_add(1, std::memory_order_relaxed);
}
]])
string(REPLACE "${OLD_STOPNOTE}" "${NEW_STOPNOTE}" VELORIA_PROCESSOR_SOURCE "${VELORIA_PROCESSOR_SOURCE}")

set(OLD_VISUAL_RETURN [[    state.activeVoices = visualActiveVoices.load(std::memory_order_relaxed);
    state.energy = visualEnergy.load(std::memory_order_relaxed);
    return state;
]])
set(NEW_VISUAL_RETURN [[    state.activeVoices = visualActiveVoices.load(std::memory_order_relaxed);
    state.energy = visualEnergy.load(std::memory_order_relaxed);
    state.midiNoteOns = midiNoteOnCount.load(std::memory_order_relaxed);
    state.midiNoteOffs = midiNoteOffCount.load(std::memory_order_relaxed);
    state.midiNoteOffMatches = midiNoteOffMatchCount.load(std::memory_order_relaxed);
    return state;
]])
string(REPLACE "${OLD_VISUAL_RETURN}" "${NEW_VISUAL_RETURN}" VELORIA_PROCESSOR_SOURCE "${VELORIA_PROCESSOR_SOURCE}")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/VeloriaDiagnosticProcessor.cpp" "${VELORIA_PROCESSOR_SOURCE}")

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginEditor.cpp" VELORIA_EDITOR_SOURCE)
set(OLD_STATUS [[    voiceStatus.setText("VOICE ENGINE   "+juce::String(visualState.activeVoices)+" / 8 ACTIVE",juce::dontSendNotification); fieldStatus.setText(juce::String(visualState.activeVoices)+" VOICES  //  "+juce::String((int)(visualState.energy*100.0f))+"% ENERGY",juce::dontSendNotification);
]])
set(NEW_STATUS [[    voiceStatus.setText("ON "+juce::String(visualState.midiNoteOns)+"  OFF "+juce::String(visualState.midiNoteOffs)+"  MATCH "+juce::String(visualState.midiNoteOffMatches),juce::dontSendNotification); fieldStatus.setText(juce::String(visualState.activeVoices)+" VOICES  //  "+juce::String((int)(visualState.energy*100.0f))+"% ENERGY",juce::dontSendNotification);
    footerStatus.setText("VELORIA "+juce::String(JucePlugin_VersionString)+" TEST  //  JUCE 8.0.14  //  MIDI NOTE-OFF DIAGNOSTIC",juce::dontSendNotification);
]])
string(REPLACE "${OLD_STATUS}" "${NEW_STATUS}" VELORIA_EDITOR_SOURCE "${VELORIA_EDITOR_SOURCE}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/VeloriaDiagnosticEditor.cpp" "${VELORIA_EDITOR_SOURCE}")

target_sources(Veloria
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/VeloriaDiagnosticProcessor.cpp"
        Source/PluginProcessor.h
        "${CMAKE_CURRENT_BINARY_DIR}/VeloriaDiagnosticEditor.cpp"
        Source/PluginEditor.h
        Source/dsp/StochasticOscillator.h
)

target_include_directories(Veloria PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")

target_compile_definitions(Veloria
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        VELORIA_DIAGNOSTIC_BUILD=1
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
