/*
  ==============================================================================

    EQBand.h
    Created: 3 Sep 2026 7:24:20am
    Author:  Maximos Kaliakatsos-Papakostas

  ==============================================================================
*/

#pragma once

#include <array>
#include "../State/EQState.h"

struct EQBand
{
    double frequency;
    float gain;
};

// Standard 31-band graphic EQ frequencies.
inline constexpr std::array<double, 31> standardEQFrequencies =
{
    20.0,
    25.0,
    31.5,
    40.0,
    50.0,
    63.0,
    80.0,
    100.0,
    125.0,
    160.0,
    200.0,
    250.0,
    315.0,
    400.0,
    500.0,
    630.0,
    800.0,
    1000.0,
    1250.0,
    1600.0,
    2000.0,
    2500.0,
    3150.0,
    4000.0,
    5000.0,
    6300.0,
    8000.0,
    10000.0,
    12500.0,
    16000.0,
    20000.0
};

//struct EQState
//{
//    std::array<float, 31> gains {};
//
//    EQState()
//    {
//        gains.fill(0.0f);
//    }
//};
