/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

//#include "PluginProcessor.h"
#include "ParaGraphiQComponent.h"
#include <cmath>

//==============================================================================
ParaGraphiQComponent::ParaGraphiQComponent (
                                            EchoShapeAudioProcessor& processor)
                                            : audioProcessor (processor)
{
    addAndMakeVisible(eqDrawingArea);
    addAndMakeVisible(smoothnessBand);
    addAndMakeVisible(gainBand);
    addAndMakeVisible(bandSelector);
    
    // add reset button
    addAndMakeVisible(resetButton);
    configure_resetButton();
    
    //TODO: load state from somewhere
    const auto state = audioProcessor.getEQState();
    gains = state.gains;
    eqDrawingArea.setEQState(state);
    gainsAtGestureStart = gains;
    // reflect initial state on drawing area
    eqDrawingArea.setSelectedBand(selectedBand);
    // make sure DSP starts the same
    updateDSP();
    
    // Connect the spectrum visualization to the processor.
    eqDrawingArea.setSpectrumBuffer(
        audioProcessor.getSpectrumData());

    setupCallbacks();
    setInteractionMode(interactionMode);
    
    audioProcessor.addChangeListener(this);
}

ParaGraphiQComponent::~ParaGraphiQComponent()
{
    audioProcessor.removeChangeListener(this);
}

//==============================================================================
void ParaGraphiQComponent::paint (juce::Graphics& g)
{
//    // (Our component is opaque, so we must completely fill the background with a solid colour)
//    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
//
//    g.setColour (juce::Colours::white);
//    g.setFont (juce::FontOptions (15.0f));
//    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
    auto bounds = getLocalBounds().reduced (2);

    g.setColour (VisualStyle::panelBackground);
    g.fillRoundedRectangle (
        bounds.toFloat(),
        VisualStyle::Geometry::componentCornerRadius);

    g.setColour (VisualStyle::Palette::cyan.dim);
    g.drawRoundedRectangle (
        bounds.toFloat(),
        VisualStyle::Geometry::componentCornerRadius,
        VisualStyle::Geometry::componentBorderThickness);
    
}

void ParaGraphiQComponent::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto area =
        getLocalBounds().reduced(
            VisualStyle::Geometry::componentMargin);

    auto bottom = area.removeFromBottom(50);

    auto left  = area.removeFromLeft(60);
    auto right = area.removeFromRight(60);

    // The drawing area defines the exact horizontal extent
    // shared by the graph and the band selector.
    eqDrawingArea.setBounds(area);

    smoothnessBand.setBounds(left.reduced(10));
    gainBand.setBounds(right.reduced(10));
    
//    // Right: gain control + reset button.
//    auto rightArea = right.reduced(10);
//
//    auto resetArea = rightArea.removeFromBottom(34);
//    
//    gainBand.setBounds(right.reduced(10));
//    resetButton.setBounds(resetArea);

//    auto selectorBounds = bottom;
//    selectorBounds.setX(area.getX());
//    selectorBounds.setWidth(area.getWidth());
//
//    bandSelector.setBounds(selectorBounds.reduced(0, 10));
    
    // 2. Chop off 10px from the left (this creates your empty margin)
    auto left_margin = bottom.removeFromLeft (60);

    // 3. Chop off 10px from the right (this creates the reset_area)
    auto resetArea = bottom.removeFromRight(60).reduced(10, 10);

    // 4. Whatever is left in the middle becomes your selector_area
    auto selector_area = bottom;
    
    bandSelector.setBounds(selector_area.reduced(0, 10));
    resetButton.setBounds(resetArea);
}

//==============================================================================
// Interaction mode
//==============================================================================

void ParaGraphiQComponent::setInteractionMode(
    EQDrawingArea::InteractionMode newMode)
{
    interactionMode = newMode;

    eqDrawingArea.setInteractionMode(newMode);

    const bool bandMode =
        newMode == EQDrawingArea::InteractionMode::bandSelected;

    // In drawing mode there are deliberately no touch controls.
    smoothnessBand.setActive(bandMode);
    gainBand.setActive(bandMode);
    bandSelector.setActive(bandMode);

    bandSelector.setSelectedBand(selectedBand);
    eqDrawingArea.setSelectedBand(selectedBand);

//    if (bandMode)
//    {
//        bandSelector.setSelectedBand(selectedBand);
//        eqDrawingArea.setSelectedBand(selectedBand);
//    }
//    else
//    {
//        eqDrawingArea.setSelectedBand(-1);
//    }

    repaint();
}

