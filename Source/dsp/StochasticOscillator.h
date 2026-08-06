#pragma once

#include <JuceHeader.h>
#include <array>
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
        phase = 0.0;
        current = 0.0f;
        target = 0.0f;
        segmentPosition = 0;
        chooseNextTarget();
    }

    void setFrequency(float newFrequency) noexcept
    {
        frequency = juce::jlimit(1.0f, 20000.0f, newFrequency);
    }

    void setInstability(float amount) noexcept
    {
        instability = juce::jlimit(0.0f, 1.0f, amount);
    }

    void setSeed(std::uint32_t newSeed) noexcept
    {
        seed = newSeed == 0 ? 1u : newSeed;
        random.setSeed(static_cast<juce::int64>(seed));
        reset();
    }

    [[nodiscard]] float processSample() noexcept
    {
        const auto samplesPerCycle = sampleRate / static_cast<double>(frequency);
        const auto segmentLength = juce::jmax(1, static_cast<int>(samplesPerCycle / breakpoints));
        const auto t = static_cast<float>(segmentPosition) / static_cast<float>(segmentLength);
        const auto sample = juce::jmap(t, current, target);

        if (++segmentPosition >= segmentLength)
        {
            segmentPosition = 0;
            current = target;
            chooseNextTarget();
        }

        phase += frequency / sampleRate;
        if (phase >= 1.0)
            phase -= 1.0;

        return sample;
    }

private:
    void chooseNextTarget() noexcept
    {
        const auto randomValue = random.nextFloat() * 2.0f - 1.0f;
        const auto retainedIdentity = 1.0f - instability;
        target = juce::jlimit(-1.0f, 1.0f,
                              retainedIdentity * current + instability * randomValue);
    }

    juce::Random random { 1 };
    double sampleRate { 44100.0 };
    double phase { 0.0 };
    float frequency { 220.0f };
    float instability { 0.35f };
    float current { 0.0f };
    float target { 0.0f };
    int segmentPosition { 0 };
    int breakpoints { 12 };
    std::uint32_t seed { 1 };
};
} // namespace veloria::dsp
