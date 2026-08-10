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

    void setAmplitudeDistribution(Distribution value) noexcept
    {
        amplitudeDistributionPosition = static_cast<float>(static_cast<int>(value));
    }

    void setTimeDistribution(Distribution value) noexcept
    {
        timeDistributionPosition = static_cast<float>(static_cast<int>(value));
    }

    // Continuous 0..5 morph across the six genuine stochastic distributions.
    void setAmplitudeDistributionMorph(float value) noexcept
    {
        amplitudeDistributionPosition = juce::jlimit(0.0f, 5.0f, value);
    }

    void setTimeDistributionMorph(float value) noexcept
    {
        timeDistributionPosition = juce::jlimit(0.0f, 5.0f, value);
    }

    void setAmplitudeStepScale(float value) noexcept { amplitudeStepScale = juce::jlimit(0.10f, 2.0f, value); }
    void setTimeStepScale(float value) noexcept { timeStepScale = juce::jlimit(0.10f, 2.0f, value); }
    void setBoundaryDrive(float value) noexcept { boundaryDrive = juce::jlimit(0.0f, 1.0f, value); }
    void setStochasticRate(float value) noexcept { stochasticRate = juce::jlimit(0.0f, 1.0f, value); }
    void setJump(float value) noexcept { jump = juce::jlimit(0.0f, 1.0f, value); }
    void setCorrelation(float value) noexcept { correlation = juce::jlimit(0.0f, 1.0f, value); }
    void setWalkOrder(int value) noexcept { walkOrder = juce::jlimit(1, 2, value); }
    void setOperatingModel(OperatingModel value) noexcept { operatingModel = value; }

    // POINTS is continuous to the performer. Between N and N+1, the new final
    // breakpoint grows smoothly out of the closing segment rather than appearing
    // suddenly. At integer values the topology is exactly the historical N-point field.
    void setActiveBreakpointCount(float value) noexcept
    {
        const auto position = juce::jlimit(4.0f, static_cast<float>(numBreakpoints), value);
        const auto lower = juce::jlimit(4, static_cast<int>(numBreakpoints), static_cast<int>(std::floor(position)));
        const auto fraction = position - static_cast<float>(lower);

        if (fraction <= 1.0e-5f || lower >= static_cast<int>(numBreakpoints))
        {
            activeBreakpointCount = static_cast<std::size_t>(lower);
            breakpointMorph = 1.0f;
        }
        else
        {
            activeBreakpointCount = static_cast<std::size_t>(lower + 1);
            breakpointMorph = juce::jlimit(0.0f, 1.0f, fraction);
        }

        if (segmentIndex >= activeBreakpointCount)
        {
            segmentIndex = 0;
            segmentPhase = 0.0;
        }

        if (isBrownIdss())
        {
            amplitudes.front() = 0.0f;
            amplitudes[activeBreakpointCount - 1] = 0.0f;
        }
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

        if (activeBreakpointCount > 0 && breakpointMorph < 1.0f)
        {
            const auto last = activeBreakpointCount - 1;
            amplitudeOut[last] = amplitudeForIndex(last);
            durationOut[last] = durationForIndex(last);
        }
    }

    [[nodiscard]] float processSample() noexcept
    {
        const auto count = juce::jmax(static_cast<std::size_t>(4), activeBreakpointCount);
        if (segmentIndex >= count)
            segmentIndex = 0;

        const auto nextIndex = (segmentIndex + 1) % count;
        const auto rawT = juce::jlimit(0.0f, 1.0f, static_cast<float>(segmentPhase));
        const auto t = interpolationFor(rawT);
        const auto currentAmplitude = amplitudeForIndex(segmentIndex);
        const auto nextAmplitude = amplitudeForIndex(nextIndex);
        const auto output = currentAmplitude + (nextAmplitude - currentAmplitude) * t;

        const auto totalDuration = durationSum();
        const auto driftRatio = static_cast<float>(count) / juce::jmax(0.001f, totalDuration);
        const auto pitchRatio = juce::jmap(pitchStability, driftRatio, 1.0f);
        auto remainingCycleTime = static_cast<double>(frequency * pitchRatio) / sampleRate;

        constexpr int maxSegmentCrossingsPerSample = 64;
        for (int crossing = 0;
             crossing < maxSegmentCrossingsPerSample && remainingCycleTime > 0.0;
             ++crossing)
        {
            const auto refreshedTotal = durationSum();
            const auto segmentDuration = juce::jmax(
                1.0e-8,
                static_cast<double>(durationForIndex(segmentIndex))
                    / juce::jmax(0.001, static_cast<double>(refreshedTotal)));
            const auto remainingInSegment = juce::jmax(
                0.0,
                (1.0 - segmentPhase) * segmentDuration);

            if (remainingCycleTime < remainingInSegment)
            {
                segmentPhase += remainingCycleTime / segmentDuration;
                remainingCycleTime = 0.0;
                break;
            }

            remainingCycleTime -= remainingInSegment;
            segmentPhase = 0.0;

            if (segmentIndex + 1 >= count)
            {
                segmentIndex = 0;
                evolveFieldRateControlled();
            }
            else
            {
                ++segmentIndex;
            }
        }

        if (! std::isfinite(segmentPhase) || remainingCycleTime > 0.0)
            segmentPhase = 0.0;

        return std::isfinite(output) ? output * 0.82f : 0.0f;
    }

