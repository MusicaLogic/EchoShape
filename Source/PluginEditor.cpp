/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//namespace
//{
//    //==========================================================================
//    // Small LookAndFeel wrapper so the prototype uses the shared VisualStyle
//    // typography and cyan palette consistently.
//
//    class EchoShapeLookAndFeel : public juce::LookAndFeel_V4
//    {
//    public:
//        EchoShapeLookAndFeel()
//        {
//            setColour (juce::Slider::rotarySliderFillColourId,
//                       VisualStyle::Palette::cyan.highlight);
//
//            setColour (juce::Slider::rotarySliderOutlineColourId,
//                       VisualStyle::Palette::cyan.dim);
//
//            setColour (juce::Slider::textBoxTextColourId,
//                       VisualStyle::Palette::cyan.highlight);
//
//            setColour (juce::Slider::textBoxOutlineColourId,
//                       juce::Colours::transparentBlack);
//
//            setColour (juce::TextButton::textColourOffId,
//                       VisualStyle::Palette::cyan.highlight);
//
//            setColour (juce::TextButton::textColourOnId,
//                       VisualStyle::background);
//        }
//
//        juce::Font getTextButtonFont (juce::TextButton&,
//                                       int buttonHeight) override
//        {
//            return VisualStyle::getDefaultFont (
//                juce::jmin (VisualStyle::FontSize::medium,
//                            buttonHeight * 0.42f));
//        }
//
//        juce::Font getSliderPopupFont (juce::Slider&) override
//        {
//            return VisualStyle::getDefaultFont (
//                VisualStyle::FontSize::normal);
//        }
//    };
//
//    EchoShapeLookAndFeel sharedLookAndFeel;
//}

//==============================================================================

// Temporary stand-in for the real ParaGraphiQ editor.
class EchoShapeAudioProcessorEditor::EQPlaceholderComponent
    : public juce::Component
{
public:

    std::function<void()> onToggle;
    
    EQPlaceholderComponent(EchoShapeAudioProcessor& processor):
                                        processor(processor),
                                        paraGraphiQ(processor)
    {
        addAndMakeVisible(paraGraphiQ);
        addAndMakeVisible (eqTag);

        eqTag.onClick = [this]
        {
            if (onToggle != nullptr)
                onToggle();
        };
    }
    
    void setEQVisible (bool visible)
    {
        paraGraphiQ.setVisible (visible);
    }
    
    void resized() override
    {
        paraGraphiQ.setBounds(getLocalBounds());
        constexpr int tagWidth  = 40;
        constexpr int tagHeight = 28;

        eqTag.setBounds (
            10,
            getHeight() - tagHeight - 8,
            tagWidth,
            tagHeight);
    }

    void paint (juce::Graphics& g) override
    {
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

    void mouseDown (const juce::MouseEvent& e) override
    {
        auto tag = getLocalBounds()
                     .removeFromRight (54)
                     .removeFromTop (32);

        if (tag.contains (e.getPosition()) && onToggle != nullptr)
            onToggle();
    }
private:
    class EQTagComponent : public juce::Component
    {
    public:

        std::function<void()> onClick;

        EQTagComponent()
        {
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
        }

        void paint (juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat();

            // Outer cyan border
            g.setColour (VisualStyle::Palette::cyan.highlight);

            g.fillRoundedRectangle (
                bounds,
                VisualStyle::Geometry::buttonCornerRadius);

            // Inner background
            g.setColour (VisualStyle::panelBackground);

            g.fillRoundedRectangle (
                bounds.reduced (1.0f),
                VisualStyle::Geometry::buttonCornerRadius);

            // Text
            g.setColour (VisualStyle::Palette::cyan.highlight);

            g.setFont (
                VisualStyle::getDefaultFont (
                    VisualStyle::FontSize::normal));

            g.drawText (
                "EQ",
                getLocalBounds(),
                juce::Justification::centred);
        }

        void mouseDown (const juce::MouseEvent&) override
        {
            if (onClick != nullptr)
                onClick();
        }

    private:

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (
            EQTagComponent)
    };
    
    EchoShapeAudioProcessor& processor;
    ParaGraphiQComponent paraGraphiQ;
    EQTagComponent eqTag;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (
            EQPlaceholderComponent)
};

//==============================================================================
//==============================================================================
EchoShapeAudioProcessorEditor::EchoShapeAudioProcessorEditor (EchoShapeAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    
//    setLookAndFeel (&sharedLookAndFeel);
    
    //==========================================================================
    // Delay controls

    addAndMakeVisible (timeSlider);
    configureSlider (timeSlider, "Time");

    addAndMakeVisible (feedbackSlider);
    configureSlider (feedbackSlider, "Fdb");

    addAndMakeVisible (wetSlider);
    configureSlider (wetSlider, "Wet");

    timeAttach =
        std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.parameters,
                "time",
                timeSlider);

    feedbackAttach =
        std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.parameters,
                "feedback",
                feedbackSlider);

    wetAttach =
        std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.parameters,
                "wet",
                wetSlider);

    //==========================================================================
    // Tap tempo

    addAndMakeVisible (tapTempoButton);

    tapTempoButton.setClickingTogglesState (false);
    tapTempoButton.setColour (
        juce::TextButton::buttonColourId,
        VisualStyle::panelBackground);
    tapTempoButton.setColour (
        juce::TextButton::buttonOnColourId,
        VisualStyle::panelBackground);
    tapTempoButton.setColour (
        juce::TextButton::textColourOffId,
        VisualStyle::Palette::cyan.highlight);
    tapTempoButton.setColour (
        juce::TextButton::textColourOnId,
        VisualStyle::Palette::cyan.highlight);
    tapTempoButton.setColour (
        juce::ComboBox::outlineColourId,
        VisualStyle::Palette::cyan.dim);

    tapTempoButton.onClick =
        [this] { tapTempoPressed(); };

    //==========================================================================
    // Freeze

    addAndMakeVisible (freezeButton);
    freezeButton.setClickingTogglesState (false);

    freezeButton.setColour (
        juce::TextButton::buttonColourId,
        VisualStyle::panelBackground);
    freezeButton.setColour (
        juce::TextButton::buttonOnColourId,
        VisualStyle::Palette::cyan.highlight);
    freezeButton.setColour (
        juce::TextButton::textColourOffId,
        VisualStyle::Palette::cyan.highlight);
    freezeButton.setColour (
        juce::TextButton::textColourOnId,
        VisualStyle::background);
    freezeButton.setColour (
        juce::ComboBox::outlineColourId,
        VisualStyle::Palette::cyan.dim);

    freezeButton.onClick =
        [this] { toggleFreeze(); };

    //==========================================================================
    // ParaGraphiQ placeholder

    eqPanel =
        std::make_unique<EQPlaceholderComponent>(audioProcessor);

    eqPanel->onToggle =
        [this]
        {
            eqExpanded = ! eqExpanded;
            if (eqExpanded)
                    eqPanel->setEQVisible (true);
            startTimerHz (60);
        };

    addAndMakeVisible (*eqPanel);

    // Start collapsed.
    eqAnimationPosition = 0.0f;
    
    eqPanel->setEQVisible (eqAnimationPosition > 0.05f);

    startTimerHz (60);
    
    setSize (800, 400);
}

