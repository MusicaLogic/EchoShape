/*
  ==============================================================================

    EQDrawingArea.cpp
    Created: 3 Sep 2026 7:25:02am
    Author:  Maximos Kaliakatsos-Papakostas

  ==============================================================================
*/

#include "EQDrawingArea.h"

#include <cmath>
#include <limits>

//==============================================================================
// Constructor
//==============================================================================

EQDrawingArea::EQDrawingArea()
{
    setOpaque(true);
    inputSpectrum.fill(spectrumMinDB);
    outputSpectrum.fill(spectrumMinDB);

    startTimerHz(15);
}


//==============================================================================
// JUCE
//==============================================================================

void EQDrawingArea::paint(juce::Graphics& g)
{
    g.fillAll(VisualStyle::panelBackground);

    drawFrame(g);

    drawAxisGuides(g);
    
    drawSpectrum(g);

    drawPlotFill(g);
    drawPlot(g);

    if (interactionMode == InteractionMode::bandSelected &&
        selectedBand >= 0)
    {
        drawSelectedBandLine(g);
    }
    else if (interactionMode == InteractionMode::drawing &&
             hoveredBand >= 0)
    {
        drawSelectedBandLine(g);
    }
}


//==============================================================================
// Drawing
//==============================================================================

void EQDrawingArea::drawFrame(juce::Graphics& g)
{
    const auto bounds = getLocalBounds()
                            .toFloat()
                            .reduced(1.0f);

    const bool active =
        interactionMode == InteractionMode::drawing;

    const auto colour =
        VisualStyle::getStateColor(
            colourSet,
            active,
            true);

    g.setColour(colour);

    g.drawRoundedRectangle(
        bounds,
        VisualStyle::Geometry::componentCornerRadius,
        VisualStyle::Geometry::componentBorderThickness);
}


void EQDrawingArea::drawPlot(juce::Graphics& g)
{
    const bool active =
        interactionMode == InteractionMode::drawing;

    const auto colour =
        VisualStyle::getStateColor(
            colourSet,
            active,
            true);

    juce::Path path;

    for (size_t i = 0; i < standardEQFrequencies.size(); ++i)
    {
        const float x =
            frequencyToX(standardEQFrequencies[i]);

        const float y =
            gainToY(state.gains[i]);

        if (i == 0)
            path.startNewSubPath(x, y);
        else
            path.lineTo(x, y);
    }

    g.setColour(colour);

    g.strokePath(
        path,
        juce::PathStrokeType(
            2.0f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));
}


void EQDrawingArea::drawPlotFill(juce::Graphics& g)
{
    const auto bounds = getPlotBounds();

    juce::Path fillPath;

    for (size_t i = 0; i < standardEQFrequencies.size(); ++i)
    {
        const float x =
            frequencyToX(standardEQFrequencies[i]);

        const float y =
            gainToY(state.gains[i]);

        if (i == 0)
            fillPath.startNewSubPath(x, y);
        else
            fillPath.lineTo(x, y);
    }

    // Extend the path down to the bottom of the plot.
    fillPath.lineTo(
        frequencyToX(standardEQFrequencies.back()),
        bounds.getBottom());

    fillPath.lineTo(
        frequencyToX(standardEQFrequencies.front()),
        bounds.getBottom());

    fillPath.closeSubPath();

    const bool active =
        interactionMode == InteractionMode::drawing;

    const auto colour =
        VisualStyle::getStateColor(
            colourSet,
            active,
            true);

    g.setColour(colour.withAlpha(0.04f));
    g.fillPath(fillPath);
}


void EQDrawingArea::drawSelectedBandLine(juce::Graphics& g)
{
    int band = -1;

    if (interactionMode == InteractionMode::drawing)
        band = hoveredBand;
    else
        band = selectedBand;

    if (band < 0 ||
        band >= static_cast<int>(
            standardEQFrequencies.size()))
    {
        return;
    }

    const float x =
        frequencyToX(
            standardEQFrequencies[
                static_cast<size_t>(band)]);

    const auto bounds = getPlotBounds();

    const float alpha =
        interactionMode == InteractionMode::drawing
            ? 0.18f
            : 0.40f;

    g.setColour(
        colourSet.dim.withAlpha(alpha));

    g.drawVerticalLine(
        juce::roundToInt(x),
        bounds.getY(),
        bounds.getBottom());
}