//==============================================================================
// Callbacks
//==============================================================================

void ParaGraphiQComponent::setupCallbacks()
{
    // BandSelector -> EQEditor
    bandSelector.onBandChanged =
        [this](int band)
        {
            setInteractionMode(
                    EQDrawingArea::InteractionMode::bandSelected);
            setSelectedBand(band);
        };

    // EQDrawingArea -> EQEditor
    eqDrawingArea.onSelectedBandChanged =
        [this](int band)
        {
            if (band >= 0)
                setSelectedBand(band);
        };

    eqDrawingArea.onEQChanged =
        [this](const EQState& newState)
        {
            for (int i = 0; i < numBands; ++i)
                gains[static_cast<size_t>(i)] =
                    newState.gains[static_cast<size_t>(i)];
            
            // The drawing interaction determines the relevant band.
            const int band =
                eqDrawingArea.getSelectedBand();

            if (band >= 0)
                setSelectedBand(band);
            
            updateDSP();
        };

    // Smoothness gesture
    smoothnessBand.onGestureStart =
        [this]()
        {
            setInteractionMode(
                    EQDrawingArea::InteractionMode::bandSelected);
            gainsAtGestureStart = gains;
        };

    smoothnessBand.onValueChanged =
        [this](float amount)
        {
            handleSmoothnessGesture(amount);
        };

    // Gain gesture
    gainBand.onGestureStart =
        [this]()
        {
            setInteractionMode(
                    EQDrawingArea::InteractionMode::bandSelected);

            if (selectedBand >= 0)
                gainAtGestureStart =
                    gains[static_cast<size_t>(selectedBand)];
        };

    gainBand.onValueChanged =
        [this](float amount)
        {
            handleGainGesture(amount);
        };
    
    //
    eqDrawingArea.onDrawingStarted =
        [this]()
        {
            setInteractionMode(
                EQDrawingArea::InteractionMode::drawing);
        };
    
    // Reset EQ
    resetButton.onClick =
        [this]()
        {
            resetEQ();
        };
}

//==============================================================================
// Selected band
//==============================================================================

void ParaGraphiQComponent::setSelectedBand(int band)
{
    if (band < 0 || band >= numBands)
        return;

    selectedBand = band;
    
    bandSelector.setSelectedBand(selectedBand);
    eqDrawingArea.setSelectedBand(selectedBand);
}

//==============================================================================
// Smoothness
//==============================================================================

void ParaGraphiQComponent::handleSmoothnessGesture(float amount)
{
    if (selectedBand < 0)
        return;

    // Always calculate from the curve that existed when the
    // finger touched the control. This makes the gesture
    // independent of the number of mouse/touch events.
    gains = gainsAtGestureStart;

    applySmoothness(gains, selectedBand, amount);

    eqDrawingArea.setGains(gains);
    updateDSP();
}

