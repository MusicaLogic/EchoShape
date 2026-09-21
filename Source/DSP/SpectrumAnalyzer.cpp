/*
  ==============================================================================

    SpectrumAnalyzer.cpp
    Created: 12 Sep 2026 6:33:01am
    Author:  Maximos Kaliakatsos-Papakostas

  ==============================================================================
*/

/*
  ==============================================================================

    SpectrumAnalyzer.cpp

  ==============================================================================
*/

#include "SpectrumAnalyzer.h"

#include <algorithm>
#include <cmath>


//==============================================================================
// Construction
//==============================================================================

SpectrumAnalyzer::SpectrumAnalyzer(
    double initialSampleRate)
    : fft(fftOrder),
      window(
          fftSize,
          juce::dsp::WindowingFunction<float>::hann,
          true),
      sampleRate(initialSampleRate)
{
    calculateBands();
    updateSmoothingCoefficients();
    reset();
}


//==============================================================================
// Setup
//==============================================================================

void SpectrumAnalyzer::setSampleRate(
    double newSampleRate)
{
    if (newSampleRate <= 0.0)
        return;

    sampleRate = newSampleRate;

    calculateBands();
    updateSmoothingCoefficients();
    reset();
}


void SpectrumAnalyzer::reset()
{
    sampleBuffer.fill(0.0f);
    fftData.fill(0.0f);

    targetSpectrum.fill(minDB);
    smoothedSpectrum.fill(minDB);

    sampleBufferPosition = 0;
    samplesUntilFFT = hopSize;
}


//==============================================================================
// Audio processing
//==============================================================================

void SpectrumAnalyzer::processBlock(
    const float* samples,
    int numSamples) noexcept
{
    if (samples == nullptr || numSamples <= 0)
        return;


    for (int i = 0; i < numSamples; ++i)
    {
        sampleBuffer[
            static_cast<size_t>(sampleBufferPosition)] =
            samples[i];

        sampleBufferPosition =
            (sampleBufferPosition + 1) % fftSize;

        --samplesUntilFFT;

        if (samplesUntilFFT <= 0)
        {
            performFFT();

            samplesUntilFFT = hopSize;
        }
    }
}


//==============================================================================
// FFT
//==============================================================================

void SpectrumAnalyzer::performFFT() noexcept
{
    // ================================================================
    // Copy the circular buffer into chronological order.
    // ================================================================

    for (int i = 0; i < fftSize; ++i)
    {
        const int index =
            (sampleBufferPosition + i) % fftSize;

        fftData[static_cast<size_t>(i)] =
            sampleBuffer[static_cast<size_t>(index)];
    }


    // The second half must be zeroed because the FFT function
    // operates on a 2N-sized buffer.
    std::fill(
        fftData.begin() + fftSize,
        fftData.end(),
        0.0f);


    // ================================================================
    // Window
    // ================================================================

    window.multiplyWithWindowingTable(
        fftData.data(),
        fftSize);


    // ================================================================
    // FFT
    // ================================================================

    fft.performFrequencyOnlyForwardTransform(
        fftData.data());


    // ================================================================
    // Convert FFT result into our 64 display bands.
    // ================================================================

    calculateSpectrum();
}


//==============================================================================
// Spectrum calculation
//==============================================================================

void SpectrumAnalyzer::calculateSpectrum() noexcept
{
    // ================================================================
    // Convert the FFT magnitude spectrum into dB.
    //
    // fftData contains positive-frequency FFT magnitudes.
    // ================================================================

    constexpr float epsilon = 1.0e-10f;


    for (int bandIndex = 0;
         bandIndex < numSpectrumBands;
         ++bandIndex)
    {
        const Band& band =
            bands[static_cast<size_t>(bandIndex)];


        // ------------------------------------------------------------
        // Continuous FFT-bin position corresponding to the band's
        // logarithmic centre frequency.
        // ------------------------------------------------------------

        const float position =
            juce::jlimit(
                0.0f,
                static_cast<float>(fftSize / 2 - 1),
                band.fftPosition);


        const int lowerBin =
            static_cast<int>(
                std::floor(position));

        const int upperBin =
            std::min(
                lowerBin + 1,
                fftSize / 2 - 1);


        const float fraction =
            position -
            static_cast<float>(lowerBin);


        // ------------------------------------------------------------
        // FFT magnitude interpolation.
        // ------------------------------------------------------------

        const float lowerMagnitude =
            fftData[
                static_cast<size_t>(lowerBin)];


        const float upperMagnitude =
            fftData[
                static_cast<size_t>(upperBin)];


        const float magnitude =
            lowerMagnitude +
            fraction *
            (upperMagnitude - lowerMagnitude);


        // ------------------------------------------------------------
        // Normalize FFT magnitude.
        // ------------------------------------------------------------

        float normalizedMagnitude =
            magnitude /
            static_cast<float>(fftSize);


        // Account for the discarded negative-frequency half.
        if (lowerBin != 0)
            normalizedMagnitude *= 2.0f;


        // ------------------------------------------------------------
        // Convert to dB.
        // ------------------------------------------------------------

        const float levelDB =
            20.0f *
            std::log10(
                std::max(
                    normalizedMagnitude,
                    epsilon));


        targetSpectrum[
            static_cast<size_t>(bandIndex)] =
            juce::jlimit(
                minDB,
                maxDB,
                levelDB);
    }


    // ================================================================
    // Attack / release smoothing
    // ================================================================

    for (int i = 0;
         i < numSpectrumBands;
         ++i)
    {
        const size_t index =
            static_cast<size_t>(i);

        const float target =
            targetSpectrum[index];

        float& current =
            smoothedSpectrum[index];


        if (target > current)
        {
            // Fast attack.
            current =
                target +
                attackCoefficient *
                (current - target);
        }
        else
        {
            // Slow release.
            current =
                target +
                releaseCoefficient *
                (current - target);
        }
    }
}

