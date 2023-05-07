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
    auto delayBufferSize = sampleRate * 2.0;


    delayBuffer.setSize(getTotalNumOutputChannels(), (int) delayBufferSize);
    bufferWet.setSize(getTotalNumOutputChannels(), (int) 512);

    length.reset (sampleRate, 0.001);
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

    // Calling parameter values
    auto gain = getParameterValue("gain");
    auto phase = getParameterValue("phase");
    auto mix = getParameterValue("mix");
    auto toggle = getParameterValue("toggle");

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto bufferSize = buffer.getNumSamples();

    // Popped this here to see if the bufferWet
    auto bufferSizeWet = bufferWet.getNumSamples();
    auto delayBufferSize = delayBuffer.getNumSamples();

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        // Toggle off
        if (toggle == 0)
        {
            continue;
        }

            // Toggle on
        else
        {
            // returns a pointer to the start of the memory block that holds audio samples for the channel.
            auto* channelData = buffer.getWritePointer(channel);

            // channelData for the wet buffer, this is used to fill back the delaybuffer
            auto* channelDataWet = bufferWet.getWritePointer(channel);

            // Function used to feed main buffer into delayBuffer
            fillBuffer(buffer, channel, channelData, bufferSize, delayBufferSize);

            // Reading from delayBuffer in the past and adding to bufferWet
            readFromBuffer(bufferWet, delayBuffer, channel, bufferSize, delayBufferSize);

            // Feeding combined bufferWet + delayBuffer to delayBuffer.
            fillBufferAdd(buffer, channel, channelDataWet, bufferSize, delayBufferSize);

            // applying gain to the buffer and using a logorithmic scale for the gain.
            buffer.applyGain(juce::Decibels::decibelsToGain(gain));

            // Reading from bufferWet
            copyFromWet(buffer, bufferWet, channel, bufferSize);
        }
    }
    updateBufferPositions (buffer, delayBuffer);
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
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

float AudioPluginAudioProcessor::getParameterValue(juce::String parameterID)
{
    auto atomicFloat =apvts.getRawParameterValue(parameterID);
    auto floatValue = atomicFloat -> load();
    return floatValue;
}


// This is where the circular buffer is made.
// The circular buffer is a stored memory block that also contains the incoming audio
void AudioPluginAudioProcessor::fillBuffer(juce::AudioBuffer<float>& buffer, int channel, float* channelData, int bufferSize, int delayBufferSize)
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

        // Copy remaining amount to beginning of delay buffer. Need to state in index that we are reading from numSamplesToEnd.
        delayBuffer.copyFrom(channel, 0, buffer.getWritePointer(channel, numSamplesToEnd), numSamplesAtStart);
    }
}

// This is where the circular buffer is made.
// The circular buffer is a stored memory block that also contains the incoming audio
void AudioPluginAudioProcessor::fillBufferAdd(juce::AudioBuffer<float>& buffer, int channel, float* channelData, int bufferSize, int delayBufferSize)
{
    // Check to see if main buffer copies to delay buffer without needing to wrap
    if (delayBufferSize > bufferSize + writePosition)
    {
        // Copy main buffer contents to delay buffer
        delayBuffer.addFrom(channel, writePosition, channelData, bufferSize);
    }
        // if no
    else
    {
        // Determine how much space is left at the end of the delay buffer
        auto numSamplesToEnd = delayBufferSize - writePosition;

        // Copy that amount of contents to the end...
        delayBuffer.addFrom(channel, writePosition, channelData, numSamplesToEnd);

        // Calculate how much contents is remaining to copy
        auto numSamplesAtStart = bufferSize - numSamplesToEnd;

        // Copy remaining amount to beginning of delay buffer. Need to state in index that we are reading from numSamplesToEnd.
        delayBuffer.addFrom(channel, 0, buffer.getWritePointer(channel, numSamplesToEnd), numSamplesAtStart);
    }
}

void AudioPluginAudioProcessor::readFromBuffer(juce::AudioBuffer<float>& buffer, juce::AudioBuffer<float>& delayBuffer, int channel, int bufferSize, int delayBufferSize)
{
    auto l = getParameterValue("length");
    length.setTargetValue(l);
    auto lengthValue = length.getNextValue();
    auto feedback = getParameterValue("feedback");

    // delayMs
    auto readPosition = (writePosition - getSampleRate() * pow(10, -3) * lengthValue);

    // when readposition is less than 0 it sets to the correct point near the end of the buffer.
    if (readPosition < 0)
    {
        readPosition += delayBufferSize;
    }
    // feedback
    auto g = feedback;

    //If readposition does not need to wrap around
    if (readPosition + bufferSize < delayBufferSize)
    {
        buffer.addFromWithRamp(channel, 0, delayBuffer.getReadPointer(channel, readPosition), bufferSize, g, g);
    }

    else
    {
        // adding samples at the end
        auto numSamplesToEnd = delayBufferSize - readPosition;
        buffer.addFromWithRamp(channel, 0, delayBuffer.getReadPointer(channel, readPosition), numSamplesToEnd, g, g);

        // adding samples to the beginning
        auto numSamplesAtStart = bufferSize - numSamplesToEnd;
        buffer.addFromWithRamp(channel, numSamplesToEnd, delayBuffer.getReadPointer(channel, 0), numSamplesAtStart, g, g);
    }
}


// This is used to replace buffer with bufferWet
void AudioPluginAudioProcessor::copyFromWet(juce::AudioBuffer<float>& buffer, juce::AudioBuffer<float>& delayBuffer, int channel, int bufferSize)
{
    //i made this 0 because i presume buffer and bufferWet will always be in sync because they got the same size
    buffer.copyFrom(channel, 0, bufferWet.getReadPointer(channel), bufferSize);
}

void AudioPluginAudioProcessor::updateBufferPositions(juce::AudioBuffer<float>& buffer, juce::AudioBuffer<float>& delayBuffer)
{
    auto bufferSize = buffer.getNumSamples();
    auto delayBufferSize = delayBuffer.getNumSamples();

    // Adds the writePosition + 1 buffer size.
    // This is where the writePosition increments.
    writePosition += bufferSize;

    // Allows it to wrap once the writePosition is about to become greater than the delayBufferSize.
    writePosition %= delayBufferSize;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}

// Implementing function that makes the parameters.
juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameters()
{
    // Storing parameters as a vector
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("gain","Gain", -96.0f, 48.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("feedback", "Delay Feedback", 0.0f, 1.0f, 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("mix","Dry / Wet", 0.01f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("length","Delay Time", 20.0f, 2000.0f, 500.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"phase", 1}, "Phase", 0));
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"toggle", 1}, "On/Off", 1));

    return{ params.begin(), params.end() };
}