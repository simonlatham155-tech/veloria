from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    p = Path(path)
    text = p.read_text()
    if old not in text:
        raise RuntimeError(f"Expected source block not found in {path}: {old[:100]!r}")
    p.write_text(text.replace(old, new, 1))


# DSP: four evolved stochastic dimensions. Defaults are zero so legacy presets
# and existing factory sounds remain unchanged until a performer turns a knob.
osc = "Source/dsp/StochasticOscillator.h"
replace_once(
    osc,
    "    void setAmplitudeStepScale(float value) noexcept { amplitudeStepScale = juce::jlimit(0.10f, 2.0f, value); }\n"
    "    void setTimeStepScale(float value) noexcept { timeStepScale = juce::jlimit(0.10f, 2.0f, value); }\n"
    "    void setWalkOrder(int value) noexcept { walkOrder = juce::jlimit(1, 2, value); }",
    "    void setAmplitudeStepScale(float value) noexcept { amplitudeStepScale = juce::jlimit(0.10f, 2.0f, value); }\n"
    "    void setTimeStepScale(float value) noexcept { timeStepScale = juce::jlimit(0.10f, 2.0f, value); }\n"
    "    void setBoundaryDrive(float value) noexcept { boundaryDrive = juce::jlimit(0.0f, 1.0f, value); }\n"
    "    void setStochasticRate(float value) noexcept { stochasticRate = juce::jlimit(0.0f, 1.0f, value); }\n"
    "    void setJump(float value) noexcept { jump = juce::jlimit(0.0f, 1.0f, value); }\n"
    "    void setCorrelation(float value) noexcept { correlation = juce::jlimit(0.0f, 1.0f, value); }\n"
    "    void setWalkOrder(int value) noexcept { walkOrder = juce::jlimit(1, 2, value); }"
)

replace_once(
    osc,
    "                evolveField();",
    "                evolveFieldRateControlled();"
)

replace_once(
    osc,
    "    void evolveField() noexcept\n    {",
    "    void evolveFieldRateControlled() noexcept\n"
    "    {\n"
    "        const auto rateCurve = stochasticRate * stochasticRate;\n"
    "        const auto passesExact = 1.0f + rateCurve * 7.0f;\n"
    "        const auto wholePasses = juce::jlimit(1, 8, static_cast<int>(std::floor(passesExact)));\n"
    "        for (int pass = 0; pass < wholePasses; ++pass)\n"
    "            evolveField();\n"
    "        if (wholePasses < 8 && random.nextFloat() < passesExact - static_cast<float>(wholePasses))\n"
    "            evolveField();\n"
    "    }\n\n"
    "    [[nodiscard]] float shapeStochasticRandom(float fresh, float& memory) noexcept\n"
    "    {\n"
    "        const auto rho = juce::jlimit(0.0f, 0.985f, correlation * 0.985f);\n"
    "        const auto freshGain = std::sqrt(juce::jmax(0.0f, 1.0f - rho * rho));\n"
    "        auto value = rho * memory + freshGain * fresh;\n"
    "        memory = value;\n"
    "        const auto jumpProbability = jump * jump * 0.28f;\n"
    "        if (jumpProbability > 0.0f && random.nextFloat() < jumpProbability)\n"
    "        {\n"
    "            const auto sign = random.nextBool() ? 1.0f : -1.0f;\n"
    "            value += sign * (0.75f + jump * 3.25f);\n"
    "        }\n"
    "        return value;\n"
    "    }\n\n"
    "    void evolveField() noexcept\n    {"
)

replace_once(
    osc,
    "        const auto ampRandom = distributedRandomMorph(amplitudeDistributionPosition, amplitudeWalk);\n"
    "        const auto timeRandom = distributedRandomMorph(timeDistributionPosition, timeWalk);",
    "        const auto ampFresh = distributedRandomMorph(amplitudeDistributionPosition, amplitudeWalk);\n"
    "        const auto timeFresh = distributedRandomMorph(timeDistributionPosition, timeWalk);\n"
    "        const auto ampRandom = shapeStochasticRandom(ampFresh, amplitudeRandomMemory[index]);\n"
    "        const auto timeRandom = shapeStochasticRandom(timeFresh, timeRandomMemory[index]);\n"
    "        const auto ampProximity = juce::jlimit(0.0f, 1.0f, std::abs(amplitudes[index]) / juce::jmax(0.05f, amplitudeMirror));\n"
    "        const auto timeCentre = 0.5f * (minimumDuration + maximumDuration);\n"
    "        const auto timeHalfSpan = juce::jmax(0.001f, 0.5f * (maximumDuration - minimumDuration));\n"
    "        const auto timeProximity = juce::jlimit(0.0f, 1.0f, std::abs(durations[index] - timeCentre) / timeHalfSpan);\n"
    "        const auto boundaryCurve = boundaryDrive * boundaryDrive;\n"
    "        const auto ampBoundaryBoost = 1.0f + boundaryCurve * 6.0f * ampProximity * ampProximity * ampProximity;\n"
    "        const auto timeBoundaryBoost = 1.0f + boundaryCurve * 6.0f * timeProximity * timeProximity * timeProximity;"
)

