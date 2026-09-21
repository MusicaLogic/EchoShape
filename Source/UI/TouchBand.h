/*
  ==============================================================================

    TouchBand.h
    Created: 5 Sep 2026 6:42:39am
    Author:  Maximos Kaliakatsos-Papakostas

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class TouchBand : public juce::Component
{
public:
    enum class Direction
    {
        UpOnly,
        UpDown
    };

    TouchBand(Direction direction)
        : direction(direction)
    {
        setInterceptsMouseClicks(true, false);
    }

    ~TouchBand() override = default;

    //==============================================================
//    void paint(juce::Graphics& g) override
//    {
//        auto bounds = getLocalBounds().toFloat();
//
//        // Reference line
//        g.setColour(juce::Colours::grey.withAlpha(0.6f));
//
//        if (direction == Direction::UpOnly)
//        {
//            const float y = bounds.getBottom() - 10.0f;
//
//            g.fillRect(
//                bounds.getCentreX() - 1.0f,
//                y,
//                2.0f,
//                1.0f);
//
//            // Up arrow
//            juce::Path arrow;
//            arrow.startNewSubPath(
//                bounds.getCentreX() - 5.0f, y - 2.0f);
//            arrow.lineTo(
//                bounds.getCentreX(), y - 7.0f);
//            arrow.lineTo(
//                bounds.getCentreX() + 5.0f, y - 2.0f);
//
//            g.strokePath(
//                arrow,
//                juce::PathStrokeType(1.5f));
//        }
//        else
//        {
//            const float y = bounds.getCentreY();
//
//            g.fillRect(
//                bounds.getCentreX() - 1.0f,
//                y,
//                2.0f,
//                1.0f);
//
//            // Up arrow
//            juce::Path up;
//            up.startNewSubPath(
//                bounds.getCentreX() - 5.0f, y - 5.0f);
//            up.lineTo(
//                bounds.getCentreX(), y - 10.0f);
//            up.lineTo(
//                bounds.getCentreX() + 5.0f, y - 5.0f);
//
//            // Down arrow
//            juce::Path down;
//            down.startNewSubPath(
//                bounds.getCentreX() - 5.0f, y + 5.0f);
//            down.lineTo(
//                bounds.getCentreX(), y + 10.0f);
//            down.lineTo(
//                bounds.getCentreX() + 5.0f, y + 5.0f);
//
//            g.strokePath(
//                up,
//                juce::PathStrokeType(1.5f));
//
//            g.strokePath(
//                down,
//                juce::PathStrokeType(1.5f));
//        }
//    }
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        const auto centreX = bounds.getCentreX();

        //==============================================================
        // Colours
        //==============================================================

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

        const auto arrowColour =
            isActive
                ? VisualStyle::getStateColor(
                                             colourSet,
                                             isActive,
                                             true).withAlpha(0.75f)
                : VisualStyle::getStateColor(
                                             colourSet,
                                             isActive,
                                             true).withAlpha(0.25f);

        const auto fillColour =
            juce::Colours::white.withAlpha(
                isActive ? 0.18f : 0.0f);

        //==============================================================
        // Background
        //==============================================================

        g.setColour(backgroundColour);

        g.fillRoundedRectangle(
            bounds.reduced(1.0f),
            4.0f);

        //==============================================================
        // Interaction fill
        //==============================================================

        if (isActive && isDragging)
        {
            if (direction == Direction::UpOnly)
            {
                const float amount =
                    juce::jlimit(0.0f, 1.0f,
                                 std::abs(gestureValue));

                const float bottom = bounds.getBottom();
                const float top =
                    bottom - amount * bounds.getHeight();

                g.setColour(fillColour);

                g.fillRoundedRectangle(
                    centreX - 6.0f,
                    top,
                    12.0f,
                    bottom - top,
                    4.0f);
            }
            else
            {
                const float amount =
                    juce::jlimit(-1.0f, 1.0f,
                                 gestureValue);

                const float centreY =
                    bounds.getCentreY();

                g.setColour(fillColour);

                if (amount > 0.0f)
                {
                    const float height =
                        amount * (centreY - bounds.getY());

                    g.fillRoundedRectangle(
                        centreX - 6.0f,
                        centreY - height,
                        12.0f,
                        height,
                        4.0f);
                }
                else if (amount < 0.0f)
                {
                    const float height =
                        -amount *
                        (bounds.getBottom() - centreY);

                    g.fillRoundedRectangle(
                        centreX - 6.0f,
                        centreY,
                        12.0f,
                        height,
                        4.0f);
                }
            }
        }

        //==============================================================
        // Reference line
        //==============================================================

        const float lineY =
            direction == Direction::UpOnly
                ? bounds.getBottom() - 10.0f
                : bounds.getCentreY();

        g.setColour(borderColour);

        g.fillRect(
            centreX - 1.0f,
            lineY,
            2.0f,
            1.5f);

        //==============================================================
        // Arrows
        //==============================================================

        g.setColour(arrowColour);

        if (direction == Direction::UpOnly)
        {
            juce::Path arrow;

            arrow.startNewSubPath(
                centreX - 5.0f, lineY - 2.0f);

            arrow.lineTo(
                centreX, lineY - 7.0f);

            arrow.lineTo(
                centreX + 5.0f, lineY - 2.0f);

            g.strokePath(
                arrow,
                juce::PathStrokeType(1.5f));
        }
        else
        {
            juce::Path up;

            up.startNewSubPath(
                centreX - 5.0f, lineY - 5.0f);

            up.lineTo(
                centreX, lineY - 10.0f);

            up.lineTo(
                centreX + 5.0f, lineY - 5.0f);

            juce::Path down;

            down.startNewSubPath(
                centreX - 5.0f, lineY + 5.0f);

            down.lineTo(
                centreX, lineY + 10.0f);

            down.lineTo(
                centreX + 5.0f, lineY + 5.0f);

            g.strokePath(
                up,
                juce::PathStrokeType(1.5f));

            g.strokePath(
                down,
                juce::PathStrokeType(1.5f));
        }

        //==============================================================
        // Active frame
        //==============================================================

        if (isActive)
        {
            g.setColour(
                borderColour);

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

        touchStartPosition = event.position;
        lastPosition = event.position;

        isDragging = true;
        gestureValue = 0.0f;
        
        if (onGestureStart)
            onGestureStart();

        repaint();
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
//        if (!isActive)
//            return;
        
        const float displacement =
            touchStartPosition.y - event.position.y;

        float normalized = 0.0f;
        
        if (std::abs(normalized) > 0.0f)
            isDragging = true;
        
        if (direction == Direction::UpOnly)
        {
            normalized = juce::jmax(
                0.0f,
                displacement / getHeight());
        }
        else
        {
            normalized =
                displacement / (getHeight() * 0.5f);
        }

        normalized = juce::jlimit(-1.0f, 1.0f, normalized);

        // Dead zone
        constexpr float deadZone = 0.08f;

        if (std::abs(normalized) < deadZone)
            normalized = 0.0f;
        else
        {
            const float sign =
                normalized >= 0.0f ? 1.0f : -1.0f;

            normalized =
                sign * (std::abs(normalized) - deadZone)
                / (1.0f - deadZone);
        }
        
        const float visualValue = normalized;

        // Non-linear response
        const float response =
            std::copysign(
                std::pow(std::abs(normalized), 2.0f),
                normalized);
        
        gestureValue = visualValue;

        if (onValueChanged)
            onValueChanged(response);
        
        repaint();

        lastPosition = event.position;
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        if (!isActive)
            return;

        isDragging = false;
        gestureValue = 0.0f;
        
        if (onGestureEnd)
            onGestureEnd();

        repaint();
    }

    //==============================================================
    std::function<void(float)> onValueChanged;
    std::function<void()> onGestureStart;
    std::function<void()> onGestureEnd;
    
    // state functions
    void setActive(bool active)
    {
        isActive = active;

        if (!isActive)
        {
            isDragging = false;
            gestureValue = 0.0f;
        }

        repaint();
    }

    bool getIsActive() const
    {
        return isActive;
    }

private:
    Direction direction;

    juce::Point<float> touchStartPosition;
    juce::Point<float> lastPosition;
    
    bool isActive = true;
    bool isDragging = false;
    float gestureValue = 0.0f;
    
    const VisualStyle::ColorSet& colourSet = VisualStyle::Palette::cyan;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TouchBand)
};
