// DrumBox/juce/standalone/Source/components/transportBar/TransportBar.cpp

#include "TransportBar.h"

TransportBar::TransportBar()
{
    // Play button
    playButton.setButtonText("Play");
    playButton.onClick = [this]
    {
        const bool isPlaying = (playButton.getButtonText() == "Stop");
        const bool newState = !isPlaying;
        playButton.setButtonText(newState ? "Stop" : "Play");
        
        if (onPlayChanged)
            onPlayChanged(newState);
    };
    addAndMakeVisible(playButton);

    // BPM Label
    bpmLabel.setText("BPM", juce::dontSendNotification);
    bpmLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(bpmLabel);

    // BPM Slider
    bpmSlider.setRange(40.0, 240.0, 0.1);
    bpmSlider.setValue(120.0, juce::dontSendNotification);
    bpmSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    bpmSlider.onValueChange = [this]
    {
        if (onBpmChanged)
            onBpmChanged((float)bpmSlider.getValue());
    };
    addAndMakeVisible(bpmSlider);

    // Master Label
    masterLabel.setText("Master", juce::dontSendNotification);
    masterLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(masterLabel);

    // Master Slider
    masterSlider.setRange(0.0, 1.0, 0.001);
    masterSlider.setValue(0.6, juce::dontSendNotification);
    masterSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    masterSlider.onValueChange = [this]
    {
        if (onMasterChanged)
            onMasterChanged((float)masterSlider.getValue());
    };
    addAndMakeVisible(masterSlider);
}

void TransportBar::setPlaying(bool playing)
{
    playButton.setButtonText(playing ? "Stop" : "Play");
}

void TransportBar::setBpm(float bpm)
{
    bpmSlider.setValue(bpm, juce::dontSendNotification);
}

void TransportBar::setMasterGain(float gain)
{
    masterSlider.setValue(gain, juce::dontSendNotification);
}

void TransportBar::resized()
{
    auto area = getLocalBounds();
    area.removeFromLeft(200); // Espace pour le titre (géré par MainComponent)
    
    // Play button
    playButton.setBounds(area.removeFromLeft(100).reduced(4, 12));
    area.removeFromLeft(8);
    
    // BPM
    bpmLabel.setBounds(area.removeFromLeft(50).reduced(4, 12));
    bpmSlider.setBounds(area.removeFromLeft(220).reduced(4, 12));
    area.removeFromLeft(20);
    
    // Master (à droite)
    auto masterArea = area.removeFromRight(260);
    masterLabel.setBounds(masterArea.removeFromLeft(60).reduced(4, 12));
    masterSlider.setBounds(masterArea.reduced(4, 12));
}