void EQDrawingArea::drawAxisGuides(juce::Graphics& g)
{
    const auto bounds = getPlotBounds();

    const auto guideColour =
        colourSet.dim.withAlpha(0.25f);

    const auto textColour =
        colourSet.dim.withAlpha(0.65f);

    // ============================================================
    // Horizontal gain guides
    // ============================================================

    constexpr float guideGains[] =
    {
        6.0f,
        0.0f,
        -6.0f
    };

    constexpr const char* guideLabels[] =
    {
        "+6dB",
        "0dB",
        "-6dB"
    };

    for (int i = 0; i < 3; ++i)
    {
        const float y =
            gainToY(guideGains[i]);

        // Horizontal reference line
        g.setColour(guideColour);

        g.drawHorizontalLine(
            juce::roundToInt(y),
            bounds.getX(),
            bounds.getRight());

        // Label inside the graph.
        //
        // We offset slightly from the left edge so that
        // the text isn't sitting directly on the frame.
        const float textX =
            bounds.getX() + 8.0f;

        float textY = y;

        // Keep the text visually inside the graph.
        if (i == 0)
            textY += 2.0f;
        else if (i == 2)
            textY -= 2.0f;

        g.setColour(textColour);

        g.setFont(
            VisualStyle::getDefaultFont(
                VisualStyle::FontSize::small));

        g.drawText(
            guideLabels[i],
            juce::roundToInt(textX),
            juce::roundToInt(textY - 7.0f),
            45,
            14,
            juce::Justification::left,
            false);
    }


    // ============================================================
    // Vertical frequency guides
    // ============================================================

    constexpr double guideFrequencies[] =
    {
        100.0,
        1000.0,
        10000.0
    };

    constexpr const char* frequencyLabels[] =
    {
        "100Hz",
        "1kHz",
        "10kHz"
    };

    for (int i = 0; i < 3; ++i)
    {
        const float x =
            frequencyToX(guideFrequencies[i]);

        // Vertical guide line
        g.setColour(guideColour);

        g.drawVerticalLine(
            juce::roundToInt(x),
            bounds.getY(),
            bounds.getBottom());

        // Frequency label near the bottom of the graph.
        //
        // Center each label on its guide line.
        g.setColour(textColour);

        g.setFont(
            VisualStyle::getDefaultFont(
                VisualStyle::FontSize::small));

        g.drawText(
            frequencyLabels[i],
            juce::roundToInt(x - 25.0f),
            juce::roundToInt(bounds.getBottom() - 20.0f),
            50,
            14,
            juce::Justification::centred,
            false);
    }
}


//==============================================================================
// Coordinate conversion
//==============================================================================

float EQDrawingArea::frequencyToX(double frequency) const
{
    const auto bounds = getPlotBounds();

    constexpr double minFrequency = 20.0;
    constexpr double maxFrequency = 20000.0;

    const double minLog =
        std::log10(minFrequency);

    const double maxLog =
        std::log10(maxFrequency);

    const double normalized =
        (std::log10(frequency) - minLog)
        / (maxLog - minLog);

    return bounds.getX()
         + static_cast<float>(normalized)
           * bounds.getWidth();
}


double EQDrawingArea::xToFrequency(float x) const
{
    const auto bounds = getPlotBounds();

    constexpr double minFrequency = 20.0;
    constexpr double maxFrequency = 20000.0;

    const double normalized =
        juce::jlimit(
            0.0,
            1.0,
            static_cast<double>(
                (x - bounds.getX())
                / bounds.getWidth()));

    const double minLog =
        std::log10(minFrequency);

    const double maxLog =
        std::log10(maxFrequency);

    return std::pow(
        10.0,
        minLog + normalized * (maxLog - minLog));
}


