/*
  ==============================================================================

    GraphicEQ.h
    Created: 2 Sep 2026 4:23:25pm
    Author:  Maximos Kaliakatsos-Papakostas

  ==============================================================================
*/

#pragma once

#include <array>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cassert>

class GraphicEQ
{
public:
    static constexpr std::size_t NumBands = 31;

    using Frequencies = std::array<float, NumBands>;
    using Gains       = std::array<float, NumBands>;

    // ---------------------------------------------------------------------
    // Construction
    // ---------------------------------------------------------------------

    GraphicEQ() = default;
    
    void prepare(double sampleRate,
                 const Frequencies& frequencies)
    {
        assert(sampleRate > 0.0);

        sampleRate_ = sampleRate;
        frequencies_ = frequencies;

        initialise();

        prepared_ = true;
    }
    
    bool isPrepared() const noexcept
    {
        return prepared_;
    }

    // ---------------------------------------------------------------------
    // Parameters
    // ---------------------------------------------------------------------

    // Safe to call from the UI thread while the audio thread is running.
    void setGain(std::size_t band, float gainDb)
    {
        if (band >= NumBands)
            return;

        targetGain_[band].store(
            clampGain(gainDb),
            std::memory_order_relaxed);
    }

    // Safe to call from the UI thread.
    //
    // The individual values are atomically transferred. Therefore the
    // audio thread may see some old and some new values for one block.
    // This is normally exactly what we want for interactive EQ control.
    void setGains(const Gains& gainsDb)
    {
        for (std::size_t i = 0; i < NumBands; ++i)
        {
            targetGain_[i].store(
                clampGain(gainsDb[i]),
                std::memory_order_relaxed);
        }
    }

    float getGain(std::size_t band) const
    {
        if (band >= NumBands)
            return 0.0f;

        return targetGain_[band].load(
            std::memory_order_relaxed);
    }

    // Q and frequency are normally configured before audio starts.
    //
    // We deliberately don't make Q atomic: changing Q involves changing
    // the filter design rather than merely changing an interactive
    // parameter. If runtime Q changes are required later, we can add the
    // same smoothing/atomic machinery used for gain.
    void setQ(std::size_t band, float q)
    {
        if (band >= NumBands)
            return;

        q_[band] = std::max(q, 0.05f);
    }

    float getQ(std::size_t band) const
    {
        if (band >= NumBands)
            return 0.0f;

        return q_[band];
    }

    float getFrequency(std::size_t band) const
    {
        if (band >= NumBands)
            return 0.0f;

        return frequencies_[band];
    }

    // ---------------------------------------------------------------------
    // Processing
    // ---------------------------------------------------------------------

    // Process one audio block of mono samples.
    //
    // No allocation.
    // No locks.
    // No JUCE.
    //
    
    // Call once at the beginning of an audio block.
    // Updates the smoothed gains and corresponding biquad coefficients.
    void beginBlock(std::size_t numSamples) noexcept
    {
        if (!prepared_ || numSamples == 0)
            return;

        // Block-rate smoothing.
        //
        // smoothingCoeff_ is the per-sample smoothing coefficient.
        // Convert it to the equivalent coefficient for this block.
        const float blockSmoothing =
            1.0f - std::pow(1.0f - smoothingCoeff_,
                            static_cast<float>(numSamples));

        for (std::size_t i = 0; i < NumBands; ++i)
        {
            const float target = targetGain_[i].load(std::memory_order_relaxed);

            currentGain_[i] +=
                (target - currentGain_[i]) * blockSmoothing;

            filters_[i].updateCoefficients(
                frequencies_[i],
                q_[i],
                currentGain_[i],
                sampleRate_);
        }
    }


    // Process one sample through the complete graphic EQ.
    //
    // This contains only the actual per-sample DSP.
    // It does not update parameters or coefficients.
    float processSample(float sample) noexcept
    {
        if (!prepared_)
            return sample;

        for (std::size_t i = 0; i < NumBands; ++i)
            sample = filters_[i].process(sample);

        return sample;
    }


