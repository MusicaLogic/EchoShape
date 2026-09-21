/*
  ==============================================================================

    SpectrumBands.h
    Created: 12 Sep 2026 6:32:00am
    Author:  Maximos Kaliakatsos-Papakostas

  ==============================================================================
*/

#pragma once

#include <array>
#include <cmath>

class SpectrumBands
{
public:
    static constexpr int numBands = 32;

    struct Band
    {
        float lowFrequency;
        float highFrequency;
        float centreFrequency;
    };

    SpectrumBands(
        float minFrequency = 20.0f,
        float maxFrequency = 20000.0f)
    {
        const double minLog = std::log10(minFrequency);
        const double maxLog = std::log10(maxFrequency);

        for (int i = 0; i < numBands; ++i)
        {
            const double t0 =
                static_cast<double>(i) / numBands;

            const double t1 =
                static_cast<double>(i + 1) / numBands;

            const double tc =
                (t0 + t1) * 0.5;

            bands[static_cast<size_t>(i)] =
            {
                static_cast<float>(
                    std::pow(10.0,
                             minLog + t0 * (maxLog - minLog))),

                static_cast<float>(
                    std::pow(10.0,
                             minLog + t1 * (maxLog - minLog))),

                static_cast<float>(
                    std::pow(10.0,
                             minLog + tc * (maxLog - minLog)))
            };
        }
    }

    const Band& operator[](int index) const noexcept
    {
        return bands[static_cast<size_t>(index)];
    }

private:
    std::array<Band, numBands> bands {};
};
