/*
  ==============================================================================

    SpectrumAnalyzer.h
    Created: 12 Sep 2026 6:33:01am
    Author:  Maximos Kaliakatsos-Papakostas

  ==============================================================================
*/

/*
  ==============================================================================

    SpectrumAnalyzer.h

    Lightweight real-time spectrum analyzer for ParaGraphiQ.

    - 1024-point FFT
    - 50% overlap
    - 64 logarithmically spaced display bands
    - 20 Hz - 20 kHz
    - output in dBFS
    - attack/release smoothing

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <array>
#include <cmath>

#include "SpectrumDataBuffer.h"


class SpectrumAnalyzer
{
public:

    // ================================================================
    // Configuration
    // ================================================================

    static constexpr int fftOrder = 12;
    static constexpr int fftSize = 1 << fftOrder;

    static constexpr int hopSize = fftSize / 2;

    static constexpr int numSpectrumBands = 32;

    static constexpr float minFrequency = 20.0f;
    static constexpr float maxFrequency = 20000.0f;

    static constexpr float minDB = -80.0f;
    static constexpr float maxDB = 0.0f;


    using Spectrum =
        std::array<float, numSpectrumBands>;


    // ================================================================
    // Construction
    // ================================================================

    explicit SpectrumAnalyzer(
        double sampleRate = 44100.0);


    // ================================================================
    // Setup
    // ================================================================

    void setSampleRate(double newSampleRate);

    void reset();


    // ================================================================
    // Audio processing
    // ================================================================

    // Called from the audio thread.
    //
    // The samples are assumed to be mono.
    //
    // No memory allocation is performed here.
    void processBlock(
        const float* samples,
        int numSamples) noexcept;


    // ================================================================
    // Results
    // ================================================================

    Spectrum getSpectrum() const noexcept;


    // ================================================================
    // Information
    // ================================================================

    static float getBandCentreFrequency(
        int band) noexcept;

    static float getBandLowFrequency(
        int band) noexcept;

    static float getBandHighFrequency(
        int band) noexcept;

    // debug
    void debugPrintBands() const;

private:

    // ================================================================
    // FFT
    // ================================================================

    juce::dsp::FFT fft;

    juce::dsp::WindowingFunction<float> window;


    // ================================================================
    // Sample rate
    // ================================================================

    double sampleRate;


    // ================================================================
    // FFT accumulation
    // ================================================================

    std::array<float, fftSize> sampleBuffer {};

    int sampleBufferPosition = 0;

    int samplesUntilFFT = hopSize;


    // ================================================================
    // FFT working data
    //
    // JUCE's frequency-only FFT function requires fftSize * 2
    // floating-point values.
    // ================================================================

    std::array<float, fftSize * 2> fftData {};


    // ================================================================
    // Frequency bands
    // ================================================================

    struct Band
    {
        float lowFrequency;
        float highFrequency;
        float centreFrequency;

        float fftPosition;
    };

    std::array<Band, numSpectrumBands> bands {};


    void calculateBands();


    // ================================================================
    // Spectrum
    // ================================================================

    Spectrum targetSpectrum {};

    Spectrum smoothedSpectrum {};


    // ================================================================
    // Smoothing
    // ================================================================

    static constexpr float attackTimeSeconds = 0.1f;
    static constexpr float releaseTimeSeconds = 0.5f;

    float attackCoefficient = 0.0f;
    float releaseCoefficient = 0.0f;


    void updateSmoothingCoefficients();


    // ================================================================
    // FFT
    // ================================================================

    void performFFT() noexcept;

    void calculateSpectrum() noexcept;


    // ================================================================
    // Helpers
    // ================================================================

    static float frequencyToFFTBin(
        float frequency,
        double sampleRate) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        SpectrumAnalyzer)
};
