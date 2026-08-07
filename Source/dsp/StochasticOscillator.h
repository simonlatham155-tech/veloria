#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <cstdint>

namespace veloria::dsp
{
class StochasticOscillator
{
public:
    void prepare(double newSampleRate)
    {
        sampleRate = juce::jmax(1.0, newSampleRate);
        reset();
    }

    void reset() noexcept
    {
        segmentIndex = 0;
        segmentPhase = 0.0;
        initialiseWaveform();
    }

    void setFrequency(float newFrequency) noexcept
    {
        frequency = juce::jlimit(20.0f, 18000.0f, newFrequency);
    }

    void setAmplitudeWalk(float value) noexcept { amplitudeWalk = juce::jlimit(0.0f, 1.0f, value); }
    void setTimeWalk(float value) noexcept { timeWalk = juce::jlimit(0.0f, 1.0f, value); }
    void setCorrelation(float value) noexcept { correlation = juce::jlimit(0.0f, 1.0f, value); }
    void setCurve(float value) noexcept { curve = juce::jlimit(0.0f, 1.0f, value); }

    void setSeed(std::uint32_t newSeed) noexcept
    {
        const auto safeSeed = newSeed == 0 ? 1u : newSeed;
        if (safeSeed == seed)
            return;

        seed = safeSeed;
        random.setSeed(static_cast<juce::int64>(seed));
        reset();
    }

    [[nodiscard]] float processSample() noexcept
    {
        const auto nextIndex = (segmentIndex + 1) % breakpointCount;
        const auto t = interpolationCurve(static_cast<float>(segmentPhase));
        const auto output = juce::jmap(t, amplitudes[segmentIndex], amplitudes[nextIndex]);

        // Durations are normalised each cycle, so their stochastic movement changes
        // waveform geometry while the complete cycle remains locked to MIDI pitch.
        const auto totalDuration = durationSum();
        const auto normalisedDuration = durations[segmentIndex] / totalDuration;
        const auto cyclesPerSample = static_cast<double>(frequency) / sampleRate;
        const auto segmentIncrement = cyclesPerSample
                                    / juce::jmax(1.0e-7, static_cast<double>(normalisedDuration));

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
    void initialiseWaveform() noexcept
    {
        // Start from a neutral, low-complexity closed waveform. From this point on,
        // both amplitude and time coordinates evolve only through bounded random walks.
        for (std::size_t i = 0; i < breakpointCount; ++i)
        {
            const auto angle = juce::MathConstants<float>::twoPi
                             * static_cast<float>(i)
                             / static_cast<float>(breakpointCount);
            amplitudes[i] = std::sin(angle) * 0.7f;
            durations[i] = 1.0f;
        }
    }

    void evolveBreakpoint(std::size_t index) noexcept
    {
        // Direct dynamic-stochastic controls: random walks in amplitude and time,
        // reflected at hard boundaries rather than clipped.
        const auto ampStep = juce::jmap(amplitudeWalk, 0.0005f, 0.22f);
        const auto durStep = juce::jmap(timeWalk, 0.0002f, 0.18f);

        auto newAmplitude = reflect(amplitudes[index] + bipolarRandom() * ampStep, -1.0f, 1.0f);
        auto newDuration = reflect(durations[index] + bipolarRandom() * durStep, 0.16f, 3.0f);

        // Correlation is deliberately local: neighbouring breakpoints can evolve
        // independently or behave more like one coherent moving shape.
        const auto previous = (index + breakpointCount - 1) % breakpointCount;
        const auto next = (index + 1) % breakpointCount;
        const auto neighbourMean = 0.5f * (amplitudes[previous] + amplitudes[next]);
        newAmplitude = juce::jmap(correlation, newAmplitude, neighbourMean);

        amplitudes[index] = reflect(newAmplitude, -1.0f, 1.0f);
        durations[index] = newDuration;
    }

    [[nodiscard]] float interpolationCurve(float t) const noexcept
    {
        // Linear interpolation leaves the breakpoint geometry exposed; increasing
        // Curve progressively smooths the transition without introducing a filter.
        const auto smooth = t * t * (3.0f - 2.0f * t);
        return juce::jmap(curve, t, smooth);
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
        while (value < minimum || value > maximum)
        {
            if (value > maximum)
                value = maximum - (value - maximum);
            else
                value = minimum + (minimum - value);
        }
        return value;
    }

    static constexpr std::size_t breakpointCount = 16;
    std::array<float, breakpointCount> amplitudes {};
    std::array<float, breakpointCount> durations {};

    juce::Random random { 1 };
    double sampleRate { 44100.0 };
    double segmentPhase { 0.0 };
    std::size_t segmentIndex { 0 };
    float frequency { 220.0f };
    float amplitudeWalk { 0.14f };
    float timeWalk { 0.08f };
    float correlation { 0.22f };
    float curve { 0.65f };
    std::uint32_t seed { 1 };
};
} // namespace veloria::dsp
