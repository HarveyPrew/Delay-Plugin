#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

// Inherting from the listener is done here.
//==============================================================================
class AudioPluginAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

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

    //An object of APVTS is made to store the parameters of the plugin
    juce::AudioProcessorValueTreeState treeState;

private:

    // 192000 Is the highest possible sample rate.
    static constexpr auto effectDelaySamples = 192000;

    //dsp module, delayLine is used to handle the delay signals.
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayModule { effectDelaySamples };

    // TODO Find out what this does
    std::array<float, 2> delayValue { {} };
    void delayProcess(juce::AudioBuffer<float>& buffer,size_t channel, int numChannels);

    // Function is used to create the parameter layout, parameter attributes are stored here.
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();


    void addingSamplesToOutputAndDelayModule(size_t channel, const float* samplesIn,float* samplesOut,
                                             size_t sample);

    //Function is called to add parameter into treeState vector.
    std::unique_ptr<juce::AudioParameterFloat> floatParameterAsPointer(juce::String id, juce::String name,
                                                                       float minValue, float maxValue,
                                                                       float defaultValue);

    //Function is called to add parameter into treeState vector.
    std::unique_ptr<juce::AudioParameterBool> boolParameterAsPointer(juce::String id, juce::String name, float defaultValue);

    void addFloatParameterPointerToVector(std::vector <std::unique_ptr<juce::RangedAudioParameter>>& params,
                                          std::unique_ptr<juce::AudioParameterFloat>& parameter);

    void addBoolParameterPointerToVector(std::vector <std::unique_ptr<juce::RangedAudioParameter>>& params,
                                          std::unique_ptr<juce::AudioParameterBool>& parameter);

    void levelOfOutput(float* samplesOut, size_t sample, float input, float mix, float delayOutput, float gain,
                       float phase);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};