//==============================================================================
// Frequency bands
//==============================================================================

void SpectrumAnalyzer::calculateBands()
{
    const double minLog =
        std::log10(
            static_cast<double>(minFrequency));

    const double maxLog =
        std::log10(
            static_cast<double>(maxFrequency));


    for (int i = 0;
         i < numSpectrumBands;
         ++i)
    {
        const double t0 =
            static_cast<double>(i) /
            static_cast<double>(numSpectrumBands);

        const double t1 =
            static_cast<double>(i + 1) /
            static_cast<double>(numSpectrumBands);

        const double tc =
            (t0 + t1) * 0.5;


        const float low =
            static_cast<float>(
                std::pow(
                    10.0,
                    minLog +
                    t0 * (maxLog - minLog)));


        const float high =
            static_cast<float>(
                std::pow(
                    10.0,
                    minLog +
                    t1 * (maxLog - minLog)));


        const float centre =
            static_cast<float>(
                std::pow(
                    10.0,
                    minLog +
                    tc * (maxLog - minLog)));


        const float fftPosition =
            frequencyToFFTBin(
                centre,
                sampleRate);


        bands[static_cast<size_t>(i)] =
        {
            low,
            high,
            centre,
            fftPosition
        };
    }
}


//==============================================================================
// Smoothing
//==============================================================================

void SpectrumAnalyzer::updateSmoothingCoefficients()
{
    // The spectrum is updated every hopSize samples.
    //
    // Convert the desired attack/release time into a coefficient
    // suitable for exponential smoothing.

    const float updateInterval =
        static_cast<float>(
            hopSize / sampleRate);


    attackCoefficient =
        std::exp(
            -updateInterval /
            attackTimeSeconds);


    releaseCoefficient =
        std::exp(
            -updateInterval /
            releaseTimeSeconds);
}


//==============================================================================
// Results
//==============================================================================

SpectrumAnalyzer::Spectrum
SpectrumAnalyzer::getSpectrum() const noexcept
{
    return smoothedSpectrum;
}


//==============================================================================
// Frequency information
//==============================================================================

float SpectrumAnalyzer::getBandCentreFrequency(
    int band) noexcept
{
    if (band < 0 || band >= numSpectrumBands)
        return minFrequency;

    const double minLog =
        std::log10(
            static_cast<double>(minFrequency));

    const double maxLog =
        std::log10(
            static_cast<double>(maxFrequency));

    const double t =
        (static_cast<double>(band) + 0.5) /
        static_cast<double>(numSpectrumBands);

    return static_cast<float>(
        std::pow(
            10.0,
            minLog + t * (maxLog - minLog)));
}


float SpectrumAnalyzer::getBandLowFrequency(
    int band) noexcept
{
    if (band < 0 || band >= numSpectrumBands)
        return minFrequency;

    const double minLog =
        std::log10(
            static_cast<double>(minFrequency));

    const double maxLog =
        std::log10(
            static_cast<double>(maxFrequency));

    const double t =
        static_cast<double>(band) /
        static_cast<double>(numSpectrumBands);

    return static_cast<float>(
        std::pow(
            10.0,
            minLog + t * (maxLog - minLog)));
}


float SpectrumAnalyzer::getBandHighFrequency(
    int band) noexcept
{
    if (band < 0 || band >= numSpectrumBands)
        return maxFrequency;

    const double minLog =
        std::log10(
            static_cast<double>(minFrequency));

    const double maxLog =
        std::log10(
            static_cast<double>(maxFrequency));

    const double t =
        static_cast<double>(band + 1) /
        static_cast<double>(numSpectrumBands);

    return static_cast<float>(
        std::pow(
            10.0,
            minLog + t * (maxLog - minLog)));
}


//==============================================================================
// Helpers
//==============================================================================

float SpectrumAnalyzer::frequencyToFFTBin(
    float frequency,
    double sampleRate) noexcept
{
    return frequency *
           static_cast<float>(fftSize) /
           static_cast<float>(sampleRate);
}

// debug
void SpectrumAnalyzer::debugPrintBands() const
{
    DBG("==========================================");
    DBG("Spectrum bands");

    for (int i = 0; i < numSpectrumBands; ++i)
    {
        const auto& band =
            bands[static_cast<size_t>(i)];

        DBG(
            juce::String(i)
            + ": "
            + juce::String(band.lowFrequency, 1)
            + " - "
            + juce::String(band.highFrequency, 1)
            + " Hz | position "
            + juce::String(band.fftPosition));
    }
}
