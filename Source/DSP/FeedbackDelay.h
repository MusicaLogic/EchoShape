/*
  ==============================================================================

    FeedbackDelay.h
    Created: 22 Sep 2026 6:41:00am
    Author:  Maximos Kaliakatsos-Papakostas

  ==============================================================================
*/

#pragma once

#include "GraphicEQ.h"

#include <atomic>
#include <cstddef>
#include <vector>

class FeedbackDelay
{
public:
    FeedbackDelay() = default;
    ~FeedbackDelay() = default;

    //--------------------------------------------------------------------------
    // Preparation
    //--------------------------------------------------------------------------

    void prepare(double sampleRate,
                 const GraphicEQ::Frequencies& frequencies,
                 float maxDelaySeconds = 2.0f);

    void reset();

    //--------------------------------------------------------------------------
    // Parameters
    //
    // These functions are safe to call from the UI / processor thread while
    // audio processing is taking place.
    //--------------------------------------------------------------------------

    void setDelayTime(float seconds) noexcept;
    void setFeedback(float feedback) noexcept;
    void setWet(float wet) noexcept;

    void setEQGains(const GraphicEQ::Gains& gains) noexcept;
    void setEQGain(std::size_t band, float gainDb) noexcept;
    float getEQGain(std::size_t band) const;

    //--------------------------------------------------------------------------
    // Processing
    //--------------------------------------------------------------------------

    // Must be called once before processSample() when processing a block.
    void beginBlock(std::size_t numSamples) noexcept;

    float processSample(float input) noexcept;

    void process(float* samples, std::size_t numSamples) noexcept;

private:

    //--------------------------------------------------------------------------
    // Delay-line helpers
    //--------------------------------------------------------------------------

    float readDelay() noexcept;
    void writeDelay(float sample) noexcept;

    //--------------------------------------------------------------------------
    // Parameters / state
    //--------------------------------------------------------------------------

    double sampleRate_ = 44100.0;

    float maxDelaySeconds_ = 2.0f;
    std::size_t bufferSize_ = 0;

    std::vector<float> delayBuffer_;

    std::size_t writePosition_ = 0;

    // Delay time is smoothed on the audio thread.
    std::atomic<float> delayTimeTargetSeconds_ { 0.5f };
    float delayTimeCurrentSamples_ = 0.0f;

    // Number of samples by which delay time can change per processed sample.
    float delayChangeSpeed_ = 2.0f;

    // Feedback and wet are communicated atomically because they may be
    // changed from the UI / parameter thread.
    std::atomic<float> feedback_ { 0.5f };
    std::atomic<float> wet_      { 0.5f };

    // The EQ belongs to this delay instance.
    GraphicEQ graphicEQ_;

    bool prepared_ = false;
};