float EQDrawingArea::gainToY(float gain) const
{
    const auto bounds = getPlotBounds();

    const float normalized =
        juce::jmap(
            juce::jlimit(minGain, maxGain, gain),
            minGain,
            maxGain,
            1.0f,
            0.0f);

    return bounds.getY()
         + normalized * bounds.getHeight();
}


float EQDrawingArea::yToGain(float y) const
{
    const auto bounds = getPlotBounds();

    const float normalized =
        juce::jlimit(
            0.0f,
            1.0f,
            (y - bounds.getY())
            / bounds.getHeight());

    return juce::jmap(
        normalized,
        1.0f,
        0.0f,
        minGain,
        maxGain);
}


//==============================================================================
// Geometry
//==============================================================================

juce::Rectangle<float> EQDrawingArea::getPlotBounds() const
{
    return getLocalBounds()
        .toFloat()
        .reduced(plotPadding);
}


//==============================================================================
// Interaction
//==============================================================================

int EQDrawingArea::findClosestBand(float x) const
{
    int closestBand = 0;

    float closestDistance =
        std::numeric_limits<float>::max();

    for (size_t i = 0;
         i < standardEQFrequencies.size();
         ++i)
    {
        const float bandX =
            frequencyToX(standardEQFrequencies[i]);

        const float distance =
            std::abs(x - bandX);

        if (distance < closestDistance)
        {
            closestDistance = distance;
            closestBand =
                static_cast<int>(i);
        }
    }

    return closestBand;
}


void EQDrawingArea::updateFromMousePosition(
    const juce::Point<float>& position)
{
    const auto bounds = getPlotBounds();

    if (!bounds.contains(position))
        return;

    const int band =
        findClosestBand(position.x);

    const float gain =
        yToGain(position.y);

    state.gains[
        static_cast<size_t>(band)] = gain;

    notifyEQChanged();

    repaint();
}


//==============================================================================
// Mouse interaction
//==============================================================================

void EQDrawingArea::mouseDown(
    const juce::MouseEvent& event)
{
    // If we are currently in band-selection mode,
    // touching the graph switches back to drawing mode.
    if (interactionMode != InteractionMode::drawing)
    {
        if (onDrawingStarted)
            onDrawingStarted();

        return;
    }
    updateFromMousePosition(event.position);
    
    if (interactionMode == InteractionMode::drawing)
    {
        return;
    }

    // Band-selected mode:
    // select whichever band is closest to the mouse.
    selectedBand =
        findClosestBand(event.position.x);
    setSelectedBand(selectedBand);

    repaint();
}


void EQDrawingArea::mouseDrag(
    const juce::MouseEvent& event)
{
    if (interactionMode == InteractionMode::drawing)
    {
        updateFromMousePosition(event.position);
        const int newHoveredBand =
            findClosestBand(event.position.x);

        if (newHoveredBand != hoveredBand)
        {
            hoveredBand = newHoveredBand;
            repaint();
        }
        setSelectedBand(hoveredBand);
        return;
    }

    // Band-selected mode:
    // vertically move the currently selected band.
    if (selectedBand >= 0)
    {
        const float gain =
            yToGain(event.position.y);

        state.gains[
            static_cast<size_t>(selectedBand)] =
            gain;

        notifyEQChanged();

        repaint();
    }
}


void EQDrawingArea::mouseUp(
    const juce::MouseEvent&)
{
    if (interactionMode == InteractionMode::drawing)
    {
        hoveredBand = -1;
        repaint();
    }
}

void EQDrawingArea::mouseMove(
    const juce::MouseEvent& event)
{
    if (interactionMode != InteractionMode::drawing)
        return;

    const auto bounds = getPlotBounds();

    if (!bounds.contains(event.position))
    {
        if (hoveredBand != -1)
        {
            hoveredBand = -1;
            repaint();
        }

        return;
    }

    const int newHoveredBand =
        findClosestBand(event.position.x);

    if (newHoveredBand != hoveredBand)
    {
        hoveredBand = newHoveredBand;
        repaint();
    }
}

