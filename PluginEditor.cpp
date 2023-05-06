#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    // ----------------------------------- Gain -----------------------------------
    gain.setSliderStyle(juce::Slider::LinearHorizontal); // A vertical slider is selected
    gain.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20); // Dimensions of text box showing value
    gain.setPopupDisplayEnabled(true, false, this); //BPM shown as popup
    gain.setTextValueSuffix(" dB"); // Showing units alongside with value

    addAndMakeVisible (gain);

    gainSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "gain", gain);

    // ----------------------------------- Feedback -----------------------------------
    feedback.setSliderStyle(juce::Slider::LinearHorizontal); // A vertical slider is selected
    feedback.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20); // Dimensions of text box showing value
    feedback.setPopupDisplayEnabled(true, false, this); //BPM shown as popup
    feedback.setTextValueSuffix(" %"); // Showing units alongside with value

    addAndMakeVisible (feedback);

    feedbackSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "feedback", feedback);

    // ----------------------------------- Mix -----------------------------------
    mix.setSliderStyle(juce::Slider::LinearHorizontal); // A vertical slider is selected
    mix.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20); // Dimensions of text box showing value
    mix.setPopupDisplayEnabled(true, false, this); //dB shown as popup
    mix.setTextValueSuffix(" dB"); // Showing units alongside with value

    addAndMakeVisible (mix);

    mixSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "mix", mix);

    // ----------------------------------- Length -----------------------------------
    length.setSliderStyle(juce::Slider::LinearHorizontal); // A vertical slider is selected
    length.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20); // Dimensions of text box showing value
    length.setPopupDisplayEnabled(true, false, this); //mS shown as popup
    length.setTextValueSuffix(" mS"); // Showing units alongside with value

    addAndMakeVisible (length);

    lengthSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "length", length);

    // ----------------------------------- Toggle -----------------------------------
    addAndMakeVisible(toggle);
    toggle.setToggleable(true);
    toggle.setButtonText("Toggle");
    toggleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processorRef.apvts, "toggle", toggle);

    // ----------------------------------- Phase -----------------------------------
    addAndMakeVisible(phase);
    phase.setToggleable(true);
    toggle.setButtonText("Phase");
    phaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processorRef.apvts, "phase", phase);

    setSize (400, 400);
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
    g.drawFittedText ("Gain", 0, 10, getWidth(), 30, juce::Justification::left, 1);
    g.drawFittedText ("Feedback", 0, 30, getWidth(), 30, juce::Justification::left, 1);
    g.drawFittedText ("Dry/ Wet", 0, 50, getWidth(), 30, juce::Justification::left, 1);
    g.drawFittedText ("Delay Time", 0, 100, getWidth(), 30, juce::Justification::left, 1);
    g.drawFittedText ("ϕ", 0, 320, getWidth(), 30, juce::Justification::right, 1);
}

void AudioPluginAudioProcessorEditor::resized()
{

    gain.setBounds (200, 10, 180, 180);
    feedback.setBounds(200, 30, 180, 180);
    mix.setBounds(200, 50, 180, 180);
    length.setBounds(200, 100, 180, 180);
    toggle.setBounds(300, 350, 80, 50);
    phase.setBounds(300, 0, 80, 50);
}
