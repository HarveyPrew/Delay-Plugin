#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), apvts(*this, nullptr, "Parameters", createParameters())
{

}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    // Declare and initialise int variable that holds the "echo" length in msec.
    int delayMilliseconds = 400;
    time = 0;
    auto delayBufferSize = sampleRate * 2.0;
    delayBuffer.setSize(getTotalNumOutputChannels(), (int) delayBufferSize);

    // Calculates number of delay samples required for the given delay time.
    // Rounded to nearest int.
    // sampleRate parameter passed to prepareToPlay method by the JUCE framework,
    // when the audio processing system is initialised.
    //updateDelayBuffer(delayMilliseconds, sampleRate);
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();


    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());


    float gain = pointerToFloat("gain");
    float feedback = pointerToFloat("feedback");
    float mix = pointerToFloat("mix");
    float length = pointerToFloat("length");

    //auto delaySamples = (int) std::round (44100 * length / 1000.0);

    // Set size states number of channels and number of samples per channel in the buffer.
    //delayBuffer.setSize(2, delaySamples);


    auto bufferSize = buffer.getNumSamples();
    auto delayBufferSize = delayBuffer.getNumSamples();

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        // enables us to copy data from one buffer to another.
        auto* channelData = buffer.getWritePointer (channel);
        fillBuffer(channel, bufferSize, delayBufferSize, channelData);
        readFromBuffer(channel, bufferSize, delayBufferSize, buffer, delayBuffer, length);

    }

    writePosition += bufferSize;
    writePosition %= delayBufferSize;
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

float AudioPluginAudioProcessor::pointerToFloat(juce::String parameterID)
{
    auto atomicFloat =apvts.getRawParameterValue(parameterID);
    auto floatValue = atomicFloat -> load();
    return floatValue;
}

void AudioPluginAudioProcessor::updateDelayBuffer(int delayMilliseconds, int sampleRate) {
    auto delaySamples = (int) std::round (sampleRate * delayMilliseconds / 1000.0);
    // Set size states number of channels and number of samples per channel in the buffer.
    delayBuffer.setSize(getTotalNumOutputChannels(), delaySamples);

    // Cleans buffer from preexisting data.
    delayBuffer.clear();

    // Sets delaybufferpos to 0 each start up.
    delayBufferPos = 0;
}

void AudioPluginAudioProcessor::fillBuffer(int channel, int bufferSize, int delayBufferSize, float* channelData)
{
    // Check to see if main buffer copies to delay buffer without needing to wrap
    if (delayBufferSize > bufferSize + writePosition)
    {
        // Copy main buffer contents to delay buffer
        delayBuffer.copyFrom(channel, writePosition, channelData, bufferSize);
    }
        // if no
    else
    {
        // Determine how much space is left at the end of the delay buffer
        auto numSamplesToEnd = delayBufferSize - writePosition;

        // Copy that amount of contents to the end...
        delayBuffer.copyFrom(channel, writePosition, channelData, numSamplesToEnd);

        // Calculate how much contents is remaining to copy
        auto numSamplesAtStart = bufferSize - numSamplesToEnd;

        // Copy remaining amount to beginning of delay buffer
        delayBuffer.copyFrom(channel, 0, channelData + numSamplesToEnd, numSamplesAtStart);
    }
}

void AudioPluginAudioProcessor::readFromBuffer(int channel, int bufferSize, int delayBufferSize, juce::AudioBuffer<float>& buffer, juce::AudioBuffer<float>& delayBuffer, float& length)
{
    // 1 seconds of audio from in the past
    auto readPosition = writePosition - (getSampleRate() * length);

    if (readPosition < 0)
    {
        readPosition += delayBufferSize;
    }

    auto g = 0.7f;

    if (readPosition + bufferSize < delayBufferSize)
    {
        buffer.addFromWithRamp(channel, 0, delayBuffer.getReadPointer(channel, readPosition), bufferSize, g, g);
    }

    else
    {
        auto numSamplesToEnd = delayBufferSize - readPosition;
        buffer.addFromWithRamp(channel, 0, delayBuffer.getReadPointer(channel, readPosition), numSamplesToEnd, g, g);

        auto numSamplesAtStart = bufferSize - numSamplesToEnd;
        buffer.addFromWithRamp(channel, numSamplesToEnd, delayBuffer.getReadPointer(channel, 0), numSamplesAtStart, g, g);
    }
}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}

// Declaring function that makes parameters
juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameters()
{
    // Storing parameters as a vector
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("gain","Gain", 0.0f, 1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("feedback", "Delay Feedback", 0.0f, 1.0f, 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("mix","Dry / Wet", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("length","Delay Time", 0.01f, 2.0f, 1.0f));

    return{ params.begin(), params.end() };
}