void ParaGraphiQComponent::applySmoothness(
    std::array<float, numBands>& targetGains,
    int centreBand,
    float amount)
{
    if (centreBand < 0 || centreBand >= numBands)
        return;

    amount = juce::jlimit(0.0f, 1.0f, amount);

    if (amount <= 0.0f)
        return;

    const auto original = targetGains;

    const float anchorGain =
        original[static_cast<size_t>(centreBand)];

    const int radius =
        juce::jmax(
            1,
            juce::roundToInt(1.0f + 4 * amount));

    for (int i = juce::jmax(0, centreBand - radius);
         i <= juce::jmin(numBands - 1, centreBand + radius);
         ++i)
    {
        if (i == centreBand)
            continue;

        const int distance =
            std::abs(i - centreBand);

        // Neighbours farther away contribute less.
        const float distanceWeight =
            1.0f -
            static_cast<float>(distance) /
            static_cast<float>(radius + 1);

        // ---------------------------------------------------------
        // How far this point currently is from the anchor.
        // ---------------------------------------------------------

        const float neighbourGain =
            original[static_cast<size_t>(i)];

        const float difference =
            anchorGain - neighbourGain;

        if (std::abs(difference) < 1.0e-6f)
            continue;


        // ---------------------------------------------------------
        // Calculate the local average.
        // ---------------------------------------------------------

        const int leftBand =
            juce::jmax(0, i - 1);

        const int rightBand =
            juce::jmin(numBands - 1, i + 1);

        const float localAverage =
            0.25f * original[static_cast<size_t>(leftBand)] +
            0.50f * neighbourGain +
            0.25f * original[static_cast<size_t>(rightBand)];


        // ---------------------------------------------------------
        // Determine how much the local smoothing wants to move
        // this point.
        // ---------------------------------------------------------

        float desiredChange =
            (1./(distance+1.))*anchorGain + localAverage - neighbourGain;
//            localAverage - neighbourGain;


        // ---------------------------------------------------------
        // Only allow movement toward the anchor.
        // ---------------------------------------------------------

        if (difference > 0.0f)
            desiredChange =
                juce::jmax(0.0f, desiredChange);
        else
            desiredChange =
                juce::jmin(0.0f, desiredChange);


        // ---------------------------------------------------------
        // Relative distance from anchor.
        //
        // 1 = far away
        // 0 = already at anchor
        //
        // The 12 dB normalization matches the current EQ range.
        // ---------------------------------------------------------

        const float normalizedDistance =
            juce::jlimit(
                0.0f,
                1.0f,
                std::abs(difference) / 12.0f);


        // Quadratic "soft landing".
        const float anchorWeight =
            normalizedDistance *
            normalizedDistance;


        const float strength =
            amount *
            distanceWeight *
            std::pow(anchorWeight, 0.1);


        float change =
            desiredChange * strength;


        // ---------------------------------------------------------
        // Absolute safety constraint:
        // never cross the anchor.
        // ---------------------------------------------------------

        if (difference > 0.0f)
        {
            change =
                juce::jmin(change, difference);
        }
        else
        {
            change =
                juce::jmax(change, difference);
        }


        targetGains[static_cast<size_t>(i)] =
            neighbourGain + change;
    }
}

//==============================================================================
// Gain
//==============================================================================

void ParaGraphiQComponent::handleGainGesture(float amount)
{
    if (selectedBand < 0)
        return;

    // amount is already signed and non-linear because TouchBand
    // performs the gesture response mapping.
    constexpr float gainRange = 12.0f;

    const float newGain =
        gainAtGestureStart + amount * gainRange;

    gains[static_cast<size_t>(selectedBand)] =
        juce::jlimit(-12.0f, 12.0f, newGain);

    eqDrawingArea.setGains(gains);
    updateDSP();
}

//==============================================================================
// DSP
//==============================================================================

void ParaGraphiQComponent::updateDSP()
{
    audioProcessor.setGains(gains);
}

// Listen to audio processor state changes
void ParaGraphiQComponent::changeListenerCallback(
    juce::ChangeBroadcaster* source)
{
    if (source != &audioProcessor)
        return;

    const auto state =
        audioProcessor.getEQState();

    gains = state.gains;
    gainsAtGestureStart = gains;

    eqDrawingArea.setEQState(state);
}

//==============================================================================
// Reset
//==============================================================================

void ParaGraphiQComponent::resetEQ()
{
    // Reset the EQ curve to flat.
    gains.fill(0.0f);

    // Any subsequent smoothness gesture should start
    // from the newly reset curve.
    gainsAtGestureStart = gains;

    // Reset the starting point used by the gain gesture.
    gainAtGestureStart = 0.0f;

    // Update the visual representation.
    eqDrawingArea.setGains(gains);

    // Update the DSP.
    updateDSP();
}

void ParaGraphiQComponent::configure_resetButton(){
    // configure reset button
    resetButton.setButtonText("---");
    resetButton.setTooltip("Reset EQ");
    // set colors
    const auto borderColour =
        VisualStyle::getStateColor(
                                     colourSet,
                                     false,
                                   true).withAlpha(0.35f);
    // make visuals of button
    resetButton.setColour(
        juce::TextButton::buttonColourId,
        VisualStyle::panelBackground);
    resetButton.setColour(
        juce::TextButton::textColourOffId,
                          borderColour);
    resetButton.setColour(
        juce::TextButton::textColourOnId,
                          borderColour);
    resetButton.setColour(juce::ComboBox::outlineColourId, borderColour);
}
