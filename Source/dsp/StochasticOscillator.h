#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <cstdint>

namespace veloria::dsp
{
// Xenakis-style dynamic stochastic synthesis core.
//
// One waveform cycle is a polygon defined by amplitude/time breakpoints.
// Each breakpoint has two cascaded random walks:
//   1) a primary walk that evolves the step size;
//   2) a secondary walk that moves the breakpoint itself.
// Both walks use reflecting ("mirror") boundaries.
//
// For keyboard use, breakpoint durations are normalised to the requested MIDI
// period. That is an instrument constraint around GENDYN: the waveform geometry
// remains stochastic, while the complete cycle stays tuned to the played note.
class StochasticOscillator
{
public:
    static constexpr std::size_t numBreakpoints = 12;

    void prepare(double newSampleRate)
    {
        sampleRate = juce::jmax(1.0, newSampleRate);
        reset();
    }

    void reset() noexcept
    {
        segmentIndex = 0;
        segmentPhase = 0.0;
        initialiseState();
    }

    void setFrequency(float newFrequency) noexcept
    {
        frequency = juce::jlimit(20.0f, 18000.0f, newFrequency);
    }

    void setAmplitudeWalk(float value) noexcept { amplitudeWalk = juce::jlimit(0.0f, 1.0f, value); }
    void setTimeWalk(float value) noexcept { timeWalk = juce::jlimit(0.0f, 1.0f, value); }
    void setAmplitudeMirror(float value) noexcept { amplitudeMirror = juce::jlimit(0.05f, 1.0f, value); }
    void setTimeMirror(float value) noexcept { timeMirror = juce::jlimit(0.05f, 1.0f, value); }

    void setSeed(std::uint32_t newSeed) noexcept
    {
        const auto safeSeed = newSeed == 0 ? 1u : newSeed;
        seed = safeSeed;
        random.setSeed(static_cast<juce::int64>(seed));
        reset();
    }

    void copyState(std::array<float, numBreakpoints>& amplitudeOut,
                   std::array<float, numBreakpoints>& durationOut) const noexcept
    {
        amplitudeOut = amplitudes;
        durationOut = durations;
    }

    [[nodiscard]] float processSample() noexcept
    {
        const auto nextIndex = (segmentIndex + 1) % numBreakpoints;
        const auto t = static_cast<float>(segmentPhase);

        // GENDYN is breakpoint-interpolation synthesis. Keep interpolation linear:
        // the stochastic polygon itself is the oscillator.
        const auto output = amplitudes[segmentIndex]
                          + (amplitudes[nextIndex] - amplitudes[segmentIndex]) * t;

        const auto totalDuration = durationSum();
        const auto normalisedDuration = durations[segmentIndex] / totalDuration;
        const auto cyclesPerSample = static_cast<double>(frequency) / sampleRate;
        const auto segmentIncrement = cyclesPerSample
                                    / juce::jmax(1.0e-8, static_cast<double>(normalisedDuration));

        segmentPhase += segmentIncrement;

        if (segmentPhase >= 1.0)
        {
            segmentPhase -= std::floor(segmentPhase);
            evolveBreakpoint(segmentIndex);
            segmentIndex = nextIndex;
        }

        return output * 0.82f;
    }

private:
    void initialiseState() noexcept
    {
        for (std::size_t i = 0; i < numBreakpoints; ++i)
        {
            const auto angle = juce::MathConstants<float>::twoPi
                             * static_cast<float>(i)
                             / static_cast<float>(numBreakpoints);
            amplitudes[i] = std::sin(angle) * 0.55f;
            durations[i] = 1.0f;
            amplitudeStepState[i] = 0.0f;
            timeStepState[i] = 0.0f;
        }
    }

    void evolveBreakpoint(std::size_t index) noexcept
    {
        const auto maxAmpStep = juce::jmap(amplitudeWalk, 0.0002f, 0.24f);
        const auto maxTimeStep = juce::jmap(timeWalk, 0.0002f, 0.20f);

        amplitudeStepState[index] = reflect(
            amplitudeStepState[index] + bipolarRandom() * maxAmpStep * 0.35f,
            -maxAmpStep,
            maxAmpStep);

        timeStepState[index] = reflect(
            timeStepState[index] + bipolarRandom() * maxTimeStep * 0.35f,
            -maxTimeStep,
            maxTimeStep);

        amplitudes[index] = reflect(
            amplitudes[index] + amplitudeStepState[index],
            -amplitudeMirror,
            amplitudeMirror);

        const auto minimumDuration = juce::jmax(0.02f, 1.0f - timeMirror * 0.92f);
        const auto maximumDuration = 1.0f + timeMirror * 2.75f;
        durations[index] = reflect(
            durations[index] + timeStepState[index],
            minimumDuration,
            maximumDuration);
    }

    [[nodiscard]] float durationSum() const noexcept
    {
        float total = 0.0f;
        for (const auto duration : durations)
            total += duration;
        return juce::jmax(0.001f, total);
    }

    [[nodiscard]] float bipolarRandom() noexcept
    {
        return random.nextFloat() * 2.0f - 1.0f;
    }

    [[nodiscard]] static float reflect(float value, float minimum, float maximum) noexcept
    {
        if (maximum <= minimum)
            return minimum;

        while (value < minimum || value > maximum)
        {
            if (value > maximum)
                value = maximum - (value - maximum);
            else
                value = minimum + (minimum - value);
        }
        return value;
    }

    std::array<float, numBreakpoints> amplitudes {};
    std::array<float, numBreakpoints> durations {};
    std::array<float, numBreakpoints> amplitudeStepState {};
    std::array<float, numBreakpoints> timeStepState {};

    juce::Random random { 1 };
    double sampleRate { 44100.0 };
    double segmentPhase { 0.0 };
    std::size_t segmentIndex { 0 };

    float frequency { 220.0f };
    float amplitudeWalk { 0.12f };
    float timeWalk { 0.08f };
    float amplitudeMirror { 0.88f };
    float timeMirror { 0.45f };
    std::uint32_t seed { 1u };
};
} // namespace veloria::dsp
