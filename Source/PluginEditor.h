/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Style/VisualStyle.h"
#include "UI/ParaGraphiQComponent.h"

//==============================================================================
/**
*/
class EchoShapeAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    EchoShapeAudioProcessorEditor (EchoShapeAudioProcessor&);
    ~EchoShapeAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    EchoShapeAudioProcessor& audioProcessor;
    
    void configureSlider (juce::Slider&, const juce::String& suffix);
    void timerCallback() override;

    void tapTempoPressed();
    void toggleFreeze();

    // Delay controls
    juce::Slider timeSlider;
    juce::Slider feedbackSlider;
    juce::Slider wetSlider;

    // Buttons
    juce::TextButton tapTempoButton { "TAP" };
    juce::TextButton freezeButton   { "FREEZE" };

    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> timeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> feedbackAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> wetAttach;

    // Temporary UI-only state
    bool freezeActive = false;
    float feedbackBeforeFreeze = 0.0f;

    // Tap-tempo state
    std::vector<double> tapTimes;
    static constexpr int maxTapCount = 6;

    // Temporary ParaGraphiQ stand-in
    class EQPlaceholderComponent;
    std::unique_ptr<EQPlaceholderComponent> eqPanel;

    bool eqExpanded = false;
    float eqAnimationPosition = 0.0f;   // 0 = collapsed, 1 = expanded

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EchoShapeAudioProcessorEditor)
};
