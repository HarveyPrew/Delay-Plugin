#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    // ----------------------------------- Gain -----------------------------------
    gain.setSliderStyle(juce::Slider::LinearVertical); // A vertical slider is selected
    gain.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20); // Dimensions of text box showing value
    gain.setPopupDisplayEnabled(true, false, this); //BPM shown as popup
    gain.setTextValueSuffix(" dB"); // Showing units alongside with value

    addAndMakeVisible (gain);

    gainSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "gain", gain);

    // ----------------------------------- Feedback -----------------------------------
    feedback.setSliderStyle(juce::Slider::LinearVertical); // A vertical slider is selected
    feedback.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20); // Dimensions of text box showing value
    feedback.setPopupDisplayEnabled(true, false, this); //BPM shown as popup
    feedback.setTextValueSuffix(" %"); // Showing units alongside with value

    addAndMakeVisible (feedback);

    feedbackSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "feedback", feedback);

    // ----------------------------------- Mix -----------------------------------
    mix.setSliderStyle(juce::Slider::LinearVertical); // A vertical slider is selected
    mix.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20); // Dimensions of text box showing value
    mix.setPopupDisplayEnabled(true, false, this); //dB shown as popup
    mix.setTextValueSuffix(" dB"); // Showing units alongside with value

    addAndMakeVisible (mix);

    mixSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "mix", mix);

    // ----------------------------------- Length -----------------------------------
    length.setSliderStyle(juce::Slider::LinearVertical); // A vertical slider is selected
    length.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20); // Dimensions of text box showing value
    length.setPopupDisplayEnabled(true, false, this); //mS shown as popup
    length.setTextValueSuffix(" mS"); // Showing units alongside with value

    addAndMakeVisible (length);

    lengthSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "length", length);

    setSize (400, 300);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Fill the whole window white
    g.fillAll (juce::Colours::green);

    // Set current drawing colour to black
    g.setColour(juce::Colours::black);

    // Set the font size and draw text to the screen
    g.setFont(15.0f);

    g.drawFittedText ("Delay", 0, 0, getWidth(), 30, juce::Justification::centred, 1);
}

void AudioPluginAudioProcessorEditor::resized()
{
    gain.setBounds (0, 160, 180, 180);

    feedback.setBounds(100, 160, 80, 50);

    mix.setBounds(200, 160, 80, 50);

    length.setBounds(300, 160, 80, 50);
}
