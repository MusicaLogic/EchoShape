/*
  ==============================================================================

    FeedbackDelay.cpp
    Created: 22 Sep 2026 6:41:00am
    Author:  Maximos Kaliakatsos-Papakostas

  ==============================================================================
*/

#include "FeedbackDelay.h"

#include <algorithm>
#include <cmath>
#include <cstring>

//==============================================================================
// Preparation
//==============================================================================

void FeedbackDelay::prepare(double sampleRate,
                            const GraphicEQ::Frequencies& frequencies,
                            float maxDelaySeconds)
{
    sampleRate_ = sampleRate;
    maxDelaySeconds_ = std::max(0.001f, maxDelaySeconds);

    // +2 gives us enough room for the interpolation read.
    bufferSize_ = static_cast<std::size_t>(
        std::ceil(sampleRate_ * maxDelaySeconds_)
    ) + 2;

    delayBuffer_.assign(bufferSize_, 0.0f);

    writePosition_ = 0;

    // Prepare the EQ owned by this delay.
    graphicEQ_.prepare(sampleRate_, frequencies);

    // Convert the current target delay into samples.
    const float initialDelaySeconds =
        delayTimeTargetSeconds_.load(std::memory_order_relaxed);

    delayTimeCurrentSamples_ =
        std::clamp(initialDelaySeconds, 0.0f, maxDelaySeconds_)
        * static_cast<float>(sampleRate_);
    
    freeze_ = 0;
    
    prepared_ = true;
}

//==============================================================================

void FeedbackDelay::reset()
{
    if (!prepared_)
        return;

    std::fill(delayBuffer_.begin(), delayBuffer_.end(), 0.0f);

    writePosition_ = 0;

    const float targetSeconds =
        delayTimeTargetSeconds_.load(std::memory_order_relaxed);

    delayTimeCurrentSamples_ =
        std::clamp(targetSeconds, 0.0f, maxDelaySeconds_)
        * static_cast<float>(sampleRate_);

    graphicEQ_.reset();
}

//==============================================================================
// Parameters
//==============================================================================

void FeedbackDelay::setDelayTime(float seconds) noexcept
{
    seconds = std::clamp(seconds, 0.0f, maxDelaySeconds_);

    delayTimeTargetSeconds_.store(
        seconds,
        std::memory_order_relaxed
    );
}

//==============================================================================

void FeedbackDelay::setFeedback(float feedback) noexcept
{
    // Keep feedback below unity. This is particularly important because
    // the EQ can provide positive gain inside the feedback loop.
    feedback = std::clamp(feedback, 0.0f, 0.999f);

    feedback_.store(
        feedback,
        std::memory_order_relaxed
    );
}

//==============================================================================

void FeedbackDelay::setWet(float wet) noexcept
{
    wet = std::clamp(wet, 0.0f, 1.0f);

    wet_.store(
        wet,
        std::memory_order_relaxed
    );
}

//==============================================================================

void FeedbackDelay::setFreeze(bool freeze) noexcept
{
    freeze_.store(freeze, std::memory_order_relaxed);
}

//==============================================================================

void FeedbackDelay::setEQGains(const GraphicEQ::Gains& gains) noexcept
{
    graphicEQ_.setGains(gains);
}

void FeedbackDelay::setEQGain(std::size_t band, float gainDb) noexcept
{
    graphicEQ_.setGain(band, gainDb);
}

float FeedbackDelay::getEQGain(std::size_t band) const{
    return graphicEQ_.getGain(band);
}

//==============================================================================
// Block processing
//==============================================================================

void FeedbackDelay::beginBlock(std::size_t numSamples) noexcept
{
    if (!prepared_)
        return;

    // GraphicEQ performs its own gain smoothing and coefficient update.
    graphicEQ_.beginBlock(numSamples);
}

//==============================================================================
// Delay-line read
//==============================================================================

float FeedbackDelay::readDelay() noexcept
{
    // Target delay in samples, smoothed toward its requested value.
    const float targetDelaySamples =
        std::clamp(
            delayTimeTargetSeconds_.load(std::memory_order_relaxed),
            0.0f,
            maxDelaySeconds_
        ) * static_cast<float>(sampleRate_);

    // Smooth delay-time changes.
    const float difference =
        targetDelaySamples - delayTimeCurrentSamples_;

    if (std::abs(difference) <= delayChangeSpeed_)
    {
        delayTimeCurrentSamples_ = targetDelaySamples;
    }
    else
    {
        delayTimeCurrentSamples_ +=
            (difference > 0.0f)
                ? delayChangeSpeed_
                : -delayChangeSpeed_;
    }

    // We use fractional delay interpolation.
    float readPosition =
        static_cast<float>(writePosition_)
        - delayTimeCurrentSamples_;

    while (readPosition < 0.0f)
        readPosition += static_cast<float>(bufferSize_);

    while (readPosition >= static_cast<float>(bufferSize_))
        readPosition -= static_cast<float>(bufferSize_);

    const auto index0 =
        static_cast<std::size_t>(readPosition);

    const auto index1 =
        (index0 + 1) % bufferSize_;

    const float fraction =
        readPosition - static_cast<float>(index0);

    const float sample0 = delayBuffer_[index0];
    const float sample1 = delayBuffer_[index1];

    // Linear interpolation.
    return sample0 + fraction * (sample1 - sample0);
}

//==============================================================================
// Delay-line write
//==============================================================================

void FeedbackDelay::writeDelay(float sample) noexcept
{
    delayBuffer_[writePosition_] = sample;

    ++writePosition_;

    if (writePosition_ >= bufferSize_)
        writePosition_ = 0;
}

//==============================================================================
// Sample processing
//==============================================================================

float FeedbackDelay::processSample(float input) noexcept
{
    if (!prepared_)
        return input;

    //-------------------------------------------------------------------------- 
    // 1. Read the delayed signal.
    //--------------------------------------------------------------------------

    const float delayedSample = readDelay();

    //-------------------------------------------------------------------------- 
    // 2. Process ONLY the feedback signal through the GraphicEQ.
    //
    //    The final delayed output remains un-EQ'd.
    //--------------------------------------------------------------------------

//    const float equalizedFeedback =
//        graphicEQ_.processSample(delayedSample);

    //-------------------------------------------------------------------------- 
    // 3. Put the EQ'd signal back into the feedback path.
    //--------------------------------------------------------------------------

    const float feedback =
        feedback_.load(std::memory_order_relaxed);
    
    const float freeze =
        (float)freeze_.load(std::memory_order_relaxed);
    
//    const float bufferInput =
//        (1.0f-freeze)*input + feedback * equalizedFeedback;
    
//    writeDelay(bufferInput);
    
    const float bufferInput =
        (1.0f-freeze)*input + feedback * delayedSample;
    
    const float equalizedBufferInput =
        graphicEQ_.processSample(bufferInput);
    
//    const float clippedBufferInput = equalizedBufferInput > 0.5 ? 0.5 :
//                equalizedBufferInput < -0.5 ? 0.5 : equalizedBufferInput;

    writeDelay(equalizedBufferInput);

    //-------------------------------------------------------------------------- 
    // 4. Return the normal delay output.
    //--------------------------------------------------------------------------

    const float wet =
        wet_.load(std::memory_order_relaxed);

    return input + delayedSample * wet;
}

//==============================================================================
// Block processing
//==============================================================================

void FeedbackDelay::process(float* samples,
                            std::size_t numSamples) noexcept
{
    if (!prepared_ || samples == nullptr)
        return;

    beginBlock(numSamples);

    for (std::size_t i = 0; i < numSamples; ++i)
        samples[i] = processSample(samples[i]);
}
