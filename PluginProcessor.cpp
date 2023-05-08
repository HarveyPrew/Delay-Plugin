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
), treeState(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    treeState.addParameterListener("delay", this);
    treeState.addParameterListener("mix", this);
    treeState.addParameterListener("feedback", this);
    treeState.addParameterListener("gain", this);
    treeState.addParameterListener("phase", this);
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
    treeState.removeParameterListener("delay", this);
    treeState.removeParameterListener("mix", this);
    treeState.removeParameterListener("feedback", this);
    treeState.removeParameterListener("gain", this);
    treeState.removeParameterListener("phase", this);
}

juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    std::vector <std::unique_ptr<juce::RangedAudioParameter>> params;

    auto pDelay = std::make_unique<juce::AudioParameterFloat>("delay", "Delay", 0.01f, 1000.0f, 500.0f);
    auto mix = std::make_unique<juce::AudioParameterFloat>("mix", "Mix", 0.0f, 100.0f, 50.0f);
    auto feedback = std::make_unique<juce::AudioParameterFloat>("feedback", "Feedback", 0.0f, 100.0f, 50.0f);
    auto gain = std::make_unique<juce::AudioParameterFloat>("gain", "Gain", 0.0f, 1.0f, 0.5f);
    auto toggle = std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"toggle", 1}, "Toggle", 1);
    auto phase = std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"phase", 1}, "Phase", 1);

    params.push_back(std::move(pDelay));
    params.push_back(std::move(mix));
    params.push_back(std::move(feedback));
    params.push_back(std::move(gain));
    params.push_back(std::move(toggle));
    params.push_back(std::move(phase));

    return { params.begin(), params.end() };
}

void AudioPluginAudioProcessor::parameterChanged(const juce::String &parameterID, float newValue)
{
    delayModule.setDelay(newValue / 1000.0f * getSampleRate());
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
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Initialize spec for dsp modules
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = sampleRate;
    spec.numChannels = getTotalNumOutputChannels();

    delayModule.prepare(spec);
    delayModule.reset();
    std::fill (lastDelayOutput.begin(), lastDelayOutput.end(), 0.0f);
    delayModule.setDelay(treeState.getRawParameterValue("delay")->load() / 1000.0f * getSampleRate());
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

    const auto numChannels = juce::jmax (totalNumInputChannels, totalNumOutputChannels);

    auto mix = treeState.getRawParameterValue("mix")->load()*0.01;
    auto feedback = treeState.getRawParameterValue("feedback")->load()*0.01;
    auto gain = treeState.getRawParameterValue("gain")->load();
    auto toggle = treeState.getRawParameterValue("toggle")->load();

    // Resetting the delay time
    delayModule.setDelay(treeState.getRawParameterValue("delay")->load() / 1000.0f * getSampleRate());

    for (size_t channel = 0; channel < numChannels; ++channel)
    {
        if (toggle == 0)
        {
            continue;
        }
        else
        {
            delayProcess(buffer, channel, numChannels, feedback, mix, gain);
        }
    }
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

void AudioPluginAudioProcessor::delayProcess(juce::AudioBuffer<float>& buffer,size_t channel, int numChannels, float feedback,
                                             float mix, float gain)
{
    auto audioBlock = juce::dsp::AudioBlock<float> (buffer).getSubsetChannelBlock (0, (size_t) numChannels);
    auto context = juce::dsp::ProcessContextReplacing<float>(audioBlock);
    auto& input = context.getInputBlock();
    auto& output = context.getOutputBlock();
    auto* samplesIn = input.getChannelPointer (channel);
    auto* samplesOut = output.getChannelPointer (channel);

    for (size_t sample = 0; sample < input.getNumSamples(); ++sample)
    {
        addingSamplesToOutputAndDelayModule(channel, samplesIn, samplesOut, sample, feedback, mix, gain);
    }
}

void AudioPluginAudioProcessor::addingSamplesToOutputAndDelayModule(size_t channel, const float* samplesIn,
                                                                    float* samplesOut, size_t sample,
                                                                    float feedback, float mix, float gain) {
    // extracting output of delay sample
    // the popSample task takes what's in the delayModule and puts it in delayOutput
    auto delayOutput = delayModule.popSample((int)channel);

    // Input Sample
    // we are taking in the input samples
    auto input = samplesIn[sample];

    //made an input for delay which is the combination of the input samples and feedback * delayoutput
    auto inputForDelay = samplesIn[sample] + feedback*delayOutput;

    // pusing input sample + delayOutput into delay module
    delayModule.pushSample((int)channel, inputForDelay);

    auto phase = treeState.getRawParameterValue("phase")->load();

    if (phase == 0)
    {
        // Combining both input and delayed sample
        samplesOut[sample] = (input*(1 - mix) + delayOutput*mix) * gain;
    }
    else
    {
        // Adding phase
        samplesOut[sample] = (input*(1 - mix) + delayOutput*mix) * gain * -1;
    }


}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