    // Process a complete block.
    //
    // This preserves the original GraphicEQ interface and behavior,
    // while internally using beginBlock() + processSample().
    void process(float* samples, std::size_t numSamples) noexcept
    {
        if (!prepared_ || samples == nullptr || numSamples == 0)
            return;

        beginBlock(numSamples);

        for (std::size_t n = 0; n < numSamples; ++n)
            samples[n] = processSample(samples[n]);
    }
    
    
//    // Coefficients are recalculated once per block rather than once per
//    // sample. Gain smoothing is still continuous over the block.
//    void process(float* samples, std::size_t numSamples)
//    {
//        if (!prepared_ || samples == nullptr || numSamples == 0)
//            return;
//
//        // -------------------------------------------------------------
//        // Update smoothed gains once for this block.
//        // -------------------------------------------------------------
//
//        const float blockSmoothing =
//            1.0f -
//            std::pow(
//                1.0f - smoothingCoeff_,
//                static_cast<float>(numSamples));
//
//        for (std::size_t band = 0; band < NumBands; ++band)
//        {
//            const float target =
//                targetGain_[band].load(
//                    std::memory_order_relaxed);
//
//            currentGain_[band] +=
//                blockSmoothing *
//                (target - currentGain_[band]);
//
//            filters_[band].updateCoefficients(
//                frequencies_[band],
//                q_[band],
//                currentGain_[band],
//                sampleRate_);
//        }
//
//        // -------------------------------------------------------------
//        // Process audio.
//        // -------------------------------------------------------------
//
//        for (std::size_t n = 0; n < numSamples; ++n)
//        {
//            float x = samples[n];
//
//            for (std::size_t band = 0; band < NumBands; ++band)
//                x = filters_[band].process(x);
//
//            samples[n] = x;
//        }
//    }

    // ---------------------------------------------------------------------
    // Reset
    // ---------------------------------------------------------------------

    void reset()
    {
        if (!prepared_)
            return;

        for (std::size_t i = 0; i < NumBands; ++i)
        {
            targetGain_[i].store(
                0.0f,
                std::memory_order_relaxed);

            currentGain_[i] = 0.0f;

            filters_[i].reset();

            filters_[i].updateCoefficients(
                frequencies_[i],
                q_[i],
                0.0f,
                sampleRate_);
        }
    }

private:

    // =====================================================================
    // Biquad
    // =====================================================================

    class Biquad
    {
    public:

        void updateCoefficients(float frequency,
                                float Q,
                                float gainDb,
                                double sampleRate)
        {
            constexpr float pi =
                3.14159265358979323846f;

            // RBJ peaking-EQ cookbook.
            const float A =
                std::pow(10.0f, gainDb * 0.025f);

            const float omega =
                2.0f * pi *
                frequency /
                static_cast<float>(sampleRate);

            const float sinOmega = std::sin(omega);
            const float cosOmega = std::cos(omega);

            const float alpha =
                sinOmega / (2.0f * Q);

            const float b0 =
                1.0f + alpha * A;

            const float b1 =
                -2.0f * cosOmega;

            const float b2 =
                1.0f - alpha * A;

            const float a0 =
                1.0f + alpha / A;

            const float a1 =
                -2.0f * cosOmega;

            const float a2 =
                1.0f - alpha / A;

            const float invA0 = 1.0f / a0;

            b0_ = b0 * invA0;
            b1_ = b1 * invA0;
            b2_ = b2 * invA0;

            a1_ = a1 * invA0;
            a2_ = a2 * invA0;
        }

        float process(float x)
        {
            // Direct Form II Transposed.
            //
            // This form is compact and has good numerical behavior.

            const float y =
                b0_ * x + z1_;

            z1_ =
                b1_ * x
                - a1_ * y
                + z2_;

            z2_ =
                b2_ * x
                - a2_ * y;

            return y;
        }

        void reset()
        {
            z1_ = 0.0f;
            z2_ = 0.0f;
        }

    private:

        float b0_ = 1.0f;
        float b1_ = 0.0f;
        float b2_ = 0.0f;

        float a1_ = 0.0f;
        float a2_ = 0.0f;

        float z1_ = 0.0f;
        float z2_ = 0.0f;
    };

    // =====================================================================
    // Initialization
    // =====================================================================
    
