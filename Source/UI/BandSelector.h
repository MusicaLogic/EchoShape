/*
  ==============================================================================

    BandSelector.h
    Created: 5 Sep 2026 3:05:08pm
    Author:  Maximos Kaliakatsos-Papakostas

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class BandSelector : public juce::Component
{
public:

    //==============================================================
    BandSelector()
    {
        setInterceptsMouseClicks(true, false);
    }

    ~BandSelector() override = default;


    //==============================================================
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        const float left  = bounds.getX();
        const float right = bounds.getRight();
        const float top   = bounds.getY();
        const float bottom = bounds.getBottom();

        //==========================================================
        // Colours
        //==========================================================

        const auto backgroundColour =
            isActive
                ? VisualStyle::getStateColor(
                      colourSet,
                      isActive,
                      false).withAlpha(0.35f)
                : VisualStyle::getStateColor(
                      colourSet,
                      isActive,
                      false).withAlpha(0.12f);

        const auto borderColour =
            isActive
                ? VisualStyle::getStateColor(
                      colourSet,
                      isActive,
                      true).withAlpha(
                          isDragging ? 0.95f : 0.65f)
                : VisualStyle::getStateColor(
                      colourSet,
                      isActive,
                      true).withAlpha(0.25f);

        const auto tickColour =
            isActive
                ? VisualStyle::getStateColor(
                      colourSet,
                      isActive,
                      true).withAlpha(0.20f)
                : VisualStyle::getStateColor(
                      colourSet,
                      isActive,
                      true).withAlpha(0.08f);

        const auto indicatorColour =
            isActive
                ? VisualStyle::getStateColor(
                      colourSet,
                      isActive,
                      true).withAlpha(
                          isDragging ? 1.0f : 0.80f)
                : VisualStyle::getStateColor(
                      colourSet,
                      isActive,
                      true).withAlpha(0.30f);


        //==========================================================
        // Background
        //==========================================================

        g.setColour(backgroundColour);

        g.fillRoundedRectangle(
            bounds.reduced(1.0f),
            4.0f);


        //==========================================================
        // 31 band positions
        //==========================================================

        const float usableLeft  = left + 6.0f;
        const float usableRight = right - 6.0f;

        const float usableWidth =
            usableRight - usableLeft;

        for (int band = 0; band < 31; ++band)
        {
            const float proportion =
                static_cast<float>(band) / 30.0f;

            const float x =
                usableLeft + proportion * usableWidth;

            g.setColour(
                band == selectedBand
                    ? indicatorColour
                    : tickColour);

            const float tickHeight =
                (band == selectedBand)
                    ? bounds.getHeight() * 0.65f
                    : bounds.getHeight() * 0.30f;

            g.fillRect(
                x - 0.5f,
                bounds.getCentreY() - tickHeight * 0.5f,
                1.0f,
                tickHeight);
        }


        //==========================================================
        // Selected-band indicator
        //==========================================================

        if (selectedBand >= 0)
        {
            const float proportion =
                static_cast<float>(selectedBand) / 30.0f;

            const float x =
                usableLeft + proportion * usableWidth;

            g.setColour(indicatorColour);

            g.fillRect(
                x - 1.0f,
                top + 3.0f,
                2.0f,
                bounds.getHeight() - 6.0f);
        }


        //==========================================================
        // Active frame
        //==========================================================

        if (isActive)
        {
            g.setColour(borderColour);

            g.drawRoundedRectangle(
                bounds.reduced(1.0f),
                4.0f,
                isDragging ? 2.0f : 1.0f);
        }
    }


    //==============================================================
    void mouseDown(const juce::MouseEvent& event) override
    {
//        if (!isActive)
//            return;

        isDragging = true;

        updateBandFromPosition(event.position.x);

        repaint();
    }


    //==============================================================
    void mouseDrag(const juce::MouseEvent& event) override
    {
//        if (!isActive)
//            return;

        updateBandFromPosition(event.position.x);
    }


    //==============================================================
    void mouseUp(const juce::MouseEvent&) override
    {
        if (!isActive)
            return;

        isDragging = false;

        repaint();
    }


    //==============================================================
    void setActive(bool active)
    {
        isActive = active;

        if (!isActive)
            isDragging = false;

        repaint();
    }


    bool getIsActive() const
    {
        return isActive;
    }


    //==============================================================
    void setSelectedBand(int band)
    {
        selectedBand =
            juce::jlimit(0, 30, band);

        repaint();
    }


    int getSelectedBand() const
    {
        return selectedBand;
    }


    //==============================================================
    std::function<void(int)> onBandChanged;


private:

    //==============================================================
    void updateBandFromPosition(float x)
    {
        const float usableLeft  = 6.0f;
        const float usableRight =
            static_cast<float>(getWidth()) - 6.0f;

        const float usableWidth =
            usableRight - usableLeft;

        const float proportion =
            juce::jlimit(
                0.0f,
                1.0f,
                (x - usableLeft) / usableWidth);

        const int newBand =
            juce::roundToInt(proportion * 30.0f);

        if (newBand != selectedBand)
        {
            selectedBand = newBand;

            if (onBandChanged)
                onBandChanged(selectedBand);

            repaint();
        }
    }


    //==============================================================
    bool isActive = true;
    bool isDragging = false;

    int selectedBand = 15;

    const VisualStyle::ColorSet& colourSet =
        VisualStyle::Palette::cyan;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BandSelector)
};
