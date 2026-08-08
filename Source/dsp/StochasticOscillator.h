#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <cstdint>

namespace veloria::dsp
{
// Veloria dynamic stochastic synthesis core.
//
// One tuned cycle is a polygon whose amplitude and duration breakpoints evolve
// through cascaded reflected random walks.  The oscillator deliberately keeps
// pitch normalisation outside the stochastic geometry: MIDI pitch remains a
// musical contract while the waveform itself is free to evolve.
//
// This is the Mk-I philosophy: probability is part of the oscillator, not an
// LFO bolted onto a conventional waveform.
class StochasticOscillator
{
public:
    static constexpr std::size_t numBreakpoints = 12;

    enum class Distribution
    {
        adaptive,   // musical default: smooth at small walks, heavier tails as energy rises
        uniform,
        gaussian,
        logistic,
        cauchy,
        arcsine
    };

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
    void setAmplitudeDistribution(Distribution value) noexcept { amplitudeDistribution = value; }
    void setTimeDistribution(Distribution value) noexcept { timeDistribution = value; }

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

        // Second-order behaviour: probability changes the velocity of the walk,
        // and that evolving velocity moves the breakpoint.  This gives Veloria
        // memory/continuity instead of unrelated random waveform replacement.
        amplitudeStepState[index] = reflect(
            amplitudeStepState[index]
                + distributedRandom(amplitudeDistribution, amplitudeWalk) * maxAmpStep * 0.35f,
            -maxAmpStep,
            maxAmpStep);

        timeStepState[index] = reflect(
            timeStepState[index]
                + distributedRandom(timeDistribution, timeWalk) * maxTimeStep * 0.35f,
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

    [[nodiscard]] float uniformBipolar() noexcept
    {
        return random.nextFloat() * 2.0f - 1.0f;
    }

    [[nodiscard]] float gaussianBipolar() noexcept
    {
        // Irwin-Hall approximation: strongly favours small organic movements.
        float sum = 0.0f;
        for (int i = 0; i < 6; ++i)
            sum += uniformBipolar();
        return juce::jlimit(-1.0f, 1.0f, sum / 3.0f);
    }

    [[nodiscard]] float logisticBipolar() noexcept
    {
        const auto u = juce::jlimit(0.0001f, 0.9999f, random.nextFloat());
        const auto x = std::log(u / (1.0f - u));
        return juce::jlimit(-1.0f, 1.0f, x * 0.22f);
    }

    [[nodiscard]] float cauchyBipolar() noexcept
    {
        const auto u = juce::jlimit(0.0001f, 0.9999f, random.nextFloat());
        const auto x = std::tan(juce::MathConstants<float>::pi * (u - 0.5f));
        return juce::jlimit(-1.0f, 1.0f, x * 0.16f);
    }

    [[nodiscard]] float arcsineBipolar() noexcept
    {
        const auto u = random.nextFloat();
        return std::sin(juce::MathConstants<float>::pi * (u - 0.5f));
    }

    [[nodiscard]] float distributedRandom(Distribution distribution, float walkEnergy) noexcept
    {
        if (distribution == Distribution::adaptive)
        {
            // Small walks favour natural, correlated micro-motion (pads, strings,
            // bass sustain).  As the stochastic energy opens up, tails become
            // progressively more adventurous (FX and percussion territory).
            if (walkEnergy < 0.09f)
                distribution = Distribution::gaussian;
            else if (walkEnergy < 0.30f)
                distribution = Distribution::uniform;
            else if (walkEnergy < 0.62f)
                distribution = Distribution::logistic;
            else
                distribution = Distribution::cauchy;
        }

        switch (distribution)
        {
            case Distribution::gaussian: return gaussianBipolar();
            case Distribution::logistic: return logisticBipolar();
            case Distribution::cauchy:   return cauchyBipolar();
            case Distribution::arcsine:  return arcsineBipolar();
            case Distribution::uniform:
            case Distribution::adaptive:
            default:                     return uniformBipolar();
        }
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
    Distribution amplitudeDistribution { Distribution::adaptive };
    Distribution timeDistribution { Distribution::adaptive };
    std::uint32_t seed { 1u };
};
} // namespace veloria::dsp
