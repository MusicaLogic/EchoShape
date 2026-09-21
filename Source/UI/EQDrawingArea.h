/*
  ==============================================================================

    EQDrawingArea.h
    Created: 3 Sep 2026 7:25:02am
    Author:  Maximos Kaliakatsos-Papakostas

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Style/VisualStyle.h"
#include "EQBand.h"
#include "../DSP/SpectrumAnalyzer.h"
#include "../DSP/SpectrumDataBuffer.h"

class EQDrawingArea : public juce::Component,
                        private juce::Timer
{
public:

    enum class InteractionMode
    {
        drawing,
        bandSelected
    };

    EQDrawingArea();
    ~EQDrawingArea() override = default;

    // ============================================================
    // JUCE
    // ============================================================

    void paint(juce::Graphics& g) override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;


    // ============================================================
    // EQ state
    // ============================================================

    void setEQState(const EQState& newState);
    const EQState& getEQState() const noexcept { return state; }
    
    void setGains(const std::array<float, 31>& newGains);
    float getBandGain(int band) const noexcept;


    // ============================================================
    // Interaction mode
    // ============================================================

    void setInteractionMode(InteractionMode newMode);
    InteractionMode getInteractionMode() const noexcept
    {
        return interactionMode;
    }


    // ============================================================
    // Selected band
    // ============================================================

    int getSelectedBand() const noexcept
    {
        return selectedBand;
    }
    
    void setSelectedBand(int newSelectedBand) noexcept;
    
    // ============================================================
    // Change notification
    // ============================================================

    std::function<void(const EQState&)> onEQChanged;
    std::function<void(int)> onSelectedBandChanged;
    std::function<void()> onDrawingStarted;
    
    // ============================================================
    // Spectrum plotting
    // ============================================================
    
    static constexpr int numSpectrumBands =
        SpectrumAnalyzer::numSpectrumBands;

    using Spectrum =
        SpectrumAnalyzer::Spectrum;

    using SpectrumBuffer =
        SpectrumDataBuffer<numSpectrumBands>;
    
    void setSpectrumBuffer(
        SpectrumBuffer& buffer) noexcept;

private:

    // ============================================================
    // Drawing
    // ============================================================

    void drawFrame(juce::Graphics& g);
    void drawPlot(juce::Graphics& g);
    void drawPlotFill(juce::Graphics& g);
    void drawSelectedBandLine(juce::Graphics& g);
    void drawAxisGuides(juce::Graphics& g);
    
    static constexpr int numBands = 31;

    // ============================================================
    // Coordinate conversion
    // ============================================================

    float frequencyToX(double frequency) const;
    double xToFrequency(float x) const;

    float gainToY(float gain) const;
    float yToGain(float y) const;

    // ============================================================
    // Interaction
    // ============================================================

    int findClosestBand(float x) const;
    void updateFromMousePosition(const juce::Point<float>& position);
    void notifyEQChanged();

    // ============================================================
    // Geometry
    // ============================================================

    juce::Rectangle<float> getPlotBounds() const;


    // ============================================================
    // State
    // ============================================================

    EQState state;
    InteractionMode interactionMode = InteractionMode::drawing;
    int selectedBand = -1;
    
    // Band currently under the mouse cursor.
    // Used for the subtle vertical guide during drawing.
    int hoveredBand = -1;


    // ============================================================
    // EQ display parameters
    // ============================================================

    static constexpr float minGain = -12.0f;
    static constexpr float maxGain =  12.0f;


    // Padding inside component
    static constexpr float plotPadding = 10.0f;


    // Appearance
    const VisualStyle::ColorSet& colourSet = VisualStyle::Palette::cyan;
    
    // ============================================================
    // Spectrum visualization
    // ============================================================

    SpectrumBuffer* spectrumBuffer = nullptr;

    Spectrum inputSpectrum {};
    Spectrum outputSpectrum {};

    void timerCallback() override;

    void updateSpectrumVisualization();

    void drawSpectrum(
        juce::Graphics& g);

    void drawSpectrumLines(
        juce::Graphics& g,
        const Spectrum& spectrum,
        const juce::Colour& colour);
    
    static constexpr float spectrumMinDB = -80.0f;
    static constexpr float spectrumMaxDB = 0.0f;
    
    float spectrumDBToY(float dB) const;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EQDrawingArea)
};