void EQDrawingArea::mouseExit(
    const juce::MouseEvent&)
{
    if (interactionMode == InteractionMode::drawing &&
        hoveredBand != -1)
    {
        hoveredBand = -1;
        repaint();
    }
}


//==============================================================================
// EQ state
//==============================================================================

void EQDrawingArea::setEQState(
    const EQState& newState)
{
    state = newState;

    repaint();
}

void EQDrawingArea::setGains(
    const std::array<float, numBands>& newGains)
{
    for (int i = 0; i < numBands; ++i)
        state.gains[static_cast<size_t>(i)] = newGains[static_cast<size_t>(i)];

    repaint();
}

float EQDrawingArea::getBandGain(int band) const noexcept
{
    if (band < 0 || band >= numBands)
        return 0.0f;

    return state.gains[static_cast<size_t>(band)];
}


//==============================================================================
// Interaction mode
//==============================================================================

void EQDrawingArea::setInteractionMode(
    InteractionMode newMode)
{
    if (interactionMode == newMode)
        return;

    interactionMode = newMode;

    selectedBand = -1;
    hoveredBand = -1;

    repaint();
}

void EQDrawingArea::setSelectedBand(int newSelectedBand) noexcept
{
    if (newSelectedBand < -1 || newSelectedBand >= numBands)
        newSelectedBand = -1;

    if (selectedBand == newSelectedBand)
        return;

    selectedBand = newSelectedBand;

    if (onSelectedBandChanged)
        onSelectedBandChanged(selectedBand);

    repaint();
}


//==============================================================================
// Change notification
//==============================================================================

void EQDrawingArea::notifyEQChanged()
{
    if (onEQChanged)
        onEQChanged(state);
}

//==============================================================================
// Spectrum plotting
//==============================================================================

void EQDrawingArea::setSpectrumBuffer(
    SpectrumBuffer& buffer) noexcept
{
    spectrumBuffer = &buffer;
}

void EQDrawingArea::timerCallback()
{
    updateSpectrumVisualization();

    repaint();
}

void EQDrawingArea::updateSpectrumVisualization()
{
    if (spectrumBuffer == nullptr)
        return;

    inputSpectrum =
        spectrumBuffer->getInput();

    outputSpectrum =
        spectrumBuffer->getOutput();
}

float EQDrawingArea::spectrumDBToY(
    float dB) const
{
    const auto bounds = getPlotBounds();

    const float normalized =
        juce::jmap(
            juce::jlimit(
                spectrumMinDB,
                spectrumMaxDB,
                dB),
            spectrumMinDB,
            spectrumMaxDB,
            1.0f,
            0.0f);

    return bounds.getY()
         + normalized * bounds.getHeight();
}

void EQDrawingArea::drawSpectrum(
    juce::Graphics& g)
{
    drawSpectrumLines(
        g,
        inputSpectrum,
        VisualStyle::Palette::blue.highlight);

    drawSpectrumLines(
        g,
        outputSpectrum,
        VisualStyle::Palette::green.highlight);
}

void EQDrawingArea::drawSpectrumLines(
    juce::Graphics& g,
    const Spectrum& spectrum,
    const juce::Colour& colour)
{
    const auto bounds =
        getPlotBounds();

    g.setColour(
        colour.withAlpha(0.45f));


    for (int i = 0;
         i < numSpectrumBands;
         ++i)
    {
        const float lowFrequency =
            SpectrumAnalyzer::getBandLowFrequency(i);

        const float highFrequency =
            SpectrumAnalyzer::getBandHighFrequency(i);


        const float x1 =
            frequencyToX(lowFrequency);

        const float x2 =
            frequencyToX(highFrequency);


        const float y =
            spectrumDBToY(
                spectrum[
                    static_cast<size_t>(i)]);


        g.drawHorizontalLine(
            juce::roundToInt(y),
            x1,
            x2);
    }
}
