#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

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
    juce::AudioProcessorValueTreeState apvts;

private:

    // Defining function that makes parameters.
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    float getParameterValue (juce::String parameterID);
    //==============================================================================
    // Declare and initialize an int variable that will be used to hold the position of the buffer.
    int delayBufferPos = 0;
    int time;

    // Declare an object of class "Audio Buffer", that can store Audio Data.
    // Float parameter specifies that the type of stored data will be floating-point numbers.
    juce::AudioBuffer<float> delayBuffer;
    juce::AudioBuffer<float> bufferWet;

    // The writePosition is the point along the channel data we are. Each sample the writer position goes through gets
    // temporarily placed in the circular buffer
    int writePosition { 0 };

    void fillBuffer(juce::AudioBuffer<float>& buffer, int channel, float* channelData, int bufferSize, int delayBufferSize);
    void fillBufferAdd(juce::AudioBuffer<float>& buffer, int channel, float* channelData, int bufferSize, int delayBufferSize);
    void readFromBuffer(juce::AudioBuffer<float>& buffer, juce::AudioBuffer<float>& delayBuffer, int channel, int bufferSize, int delayBufferSize);
    void copyFromWet(juce::AudioBuffer<float>& buffer, juce::AudioBuffer<float>& delayBuffer, int channel, int bufferSize);
    void updateBufferPositions(juce::AudioBuffer<float>& buffer, juce::AudioBuffer<float>& delayBuffer);
    juce::LinearSmoothedValue<float> length { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};