/*
  ==============================================================================

    EQState.h
    Created: 9 Sep 2026 7:00:51am
    Author:  Maximos Kaliakatsos-Papakostas

  ==============================================================================
*/

#pragma once

#include <array>

struct EQState
{
    static constexpr std::size_t NumBands = 31;

    std::array<float, NumBands> gains {};
};