EchoShapeAudioProcessorEditor::~EchoShapeAudioProcessorEditor()
{
    stopTimer();
    // Restore exactly the value that was present before Freeze.
    feedbackSlider.setValue (
        feedbackBeforeFreeze,
        juce::sendNotificationSync);

    freezeActive = false;
    audioProcessor.setFreeze (freezeActive);
}

//==============================================================================
void EchoShapeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (VisualStyle::background);

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

void EchoShapeAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor.
//    auto area = getLocalBounds().reduced (24);
    auto area = getLocalBounds();
    
    // for the eq panel when collapsed
    const int collapsedHeight = 42;
    // Keep the upper controls above the EQ sheet.
    const int topRowY = area.getY() + 0;
//    const int rowHeight = 145;
    const int topRowHeight = (area.getHeight() - 2.0f*collapsedHeight)/3.0f;
    const int gap = 18;

    const int columnWidth =
        (area.getWidth() - 2 * gap) / 3;

    // Top row: WET | FEEDBACK | FREEZE
    wetSlider.setBounds (
        area.getX(),
        topRowY,
        columnWidth,
        topRowHeight);

    feedbackSlider.setBounds (
        area.getX() + columnWidth + gap,
        topRowY,
        columnWidth,
        topRowHeight);

    freezeButton.setBounds (
        area.getX() + 2 * (columnWidth + 3.0f*gap),
        topRowY + 0.25f*topRowHeight,
        columnWidth - 6.0f*gap, // columnWidth - 6.0f*gap,
                            0.5f*topRowHeight);

    // Bottom row: TIME | TAP
    const int bottomRowHeight = 2.0f*(area.getHeight() - 2.0f*collapsedHeight)/3.0f;
    const int bottomY = topRowY + topRowHeight + 24;
    const int bottomGap = 18;
    const int bottomWidth =
        (area.getWidth() - bottomGap) / 2;

    timeSlider.setBounds (
        area.getX(),
        bottomY,
        bottomWidth,
        bottomRowHeight);

    tapTempoButton.setBounds (
        area.getX() + bottomWidth + 8.0f*bottomGap,
        bottomY + 0.25f*bottomRowHeight,
        8.0f*bottomGap, // bottomWidth - 16.0f*bottomGap,
        0.5f*bottomRowHeight);

    //==========================================================================
    // Sliding EQ bottom sheet

    const int expandedHeight =
        juce::jlimit (
            220,
            getHeight() - 50,
            (int) (bottomRowHeight + 2.0f*collapsedHeight));