    void initialise()
    {
        validateFrequencies();

        calculateDefaultQs();

        for (std::size_t i = 0; i < NumBands; ++i)
        {
            targetGain_[i].store(
                0.0f,
                std::memory_order_relaxed);

            currentGain_[i] = 0.0f;

            filters_[i].reset();

            filters_[i].updateCoefficients(
                frequencies_[i],
                q_[i],
                0.0f,
                sampleRate_);
        }

        // 10 ms exponential smoothing.
        constexpr float smoothingTimeSeconds = 0.010f;

        smoothingCoeff_ =
            1.0f -
            std::exp(
                -1.0f /
                (static_cast<float>(sampleRate_) *
                 smoothingTimeSeconds));
    }
    

//    void initialise()
//    {
//        validateFrequencies();
//
//        calculateDefaultQs();
//
//        for (std::size_t i = 0; i < NumBands; ++i)
//        {
//            targetGain_[i].store(
//                0.0f,
//                std::memory_order_relaxed);
//
//            currentGain_[i] = 0.0f;
//
//            filters_[i].updateCoefficients(
//                frequencies_[i],
//                q_[i],
//                0.0f,
//                sampleRate_);
//        }
//
//        // 10 ms exponential smoothing.
//        //
//        // After approximately 10 ms, the parameter has moved substantially
//        // toward its target while remaining completely continuous.
//
//        constexpr float smoothingTimeSeconds = 0.010f;
//
//        smoothingCoeff_ =
//            1.0f -
//            std::exp(
//                -1.0f /
//                (static_cast<float>(sampleRate_) *
//                 smoothingTimeSeconds));
//    }

    // =====================================================================
    // Block parameter update
    // =====================================================================

//    void updateParameters()
//    {
//        for (std::size_t band = 0; band < NumBands; ++band)
//        {
//            const float target =
//                targetGain_[band].load(
//                    std::memory_order_relaxed);
//
//            // Move the smoothed parameter forward by one block.
//            //
//            // Instead of doing 64 individual smoothing operations here,
//            // calculate the value reached after numSamples samples.
//            //
//            // This makes the coefficient update block-based while
//            // preserving the same exponential smoothing behavior.
//
//            // The actual block length is supplied separately below.
//        }
//    }

    // =====================================================================
    // Frequency / Q
    // =====================================================================

    void calculateDefaultQs()
    {
        for (std::size_t i = 0; i < NumBands; ++i)
        {
            float lowerFrequency;
            float upperFrequency;

            if (i == 0)
            {
                const float ratio =
                    frequencies_[1] /
                    frequencies_[0];

                lowerFrequency =
                    frequencies_[0] / ratio;

                upperFrequency =
                    frequencies_[1];
            }
            else if (i == NumBands - 1)
            {
                const float ratio =
                    frequencies_[NumBands - 1] /
                    frequencies_[NumBands - 2];

                lowerFrequency =
                    frequencies_[NumBands - 2];

                upperFrequency =
                    frequencies_[NumBands - 1] * ratio;
            }
            else
            {
                lowerFrequency =
                    frequencies_[i - 1];

                upperFrequency =
                    frequencies_[i + 1];
            }

            // Approximate bandwidth in octaves.
            const float bandwidthOctaves =
                std::log2(
                    upperFrequency /
                    lowerFrequency);

            const float bandwidthRatio =
                std::pow(
                    2.0f,
                    bandwidthOctaves);

            float q =
                std::sqrt(bandwidthRatio) /
                (bandwidthRatio - 1.0f);

            q_[i] =
                std::max(q, 0.05f);
        }
    }

    void validateFrequencies()
    {
        for (std::size_t i = 1; i < NumBands; ++i)
        {
            assert(
                frequencies_[i] >
                frequencies_[i - 1]);
        }

        for (std::size_t i = 0; i < NumBands; ++i)
        {
            assert(frequencies_[i] > 0.0f);
            assert(frequencies_[i] <
                   static_cast<float>(sampleRate_) * 0.5f);
        }
    }

    static float clampGain(float gainDb)
    {
        constexpr float minGain = -24.0f;
        constexpr float maxGain =  24.0f;

        return std::clamp(
            gainDb,
            minGain,
            maxGain);
    }

private:

    double sampleRate_ = 0.0;

    Frequencies frequencies_{};

    std::array<float, NumBands> q_{};

    std::array<std::atomic<float>, NumBands> targetGain_;

    std::array<float, NumBands> currentGain_{};

    std::array<Biquad, NumBands> filters_{};

    float smoothingCoeff_ = 0.0f;
    
    bool prepared_ = false;
};
