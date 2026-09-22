/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "State/EQState.h"
#include "DSP/EQConstants.h"
#include "DSP/FeedbackDelay.h"
#include "DSP/GraphicEQ.h"
#include "DSP/SpectrumAnalyzer.h"
#include "DSP/SpectrumDataBuffer.h"
#include <array>

//==============================================================================
/**
*/
class EchoShapeAudioProcessor  : public juce::AudioProcessor,
                                    public juce::AudioProcessorValueTreeState::Listener,
                                    public juce::ChangeBroadcaster
{
public:
    //==============================================================================
    EchoShapeAudioProcessor();
    ~EchoShapeAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    void reset() override;
    
    juce::AudioProcessorValueTreeState parameters;
    
    // EQ DSP-related functions
    void setGain(std::size_t band, float gainDb);
    void setGains(const GraphicEQ::Gains& gains);
    float getGain(std::size_t band) const;
    GraphicEQ::Gains getGains() const;
    EQState getEQState() const;
    void setEQState(const EQState& state);
    
    void setFreeze(bool freeze) noexcept;
    //==============================================================================
    SpectrumDataBuffer<
        SpectrumAnalyzer::numSpectrumBands>&
    getSpectrumData() noexcept
    {
        return spectrumData;
    }

private:
    //==============================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void parameterChanged(const juce::String& id, float newValue) override;
    
    // EQ DSP-related
    // for saving/loading state
    juce::ValueTree createEQState() const;
    bool restoreEQState(const juce::ValueTree& state);
    // variables
    static constexpr std::size_t NumChannels = 2;
    std::array<FeedbackDelay, NumChannels> feedbackDelay_;
    EQState eqState;
    
    SpectrumAnalyzer inputSpectrumAnalyzer;
    SpectrumAnalyzer outputSpectrumAnalyzer;
    
    SpectrumDataBuffer<
        SpectrumAnalyzer::numSpectrumBands>
        spectrumData;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EchoShapeAudioProcessor)
};
