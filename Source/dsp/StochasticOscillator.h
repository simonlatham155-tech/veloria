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
        initialiseWaveform();
    }

    void reset() noexcept
    {
        phase = 0.0;
        segmentIndex = 0;
        segmentPhase = 0.0;
        initialiseWaveform();
    }

    void setFrequency(float newFrequency) noexcept
    {
        frequency = juce::jlimit(20.0f, 18000.0f, newFrequency);
    }

    void setStability(float value) noexcept { stability = juce::jlimit(0.0f, 1.0f, value); }
    void setLife(float value) noexcept { life = juce::jlimit(0.0f, 1.0f, value); }
    void setFocus(float value) noexcept { focus = juce::jlimit(0.0f, 1.0f, value); }
    void setBloom(float value) noexcept { bloom = juce::jlimit(0.0f, 1.0f, value); }

    void setSeed(std::uint32_t newSeed) noexcept
    {
        const auto safeSeed = newSeed == 0 ? 1u : newSeed;
        if (safeSeed == seed)
            return;

        seed = safeSeed;
        random.setSeed(static_cast<juce::int64>(seed));
        initialiseWaveform();
    }

    [[nodiscard]] float processSample() noexcept
    {
        const auto nextIndex = (segmentIndex + 1) % breakpointCount;
        const auto shapedPhase = interpolationCurve(static_cast<float>(segmentPhase));
        const auto sample = juce::jmap(shapedPhase,
                                      amplitudes[segmentIndex],
                                      amplitudes[nextIndex]);

        const auto totalDuration = durationSum();
        const auto cyclesPerSample = static_cast<double>(frequency) / sampleRate;
        const auto normalisedDuration = durations[segmentIndex] / totalDuration;
        const auto segmentIncrement = cyclesPerSample / juce::jmax(1.0e-6, static_cast<double>(normalisedDuration));

        segmentPhase += segmentIncrement;
        phase += cyclesPerSample;

        if (segmentPhase >= 1.0)
        {
            segmentPhase -= std::floor(segmentPhase);
            evolveBreakpoint(segmentIndex);
            segmentIndex = nextIndex;
        }

        if (phase >= 1.0)
            phase -= std::floor(phase);

        return std::tanh(sample * (0.85f + bloom * 0.65f));
    }

private:
    void initialiseWaveform() noexcept
    {
        phase = 0.0;
        segmentIndex = 0;
        segmentPhase = 0.0;

        for (std::size_t i = 0; i < breakpointCount; ++i)
        {
            const auto angle = juce::MathConstants<float>::twoPi
                             * static_cast<float>(i)
                             / static_cast<float>(breakpointCount);
            amplitudes[i] = std::sin(angle) * 0.75f;
            durations[i] = 1.0f;
        }
    }

    void evolveBreakpoint(std::size_t index) noexcept
    {
        const auto amplitudeStep = (0.015f + life * 0.18f) * (1.0f - stability * 0.92f);
        const auto durationStep = (0.004f + life * 0.055f) * (1.0f - stability * 0.85f);

        const auto spectralBias = 0.35f + focus * 0.65f;
        const auto amplitudeDelta = bipolarRandom() * amplitudeStep * spectralBias;
        const auto durationDelta = bipolarRandom() * durationStep;

        amplitudes[index] = reflect(amplitudes[index] + amplitudeDelta, -1.0f, 1.0f);
        durations[index] = reflect(durations[index] + durationDelta, 0.25f, 2.5f);

        const auto neighbour = (index + breakpointCount - 1) % breakpointCount;
        const auto correlation = 0.08f + focus * 0.42f;
        amplitudes[index] = juce::jmap(correlation, amplitudes[index], amplitudes[neighbour]);
    }

    [[nodiscard]] float interpolationCurve(float t) const noexcept
    {
        const auto smooth = t * t * (3.0f - 2.0f * t);
        return juce::jmap(focus, t, smooth);
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
            else if (value < minimum)
                value = minimum + (minimum - value);
        }
        return value;
    }

    static constexpr std::size_t breakpointCount = 16;
    std::array<float, breakpointCount> amplitudes {};
    std::array<float, breakpointCount> durations {};

    juce::Random random { 1 };
    double sampleRate { 44100.0 };
    double phase { 0.0 };
    double segmentPhase { 0.0 };
    std::size_t segmentIndex { 0 };
    float frequency { 220.0f };
    float stability { 0.85f };
    float life { 0.35f };
    float focus { 0.55f };
    float bloom { 0.45f };
    std::uint32_t seed { 1 };
};
} // namespace veloria::dsp
