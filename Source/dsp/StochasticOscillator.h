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
        phase = 0.0;
        lifeCounter = 0;
        harmonicState.fill(0.0f);
        harmonicTarget.fill(0.0f);
        chooseTargets();
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
        reset();
    }

    [[nodiscard]] float processSample() noexcept
    {
        const auto evolutionSamples = static_cast<int>(sampleRate * juce::jmap(life, 1.5f, 0.025f));
        if (++lifeCounter >= juce::jmax(1, evolutionSamples))
        {
            lifeCounter = 0;
            chooseTargets();
        }

        const auto smoothing = 0.00015f + life * 0.0035f;
        for (std::size_t i = 0; i < harmonicState.size(); ++i)
            harmonicState[i] += (harmonicTarget[i] - harmonicState[i]) * smoothing;

        const auto angle = static_cast<float>(juce::MathConstants<double>::twoPi * phase);
        float sample = std::sin(angle) * (0.82f + stability * 0.14f);

        const auto bodyAmount = 0.08f + bloom * 0.34f;
        for (std::size_t i = 0; i < harmonicState.size(); ++i)
        {
            const auto harmonic = static_cast<float>(i + 2);
            const auto spectralTilt = std::pow(harmonic, -(0.8f + focus * 2.2f));
            sample += std::sin(angle * harmonic) * harmonicState[i] * spectralTilt * bodyAmount;
        }

        phase += static_cast<double>(frequency) / sampleRate;
        if (phase >= 1.0)
            phase -= std::floor(phase);

        return std::tanh(sample * 0.9f);
    }

private:
    void chooseTargets() noexcept
    {
        const auto movement = (1.0f - stability) * (0.12f + life * 0.88f);
        for (std::size_t i = 0; i < harmonicTarget.size(); ++i)
        {
            const auto randomValue = random.nextFloat() * 2.0f - 1.0f;
            const auto neighbour = i == 0 ? 0.0f : harmonicTarget[i - 1] * 0.22f;
            harmonicTarget[i] = juce::jlimit(-1.0f, 1.0f,
                                              harmonicState[i] * (1.0f - movement)
                                                  + randomValue * movement + neighbour);
        }
    }

    static constexpr std::size_t harmonicCount = 12;
    std::array<float, harmonicCount> harmonicState {};
    std::array<float, harmonicCount> harmonicTarget {};
    juce::Random random { 1 };
    double sampleRate { 44100.0 };
    double phase { 0.0 };
    float frequency { 220.0f };
    float stability { 0.85f };
    float life { 0.35f };
    float focus { 0.55f };
    float bloom { 0.45f };
    int lifeCounter { 0 };
    std::uint32_t seed { 1 };
};
} // namespace veloria::dsp
