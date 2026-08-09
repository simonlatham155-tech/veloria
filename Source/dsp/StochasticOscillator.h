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
    static constexpr std::size_t numBreakpoints = 12;

    enum class Distribution
    {
        adaptive,
        uniform,
        gaussian,
        logistic,
        cauchy,
        arcsine
    };

    enum class OperatingModel
    {
        veloria,
        brownIdss
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
    void setAmplitudeStepScale(float value) noexcept { amplitudeStepScale = juce::jlimit(0.10f, 2.0f, value); }
    void setTimeStepScale(float value) noexcept { timeStepScale = juce::jlimit(0.10f, 2.0f, value); }
    void setWalkOrder(int value) noexcept { walkOrder = juce::jlimit(1, 2, value); }
    void setOperatingModel(OperatingModel value) noexcept { operatingModel = value; }
    void setActiveBreakpointCount(int value) noexcept
    {
        activeBreakpointCount = static_cast<std::size_t>(juce::jlimit(4, static_cast<int>(numBreakpoints), value));
        if (segmentIndex >= activeBreakpointCount)
            segmentIndex = 0;
    }
    void setPitchStability(float value) noexcept { pitchStability = juce::jlimit(0.0f, 1.0f, value); }
    void setInterpolationShape(float value) noexcept { interpolationShape = juce::jlimit(0.0f, 1.0f, value); }

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
        const auto count = juce::jmax(static_cast<std::size_t>(4), activeBreakpointCount);
        if (segmentIndex >= count)
            segmentIndex = 0;

        const auto nextIndex = (segmentIndex + 1) % count;
        const auto rawT = static_cast<float>(segmentPhase);
        const auto t = interpolationFor(rawT);
        const auto output = amplitudes[segmentIndex]
                          + (amplitudes[nextIndex] - amplitudes[segmentIndex]) * t;

        const auto totalDuration = durationSum();
        const auto normalisedDuration = durations[segmentIndex] / totalDuration;

        // Veloria can continuously normalise the emergent duration field back toward
        // keyboard pitch. Brown IDSS mode additionally makes high PITCH LOCK behave
        // like IDSS's fixed-segment-length stabilisation control.
        const auto driftRatio = static_cast<float>(count) / juce::jmax(0.001f, totalDuration);
        const auto pitchRatio = juce::jmap(pitchStability, driftRatio, 1.0f);
        const auto cyclesPerSample = static_cast<double>(frequency * pitchRatio) / sampleRate;
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
    [[nodiscard]] bool isBrownIdss() const noexcept
    {
        return operatingModel == OperatingModel::brownIdss;
    }

    [[nodiscard]] float interpolationFor(float rawT) const noexcept
    {
        if (! isBrownIdss())
        {
            const auto smoothT = rawT * rawT * (3.0f - 2.0f * rawT);
            return juce::jmap(interpolationShape, rawT, smoothT);
        }

        // Brown's IDSS exposed linear, cosine and square interpolation choices.
        // Veloria keeps one CURVE control and continuously morphs through those
        // three historically documented IDSS behaviours.
        const auto cosineT = 0.5f - 0.5f * std::cos(juce::MathConstants<float>::pi * rawT);
        const auto squareT = rawT * rawT;
        if (interpolationShape <= 0.5f)
            return juce::jmap(interpolationShape * 2.0f, rawT, cosineT);
        return juce::jmap((interpolationShape - 0.5f) * 2.0f, cosineT, squareT);
    }

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

        if (isBrownIdss())
        {
            amplitudes.front() = 0.0f;
            amplitudes[activeBreakpointCount - 1] = 0.0f;
        }
    }

    void evolveBreakpoint(std::size_t index) noexcept
    {
        // Brown's IDSS used exponential step slider scaling to gain much finer
        // control over gentle stochastic pitch/timbre motion near zero.
        const auto ampWalkControl = isBrownIdss() ? std::pow(amplitudeWalk, 2.15f) : amplitudeWalk;
        const auto timeWalkControl = isBrownIdss() ? std::pow(timeWalk, 2.35f) : timeWalk;
        const auto maxAmpStep = juce::jmap(ampWalkControl, 0.0002f, 0.24f) * amplitudeStepScale;
        const auto maxTimeStep = juce::jmap(timeWalkControl, 0.0002f, 0.20f) * timeStepScale;

        const auto ampRandom = distributedRandom(amplitudeDistribution, amplitudeWalk);
        const auto timeRandom = distributedRandom(timeDistribution, timeWalk);

        if (walkOrder == 1)
        {
            amplitudes[index] = reflect(
                amplitudes[index] + ampRandom * maxAmpStep,
                -amplitudeMirror,
                amplitudeMirror);

            const auto minimumDuration = juce::jmax(0.02f, 1.0f - timeMirror * 0.92f);
            const auto maximumDuration = 1.0f + timeMirror * 2.75f;
            durations[index] = reflect(
                durations[index] + timeRandom * maxTimeStep,
                minimumDuration,
                maximumDuration);
        }
        else
        {
            amplitudeStepState[index] = reflect(
                amplitudeStepState[index] + ampRandom * maxAmpStep * 0.35f,
                -maxAmpStep,
                maxAmpStep);

            timeStepState[index] = reflect(
                timeStepState[index] + timeRandom * maxTimeStep * 0.35f,
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

        if (isBrownIdss())
        {
            // IDSS offered a zero-crossing constraint at the wave-cycle pivot.
            // The two edge points are anchored here to retain that behaviour.
            if (index == 0 || index + 1 == activeBreakpointCount)
                amplitudes[index] = 0.0f;

            // Brown also provided a fixed-segment-length pitch mode. Rather than
            // introduce another hidden switch, PITCH LOCK progressively becomes
            // that stabiliser in IDSS mode while remaining continuous and playable.
            const auto fixedSegmentAmount = std::pow(pitchStability, 5.0f);
            durations[index] = juce::jmap(fixedSegmentAmount, durations[index], 1.0f);
            timeStepState[index] *= (1.0f - fixedSegmentAmount * 0.92f);
        }
    }

    [[nodiscard]] float durationSum() const noexcept
    {
        float total = 0.0f;
        for (std::size_t i = 0; i < activeBreakpointCount; ++i)
            total += durations[i];
        return juce::jmax(0.001f, total);
    }

    [[nodiscard]] float uniformBipolar() noexcept
    {
        return random.nextFloat() * 2.0f - 1.0f;
    }

    [[nodiscard]] float gaussianBipolar() noexcept
    {
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
    std::size_t activeBreakpointCount { numBreakpoints };

    float frequency { 220.0f };
    float amplitudeWalk { 0.12f };
    float timeWalk { 0.08f };
    float amplitudeMirror { 0.88f };
    float timeMirror { 0.45f };
    float amplitudeStepScale { 1.0f };
    float timeStepScale { 1.0f };
    float pitchStability { 1.0f };
    float interpolationShape { 0.0f };
    int walkOrder { 2 };
    Distribution amplitudeDistribution { Distribution::adaptive };
    Distribution timeDistribution { Distribution::adaptive };
    OperatingModel operatingModel { OperatingModel::veloria };
    std::uint32_t seed { 1u };
};
} // namespace veloria::dsp
