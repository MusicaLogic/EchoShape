/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
EchoShapeAudioProcessor::EchoShapeAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
#endif
{
}

EchoShapeAudioProcessor::~EchoShapeAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
EchoShapeAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"time",1},
        "Time",
        juce::NormalisableRange<float>(0.01f, 2.0f, 0.01f),
        0.25f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"feedback",1},
        "Feedback",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"wet",1},
        "Wet",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01),
        0.5f));

    return { params.begin(), params.end() };
}

//==============================================================================
const juce::String EchoShapeAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool EchoShapeAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool EchoShapeAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool EchoShapeAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double EchoShapeAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int EchoShapeAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int EchoShapeAudioProcessor::getCurrentProgram()
{
    return 0;
}

void EchoShapeAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String EchoShapeAudioProcessor::getProgramName (int index)
{
    return {};
}

void EchoShapeAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void EchoShapeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    // EQ DSP-related
    for (auto& fd : feedbackDelay_)
        fd.prepare(sampleRate, EQConstants::frequencies);
    
    inputSpectrumAnalyzer.setSampleRate(sampleRate);
    outputSpectrumAnalyzer.setSampleRate(sampleRate);

    inputSpectrumAnalyzer.reset();
    outputSpectrumAnalyzer.reset();
    
    // Load parameters from value tree
    float time = *parameters.getRawParameterValue("time");
    float feedback = *parameters.getRawParameterValue("feedback");
    float wet = *parameters.getRawParameterValue("wet");
    
    for (auto& fd : feedbackDelay_){
        fd.setDelayTime(time);
        fd.setFeedback(feedback);
        fd.setWet(wet);
    }
    
    parameters.addParameterListener("time", this);
    parameters.addParameterListener("feedback", this);
    parameters.addParameterListener("wet", this);
}

void EchoShapeAudioProcessor::parameterChanged(const juce::String& id, float newValue)
{
    if(id == "time"){
        for (auto& fd : feedbackDelay_) fd.setDelayTime(newValue);
    }else if(id == "feedback"){
        for (auto& fd : feedbackDelay_) fd.setFeedback(newValue);
    }else if(id == "wet"){
        for (auto& fd : feedbackDelay_) fd.setWet(newValue);
    }
}

void EchoShapeAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool EchoShapeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void EchoShapeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    
    const auto numSamples =
            static_cast<std::size_t>(buffer.getNumSamples());
    const auto numChannels =
            std::min(
                static_cast<std::size_t>(buffer.getNumChannels()),
                NumChannels);
    
    // TODO: analyze sum of left and right channels
    // TODO: both in input and output
    
    // analyze input spectrum before processing
    inputSpectrumAnalyzer.processBlock(
        buffer.getReadPointer(0),
        buffer.getNumSamples());
    
    for (int channel = 0; channel < numChannels; ++channel)
    {
        feedbackDelay_[channel].process(
                    buffer.getWritePointer(
                        static_cast<int>(channel)),
                    numSamples);
    }
    
    // analyze output spectrum after processing
    outputSpectrumAnalyzer.processBlock(
        buffer.getReadPointer(0),
        buffer.getNumSamples());
    
    // ============================================================
    // MAKE LATEST DATA AVAILABLE TO GUI
    // ============================================================

    spectrumData.publishInput(
        inputSpectrumAnalyzer.getSpectrum());

    spectrumData.publishOutput(
        outputSpectrumAnalyzer.getSpectrum());
}

//==============================================================================
bool EchoShapeAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* EchoShapeAudioProcessor::createEditor()
{
    return new EchoShapeAudioProcessorEditor (*this);
}

void EchoShapeAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    juce::ValueTree state("EchoShapeState");

    state.setProperty(
        "version",
        1,
        nullptr);

    // ============================================================
    // EQ state
    // ============================================================
    
    state.addChild(
        createEQState(),
        -1,
        nullptr);
    
    // ============================================================
    // Effect parameters
    // ============================================================
    
    state.addChild(
        parameters.copyState(),
        -1,
        nullptr);
    
    // ============================================================
    // Serialize
    // ============================================================

    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void EchoShapeAudioProcessor::setStateInformation(
    const void* data,
    int sizeInBytes)
{
    auto xml = getXmlFromBinary(
        data,
        sizeInBytes);

    if (xml == nullptr)
        return;

    if (!xml->hasTagName("EchoShapeState"))
        return;

    juce::ValueTree state =
        juce::ValueTree::fromXml(*xml);

    if (!state.isValid())
        return;

    // ============================================================
    // Version
    // ============================================================

    const int version =
        state.getProperty("version", 1);

    // Future migration code can go here.
    juce::ignoreUnused(version);
    
    // ============================================================
    // Effect parameters
    // ============================================================

    auto parameterState =
        state.getChildWithName(
            parameters.state.getType());

    if (parameterState.isValid())
        parameters.replaceState(parameterState);

    // ============================================================
    // EQ state
    // ============================================================

    auto eqState =
        state.getChildWithName("EQState");

    if (eqState.isValid())
        restoreEQState(eqState);
}

juce::ValueTree EchoShapeAudioProcessor::createEQState() const
{
    juce::ValueTree state("EQState");

    for (std::size_t i = 0;
         i < EQState::NumBands;
         ++i)
    {
        state.setProperty(
            "gain_" + juce::String(i),
            eqState.gains[i],
            nullptr);
    }

    return state;
}

bool EchoShapeAudioProcessor::restoreEQState(
    const juce::ValueTree& state)
{
    if (!state.isValid() ||
        !state.hasType("EQState"))
        return false;

    EQState newState;

    for (std::size_t i = 0;
         i < EQState::NumBands;
         ++i)
    {
        newState.gains[i] =
            static_cast<float>(
                state.getProperty(
                    "gain_" + juce::String(i),
                    0.0f));
    }

    setEQState(newState);

    return true;
}

void EchoShapeAudioProcessor::reset()
{
    for (auto& fd : feedbackDelay_)
        fd.reset();
}

// EQ DSP-related functions
void EchoShapeAudioProcessor::setFreeze(bool freeze) noexcept
{
    for (auto& fd : feedbackDelay_)
        fd.setFreeze(freeze);
}

void EchoShapeAudioProcessor::setGain(std::size_t band, float gainDb){
    for (auto& fd : feedbackDelay_)
        fd.setEQGain(band, gainDb);
}
void EchoShapeAudioProcessor::setGains(const GraphicEQ::Gains& gains){
    for (std::size_t i = 0;
         i < EQState::NumBands;
         ++i)
    {
        eqState.gains[i] = gains[i];
    }
    for (auto& fd : feedbackDelay_)
        fd.setEQGains(gains);
}
float EchoShapeAudioProcessor::getGain(std::size_t band) const{
    if (band >= GraphicEQ::NumBands)
        return 0.0f;

    return feedbackDelay_[0].getEQGain(band);
}

GraphicEQ::Gains EchoShapeAudioProcessor::getGains() const
{
    GraphicEQ::Gains gains{};

    for (std::size_t i = 0;
         i < GraphicEQ::NumBands;
         ++i)
    {
        gains[i] = eqState.gains[i];
    }

    return gains;
}

EQState EchoShapeAudioProcessor::getEQState() const
{
    return eqState;
}

void EchoShapeAudioProcessor::setEQState(
    const EQState& state)
{
    eqState = state;

    for (auto& fd : feedbackDelay_)
        fd.setEQGains(eqState.gains);
    
    // communicate to editor
    sendChangeMessage();
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EchoShapeAudioProcessor();
}