# Apply boundary boost to the actual stochastic increments, so it specifically
# increases collision/reflection behaviour near the limits rather than simply
# duplicating the STEP controls everywhere.
for old, new in [
    ("ampRandom * maxAmpStep,", "ampRandom * maxAmpStep * ampBoundaryBoost,"),
    ("timeRandom * maxTimeStep,", "timeRandom * maxTimeStep * timeBoundaryBoost,"),
    ("ampRandom * localAmpStep * 0.35f,", "ampRandom * localAmpStep * 0.35f * ampBoundaryBoost,"),
    ("timeRandom * localTimeStep * 0.35f,", "timeRandom * localTimeStep * 0.35f * timeBoundaryBoost,"),
    ("ampRandom * maxAmpStep * 0.35f,", "ampRandom * maxAmpStep * 0.35f * ampBoundaryBoost,"),
    ("timeRandom * maxTimeStep * 0.35f,", "timeRandom * maxTimeStep * 0.35f * timeBoundaryBoost,")
]:
    replace_once(osc, old, new)

replace_once(
    osc,
    "            timeCascadeState[i] = 0.5f;\n        }",
    "            timeCascadeState[i] = 0.5f;\n"
    "            amplitudeRandomMemory[i] = 0.0f;\n"
    "            timeRandomMemory[i] = 0.0f;\n"
    "        }"
)

replace_once(
    osc,
    "    std::array<float, numBreakpoints> timeCascadeState {};",
    "    std::array<float, numBreakpoints> timeCascadeState {};\n"
    "    std::array<float, numBreakpoints> amplitudeRandomMemory {};\n"
    "    std::array<float, numBreakpoints> timeRandomMemory {};"
)

replace_once(
    osc,
    "    float timeStepScale { 1.0f };\n    float pitchStability { 1.0f };",
    "    float timeStepScale { 1.0f };\n"
    "    float boundaryDrive { 0.0f };\n"
    "    float stochasticRate { 0.0f };\n"
    "    float jump { 0.0f };\n"
    "    float correlation { 0.0f };\n"
    "    float pitchStability { 1.0f };"
)

# Processor parameters + routing.
processor = "Source/PluginProcessor.cpp"
replace_once(
    processor,
    "    layout.push_back(std::make_unique<juce::AudioParameterFloat>(\"chaos\", \"Chaos\", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));",
    "    layout.push_back(std::make_unique<juce::AudioParameterFloat>(\"chaos\", \"Chaos\", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));\n"
    "    layout.push_back(std::make_unique<juce::AudioParameterFloat>(\"boundary\", \"Boundary Drive\", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));\n"
    "    layout.push_back(std::make_unique<juce::AudioParameterFloat>(\"rate\", \"Stochastic Rate\", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));\n"
    "    layout.push_back(std::make_unique<juce::AudioParameterFloat>(\"jump\", \"Stochastic Jump\", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));\n"
    "    layout.push_back(std::make_unique<juce::AudioParameterFloat>(\"correlation\", \"Correlation\", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));"
)
replace_once(
    processor,
    "    const auto chaos = parameters.getRawParameterValue(\"chaos\")->load();",
    "    const auto chaos = parameters.getRawParameterValue(\"chaos\")->load();\n"
    "    const auto boundary = parameters.getRawParameterValue(\"boundary\")->load();\n"
    "    const auto rate = parameters.getRawParameterValue(\"rate\")->load();\n"
    "    const auto jump = parameters.getRawParameterValue(\"jump\")->load();\n"
    "    const auto correlation = parameters.getRawParameterValue(\"correlation\")->load();"
)
replace_once(
    processor,
    "        voice.oscillator.setInterpolationShape(curve);",
    "        voice.oscillator.setInterpolationShape(curve);\n"
    "        voice.oscillator.setBoundaryDrive(boundary);\n"
    "        voice.oscillator.setStochasticRate(rate);\n"
    "        voice.oscillator.setJump(jump);\n"
    "        voice.oscillator.setCorrelation(correlation);"
)

