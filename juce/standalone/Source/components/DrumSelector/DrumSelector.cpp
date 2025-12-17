// DrumBox/juce/standalone/Source/components/drumSelector/DrumSelector.cpp

#include "DrumSelector.h"

using namespace DrumBoxConstants;

DrumSelector::DrumSelector()
{
    // Kick button
    kickButton.setButtonText("KICK");
    kickButton.onClick = [this]
    {
        selectDrum(DrumIndex::Kick);
        if (onDrumSelected)
            onDrumSelected(DrumIndex::Kick);
    };
    addAndMakeVisible(kickButton);

    // Snare button
    snareButton.setButtonText("SNARE");
    snareButton.onClick = [this]
    {
        selectDrum(DrumIndex::Snare);
        if (onDrumSelected)
            onDrumSelected(DrumIndex::Snare);
    };
    addAndMakeVisible(snareButton);

    // Hat button
    hatButton.setButtonText("HAT");
    hatButton.onClick = [this]
    {
        selectDrum(DrumIndex::Hat);
        if (onDrumSelected)
            onDrumSelected(DrumIndex::Hat);
    };
    addAndMakeVisible(hatButton);

    // État initial : Kick sélectionné
    selectDrum(DrumIndex::Kick);
}

void DrumSelector::selectDrum(int drumIndex)
{
    if (drumIndex < 0 || drumIndex >= kLanes)
        return;
    
    selectedDrum = drumIndex;
    updateButtonColors();
}

void DrumSelector::updateButtonColors()
{
    // Couleur active pour le drum sélectionné, gris pour les autres
    kickButton.setColour(
        juce::TextButton::buttonColourId,
        selectedDrum == DrumIndex::Kick ? Colors::kick : Colors::buttonInactive
    );
    
    snareButton.setColour(
        juce::TextButton::buttonColourId,
        selectedDrum == DrumIndex::Snare ? Colors::snare : Colors::buttonInactive
    );
    
    hatButton.setColour(
        juce::TextButton::buttonColourId,
        selectedDrum == DrumIndex::Hat ? Colors::hat : Colors::buttonInactive
    );
}

void DrumSelector::resized()
{
    auto area = getLocalBounds();
    
    using namespace Layout;
    
    // Centrer les 3 boutons horizontalement
    auto selectorCenter = area.withSizeKeepingCentre(
        drumButtonWidth * 3 + drumButtonSpacing * 2, 
        36
    );
    
    kickButton.setBounds(selectorCenter.removeFromLeft(drumButtonWidth));
    selectorCenter.removeFromLeft(drumButtonSpacing);
    snareButton.setBounds(selectorCenter.removeFromLeft(drumButtonWidth));
    selectorCenter.removeFromLeft(drumButtonSpacing);
    hatButton.setBounds(selectorCenter);
}