private:
    [[nodiscard]] bool isBrownIdss() const noexcept
    {
        return operatingModel == OperatingModel::brownIdss;
    }

    [[nodiscard]] static Distribution distributionForIndex(int index) noexcept
    {
        switch (juce::jlimit(0, 5, index))
        {
            case 1: return Distribution::uniform;
            case 2: return Distribution::gaussian;
            case 3: return Distribution::logistic;
            case 4: return Distribution::cauchy;
            case 5: return Distribution::arcsine;
            case 0:
            default: return Distribution::adaptive;
        }
    }

    [[nodiscard]] float interpolationFor(float rawT) const noexcept
    {
        if (! isBrownIdss())
        {
            const auto smoothT = rawT * rawT * (3.0f - 2.0f * rawT);
            return juce::jmap(interpolationShape, rawT, smoothT);
        }

        const auto cosineT = 0.5f - 0.5f * std::cos(juce::MathConstants<float>::pi * rawT);
        const auto squareT = rawT * rawT;
        if (interpolationShape <= 0.5f)
            return juce::jmap(interpolationShape * 2.0f, rawT, cosineT);
        return juce::jmap((interpolationShape - 0.5f) * 2.0f, cosineT, squareT);
    }

    [[nodiscard]] float amplitudeForIndex(std::size_t index) const noexcept
    {
        if (activeBreakpointCount > 0
            && breakpointMorph < 1.0f
            && index + 1 == activeBreakpointCount)
        {
            // At the start of a POINTS transition the new point lies exactly on
            // the closing target (point zero), then acquires its own stochastic
            // amplitude continuously as the knob approaches the next integer.
            return juce::jmap(breakpointMorph, amplitudes.front(), amplitudes[index]);
        }
        return amplitudes[index];
    }

    [[nodiscard]] float durationForIndex(std::size_t index) const noexcept
    {
        const auto raw = std::isfinite(durations[index]) ? durations[index] : 1.0f;
        if (activeBreakpointCount > 0
            && breakpointMorph < 1.0f
            && index + 1 == activeBreakpointCount)
        {
            // The extra closing segment grows from virtually zero duration to its
            // full stochastic duration, avoiding the hard topology jump N -> N+1.
            return juce::jmax(1.0e-5f, raw * breakpointMorph);
        }
        return raw;
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
            amplitudeCascadeState[i] = 0.5f;
            timeCascadeState[i] = 0.5f;
            amplitudeRandomMemory[i] = 0.0f;
            timeRandomMemory[i] = 0.0f;
        }

        if (isBrownIdss())
        {
            amplitudes.front() = 0.0f;
            amplitudes[activeBreakpointCount - 1] = 0.0f;
        }
    }

    void evolveFieldRateControlled() noexcept
    {
        const auto rateCurve = stochasticRate * stochasticRate;
        const auto passesExact = 1.0f + rateCurve * 7.0f;
        const auto wholePasses = juce::jlimit(1, 8, static_cast<int>(std::floor(passesExact)));
        for (int pass = 0; pass < wholePasses; ++pass)
            evolveField();
        if (wholePasses < 8 && random.nextFloat() < passesExact - static_cast<float>(wholePasses))
            evolveField();
    }

    [[nodiscard]] float shapeStochasticRandom(float fresh, float& memory) noexcept
    {
        const auto rho = juce::jlimit(0.0f, 0.985f, correlation * 0.985f);
        const auto freshGain = std::sqrt(juce::jmax(0.0f, 1.0f - rho * rho));
        auto value = rho * memory + freshGain * fresh;
        memory = value;
        const auto jumpProbability = jump * jump * 0.28f;
        if (jumpProbability > 0.0f && random.nextFloat() < jumpProbability)
        {
            const auto sign = random.nextBool() ? 1.0f : -1.0f;
            value += sign * (0.75f + jump * 3.25f);
        }
        return value;
    }

    void evolveField() noexcept
    {
        for (std::size_t i = 0; i < activeBreakpointCount; ++i)
            evolveBreakpoint(i);

        if (isBrownIdss())
        {
            amplitudes.front() = 0.0f;
            amplitudes[activeBreakpointCount - 1] = 0.0f;
        }
    }

    void evolveBreakpoint(std::size_t index) noexcept
    {
        const auto ampWalkControl = isBrownIdss() ? std::pow(amplitudeWalk, 2.15f) : amplitudeWalk;
        const auto timeWalkControl = isBrownIdss() ? std::pow(timeWalk, 2.35f) : timeWalk;
        const auto maxAmpStep = juce::jmap(ampWalkControl, 0.0002f, 0.24f) * amplitudeStepScale;
        const auto maxTimeStep = juce::jmap(timeWalkControl, 0.0002f, 0.20f) * timeStepScale;

        const auto minimumDuration = juce::jmax(0.02f, 1.0f - timeMirror * 0.92f);
        const auto maximumDuration = 1.0f + timeMirror * 2.75f;
        const auto ampFresh = distributedRandomMorph(amplitudeDistributionPosition, amplitudeWalk);
        const auto timeFresh = distributedRandomMorph(timeDistributionPosition, timeWalk);
        const auto ampRandom = shapeStochasticRandom(ampFresh, amplitudeRandomMemory[index]);
        const auto timeRandom = shapeStochasticRandom(timeFresh, timeRandomMemory[index]);
        const auto ampProximity = juce::jlimit(0.0f, 1.0f, std::abs(amplitudes[index]) / juce::jmax(0.05f, amplitudeMirror));
        const auto timeCentre = 0.5f * (minimumDuration + maximumDuration);
        const auto timeHalfSpan = juce::jmax(0.001f, 0.5f * (maximumDuration - minimumDuration));
        const auto timeProximity = juce::jlimit(0.0f, 1.0f, std::abs(durations[index] - timeCentre) / timeHalfSpan);
        const auto boundaryCurve = boundaryDrive * boundaryDrive;
        const auto ampBoundaryBoost = 1.0f + boundaryCurve * 6.0f * ampProximity * ampProximity * ampProximity;
        const auto timeBoundaryBoost = 1.0f + boundaryCurve * 6.0f * timeProximity * timeProximity * timeProximity;

        if (walkOrder == 1)
        {
            amplitudes[index] = reflect(
                amplitudes[index] + ampRandom * maxAmpStep * ampBoundaryBoost,
                -amplitudeMirror,
                amplitudeMirror);

            durations[index] = reflect(
                durations[index] + timeRandom * maxTimeStep * timeBoundaryBoost,
                minimumDuration,
                maximumDuration);
        }
        else if (isBrownIdss())
        {
            const auto ampRangeRandom = distributedRandomMorph(amplitudeDistributionPosition, amplitudeWalk);
            const auto timeRangeRandom = distributedRandomMorph(timeDistributionPosition, timeWalk);

            amplitudeCascadeState[index] = reflect(
                amplitudeCascadeState[index] + ampRangeRandom * (0.035f + ampWalkControl * 0.20f),
                0.05f,
                1.0f);
            timeCascadeState[index] = reflect(
                timeCascadeState[index] + timeRangeRandom * (0.035f + timeWalkControl * 0.20f),
                0.05f,
                1.0f);

            const auto localAmpStep = maxAmpStep * amplitudeCascadeState[index];
            const auto localTimeStep = maxTimeStep * timeCascadeState[index];

            amplitudeStepState[index] = reflect(
                amplitudeStepState[index] + ampRandom * localAmpStep * 0.35f * ampBoundaryBoost,
                -localAmpStep,
                localAmpStep);
            timeStepState[index] = reflect(
                timeStepState[index] + timeRandom * localTimeStep * 0.35f * timeBoundaryBoost,
                -localTimeStep,
                localTimeStep);

            amplitudes[index] = reflect(
                amplitudes[index] + amplitudeStepState[index],
                -amplitudeMirror,
                amplitudeMirror);
            durations[index] = reflect(
                durations[index] + timeStepState[index],
                minimumDuration,
                maximumDuration);
        }
        else
        {
            amplitudeStepState[index] = reflect(
                amplitudeStepState[index] + ampRandom * maxAmpStep * 0.35f * ampBoundaryBoost,
                -maxAmpStep,
                maxAmpStep);

            timeStepState[index] = reflect(
                timeStepState[index] + timeRandom * maxTimeStep * 0.35f * timeBoundaryBoost,
                -maxTimeStep,
                maxTimeStep);

            amplitudes[index] = reflect(
                amplitudes[index] + amplitudeStepState[index],
                -amplitudeMirror,
                amplitudeMirror);

            durations[index] = reflect(
                durations[index] + timeStepState[index],
                minimumDuration,
                maximumDuration);
        }

        if (isBrownIdss())
        {
            if (index == 0 || index + 1 == activeBreakpointCount)
                amplitudes[index] = 0.0f;

            const auto fixedSegmentAmount = std::pow(pitchStability, 5.0f);
            const auto fixedSegmentTarget = juce::jlimit(
                minimumDuration,
                maximumDuration,
                juce::jmap(timeMirror, 0.05f, 1.0f, minimumDuration, maximumDuration));
            durations[index] = juce::jmap(fixedSegmentAmount, durations[index], fixedSegmentTarget);
            timeStepState[index] *= (1.0f - fixedSegmentAmount * 0.92f);
            timeCascadeState[index] = juce::jmap(fixedSegmentAmount * 0.85f,
                                                  timeCascadeState[index], 0.5f);
        }
    }

    [[nodiscard]] float durationSum() const noexcept
    {
        float total = 0.0f;
        for (std::size_t i = 0; i < activeBreakpointCount; ++i)
            total += durationForIndex(i);
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

    [[nodiscard]] float distributedRandomMorph(float position, float walkEnergy) noexcept
    {
        const auto p = juce::jlimit(0.0f, 5.0f, position);
        const auto lowerIndex = juce::jlimit(0, 5, static_cast<int>(std::floor(p)));
        const auto upperIndex = juce::jmin(5, lowerIndex + 1);
        const auto blend = p - static_cast<float>(lowerIndex);

        const auto a = distributedRandom(distributionForIndex(lowerIndex), walkEnergy);
        if (upperIndex == lowerIndex || blend <= 1.0e-6f)
            return a;

        const auto b = distributedRandom(distributionForIndex(upperIndex), walkEnergy);
        return a + (b - a) * blend;
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
        if (! std::isfinite(value) || ! std::isfinite(minimum) || ! std::isfinite(maximum))
            return 0.5f * (minimum + maximum);
        if (maximum <= minimum)
            return minimum;

        const auto span = maximum - minimum;
        const auto period = span * 2.0f;
        auto folded = std::fmod(value - minimum, period);
        if (folded < 0.0f)
            folded += period;
        if (folded > span)
            folded = period - folded;
        return minimum + folded;
    }

    std::array<float, numBreakpoints> amplitudes {};
    std::array<float, numBreakpoints> durations {};
    std::array<float, numBreakpoints> amplitudeStepState {};
    std::array<float, numBreakpoints> timeStepState {};
    std::array<float, numBreakpoints> amplitudeCascadeState {};
    std::array<float, numBreakpoints> timeCascadeState {};
    std::array<float, numBreakpoints> amplitudeRandomMemory {};
    std::array<float, numBreakpoints> timeRandomMemory {};

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
    float boundaryDrive { 0.0f };
    float stochasticRate { 0.0f };
    float jump { 0.0f };
    float correlation { 0.0f };
    float pitchStability { 1.0f };
    float interpolationShape { 0.0f };
    float amplitudeDistributionPosition { 0.0f };
    float timeDistributionPosition { 0.0f };
    float breakpointMorph { 1.0f };
    int walkOrder { 2 };
    OperatingModel operatingModel { OperatingModel::veloria };
    std::uint32_t seed { 1u };
};
} // namespace veloria::dsp