# Include all four in MIDI learn.
proc_header = "Source/PluginProcessor.h"
replace_once(proc_header, "static constexpr int midiLearnParameterCount = 18;", "static constexpr int midiLearnParameterCount = 22;")
replace_once(
    processor,
    "    \"chaos\", \"breakpoints\", \"pitchStability\", \"curve\",\n    \"attack\", \"decay\", \"sustain\", \"release\", \"seed\", \"level\"",
    "    \"chaos\", \"boundary\", \"rate\", \"jump\", \"correlation\",\n"
    "    \"breakpoints\", \"pitchStability\", \"curve\",\n"
    "    \"attack\", \"decay\", \"sustain\", \"release\", \"seed\", \"level\""
)

# Editor declarations.
editor_h = "Source/PluginEditor.h"
replace_once(
    editor_h,
    "    MidiLearnSlider chaos, breakpoints, pitchStability, curve;",
    "    MidiLearnSlider chaos, breakpoints, pitchStability, curve;\n"
    "    MidiLearnSlider boundary, rate, jump, correlation;"
)
replace_once(
    editor_h,
    "        ampStepAttachment, timeStepAttachment, chaosAttachment,\n        breakpointsAttachment, pitchStabilityAttachment, curveAttachment;",
    "        ampStepAttachment, timeStepAttachment, chaosAttachment,\n"
    "        boundaryAttachment, rateAttachment, jumpAttachment, correlationAttachment,\n"
    "        breakpointsAttachment, pitchStabilityAttachment, curveAttachment;"
)
# Move the engine button down to leave a compact four-knob row above the envelopes.
replace_once(editor_h, "            setBounds(103, 600, 215, 20);", "            setBounds(103, 654, 215, 20);")

editor = "Source/PluginEditor.cpp"
replace_once(
    editor,
    "configureKnob(ampStep,\"ampStep\",\"AMP STEP\"); configureKnob(timeStep,\"timeStep\",\"TIME STEP\"); configureKnob(chaos,\"chaos\",\"CHAOS\"); configureKnob(breakpoints,\"breakpoints\",\"POINTS\");",
    "configureKnob(ampStep,\"ampStep\",\"AMP STEP\"); configureKnob(timeStep,\"timeStep\",\"TIME STEP\"); configureKnob(chaos,\"chaos\",\"CHAOS\"); configureKnob(breakpoints,\"breakpoints\",\"POINTS\");\n"
    "    configureKnob(boundary,\"boundary\",\"BOUNDARY\"); configureKnob(rate,\"rate\",\"RATE\"); configureKnob(jump,\"jump\",\"JUMP\"); configureKnob(correlation,\"correlation\",\"CORRELATION\");"
)
replace_once(
    editor,
    "&ampStep,&timeStep,&chaos,&breakpoints,&pitchStability,&curve,&attack",
    "&ampStep,&timeStep,&chaos,&breakpoints,&pitchStability,&curve,&boundary,&rate,&jump,&correlation,&attack"
)
# The same component list occurs again in the destructor; replace the remaining occurrence.
replace_once(
    editor,
    "&ampStep,&timeStep,&chaos,&breakpoints,&pitchStability,&curve,&attack",
    "&ampStep,&timeStep,&chaos,&breakpoints,&pitchStability,&curve,&boundary,&rate,&jump,&correlation,&attack"
)
replace_once(
    editor,
    "&ampStep,&timeStep,&chaos,&pitchStability,&curve,&attack",
    "&ampStep,&timeStep,&chaos,&boundary,&rate,&jump,&correlation,&pitchStability,&curve,&attack"
)
replace_once(
    editor,
    "chaosAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,\"chaos\",chaos); breakpointsAttachment",
    "chaosAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,\"chaos\",chaos); "
    "boundaryAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,\"boundary\",boundary); "
    "rateAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,\"rate\",rate); "
    "jumpAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,\"jump\",jump); "
    "correlationAttachment=std::make_unique<SliderAttachment>(audioProcessor.parameters,\"correlation\",correlation); breakpointsAttachment"
)
replace_once(
    editor,
    "static_cast<juce::Component*>(&chaos),static_cast<juce::Component*>(&breakpoints)",
    "static_cast<juce::Component*>(&chaos),static_cast<juce::Component*>(&boundary),static_cast<juce::Component*>(&rate),static_cast<juce::Component*>(&jump),static_cast<juce::Component*>(&correlation),static_cast<juce::Component*>(&breakpoints)"
)
replace_once(
    editor,
    "curve.setBounds(252,490,70,104);orderButton.setBounds(25,600,68,20);",
    "curve.setBounds(252,490,70,104);"
    "boundary.setBounds(24,596,70,76);rate.setBounds(100,596,70,76);jump.setBounds(176,596,70,76);correlation.setBounds(252,596,70,76);"
    "orderButton.setBounds(25,654,68,20);"
)

print("Applied BOUNDARY, RATE, JUMP and CORRELATION stochastic controls")
