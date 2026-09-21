/*
  ==============================================================================

    SpectrumDataBuffer.h
    Created: 12 Sep 2026 6:32:37am
    Author:  Maximos Kaliakatsos-Papakostas

  ==============================================================================
*/

/*
  ==============================================================================

    SpectrumDataBuffer.h

    Thread-safe latest-value communication between the audio thread
    and the GUI thread.

  ==============================================================================
*/

#pragma once

#include <array>
#include <atomic>

template <size_t NumBands>
class SpectrumDataBuffer
{
public:

    using Spectrum = std::array<float, NumBands>;


    // ================================================================
    // Audio thread
    // ================================================================

    void publishInput(const Spectrum& spectrum) noexcept
    {
        for (size_t i = 0; i < NumBands; ++i)
        {
            input[i].store(
                spectrum[i],
                std::memory_order_relaxed);
        }
    }


    void publishOutput(const Spectrum& spectrum) noexcept
    {
        for (size_t i = 0; i < NumBands; ++i)
        {
            output[i].store(
                spectrum[i],
                std::memory_order_relaxed);
        }
    }


    // ================================================================
    // GUI thread
    // ================================================================

    Spectrum getInput() const noexcept
    {
        Spectrum result {};

        for (size_t i = 0; i < NumBands; ++i)
        {
            result[i] =
                input[i].load(
                    std::memory_order_relaxed);
        }

        return result;
    }


    Spectrum getOutput() const noexcept
    {
        Spectrum result {};

        for (size_t i = 0; i < NumBands; ++i)
        {
            result[i] =
                output[i].load(
                    std::memory_order_relaxed);
        }

        return result;
    }


private:

    std::array<std::atomic<float>, NumBands> input {};
    std::array<std::atomic<float>, NumBands> output {};
};