//            (int) (getHeight() * 0.58f));

    const float eased =
        eqAnimationPosition
        * eqAnimationPosition
        * (3.0f - 2.0f * eqAnimationPosition);

    const int panelHeight =
        juce::roundToInt (
            collapsedHeight
            + (expandedHeight - collapsedHeight) * eased);

    eqPanel->setBounds (
        0,
        getHeight() - panelHeight,
        getWidth(),
        panelHeight);
}

//==============================================================================

void EchoShapeAudioProcessorEditor::configureSlider (
    juce::Slider& s,
    const juce::String& suffix)
{
    s.setSliderStyle (juce::Slider::RotaryVerticalDrag);

    s.setTextBoxStyle (
        juce::Slider::TextBoxBelow,
        false,
        78,
        20);

    s.setColour (
        juce::Slider::textBoxTextColourId,
        VisualStyle::Palette::cyan.highlight);

    s.setColour (
        juce::Slider::textBoxOutlineColourId,
        juce::Colours::transparentBlack);

    s.setColour (
        juce::Slider::rotarySliderFillColourId,
        VisualStyle::Palette::cyan.highlight);

    s.setColour (
        juce::Slider::rotarySliderOutlineColourId,
        VisualStyle::Palette::cyan.dim);

    s.setColour (
        juce::Slider::thumbColourId,
        juce::Colours::transparentBlack);

    s.setTextValueSuffix (" " + suffix);
}

//==============================================================================
// Tap tempo

void EchoShapeAudioProcessorEditor::tapTempoPressed()
{
    const auto now =
        juce::Time::getMillisecondCounterHiRes() * 0.001;

    if (! tapTimes.empty())
    {
        const double interval = now - tapTimes.back();

        if (interval > 0.15 && interval < 2.5)
            tapTimes.push_back (now);
        else
            tapTimes = { now };
    }
    else
    {
        tapTimes.push_back (now);
    }

    while ((int) tapTimes.size() > maxTapCount)
        tapTimes.erase (tapTimes.begin());

    if (tapTimes.size() >= 2)
    {
        double intervalSum = 0.0;

        for (size_t i = 1; i < tapTimes.size(); ++i)
            intervalSum += tapTimes[i] - tapTimes[i - 1];

        const double averageInterval =
            intervalSum / static_cast<double> (tapTimes.size() - 1);

        const double seconds =
            juce::jlimit (0.01, 2.0, averageInterval);

        if (auto* parameter =
                audioProcessor.parameters.getParameter ("time"))
        {
            parameter->setValueNotifyingHost (
                parameter->convertTo0to1 (
                    static_cast<float> (seconds)));
        }
    }

    tapTempoButton.setButtonText ("T A P");

    juce::Timer::callAfterDelay (
        140,
        [this]
        {
            if (tapTempoButton.getButtonText() == "T A P")
                tapTempoButton.setButtonText ("TAP");
        });
}

//==============================================================================

void EchoShapeAudioProcessorEditor::toggleFreeze()
{
    if (! freezeActive)
    {
        // Store the value immediately before Freeze takes control.
        feedbackBeforeFreeze =
            static_cast<float> (feedbackSlider.getValue());

        feedbackSlider.setValue (
            1.0,
            juce::sendNotificationSync);

        freezeActive = true;
    }
    else
    {
        // Restore exactly the value that was present before Freeze.
        feedbackSlider.setValue (
            feedbackBeforeFreeze,
            juce::sendNotificationSync);

        freezeActive = false;
    }
    
    audioProcessor.setFreeze (freezeActive);

    freezeButton.setToggleState (
        freezeActive,
        juce::dontSendNotification);

    freezeButton.repaint();
}

//==============================================================================

void EchoShapeAudioProcessorEditor::timerCallback()
{
    const float target =
        eqExpanded ? 1.0f : 0.0f;

    const float difference =
        target - eqAnimationPosition;

    if (std::abs (difference) < 0.0025f)
    {
        eqAnimationPosition = target;
        if (!eqExpanded)
                eqPanel->setEQVisible (false);
        stopTimer();
    }
    else
    {
        eqAnimationPosition += difference * 0.20f;
    }

    resized();
    eqPanel->repaint();
}
