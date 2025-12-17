#include "SequencerGrid.h"

using namespace DrumBoxConstants;

SequencerGrid::SequencerGrid()
{
    // Initialiser tous les boutons
    for (int lane = 0; lane < kLanes; ++lane)
    {
        for (int step = 0; step < kSteps; ++step)
        {
            auto &button = stepButtons[lane][step];
            addAndMakeVisible(button);

            // Callback pour chaque bouton - identique à l'ancien code
            button.onClick = [this, lane, step]()
            {
                if (onStepToggled)
                    onStepToggled(lane, step, stepButtons[lane][step].getToggleState());
            };
        }
    }
}

void SequencerGrid::setStepState(int lane, int step, bool state)
{
    if (lane >= 0 && lane < kLanes && step >= 0 && step < kSteps)
    {
        stepButtons[lane][step].setToggleState(state, juce::dontSendNotification);
    }
}

bool SequencerGrid::getStepState(int lane, int step) const
{
    if (lane >= 0 && lane < kLanes && step >= 0 && step < kSteps)
    {
        return stepButtons[lane][step].getToggleState();
    }
    return false;
}

void SequencerGrid::setPlayheadPosition(int step)
{
    if (currentPlayheadPos != step)
    {
        // Désactiver l'ancien playhead
        if (currentPlayheadPos >= 0 && currentPlayheadPos < kSteps)
        {
            for (int lane = 0; lane < kLanes; ++lane)
            {
                stepButtons[lane][currentPlayheadPos].setPlayhead(false);
            }
        }

        // Activer le nouveau playhead
        currentPlayheadPos = step;
        if (currentPlayheadPos >= 0 && currentPlayheadPos < kSteps)
        {
            for (int lane = 0; lane < kLanes; ++lane)
            {
                stepButtons[lane][currentPlayheadPos].setPlayhead(true);
            }
        }
    }
}

void SequencerGrid::paint(juce::Graphics &g)
{
    drawLaneLabels(g);
    drawMeasureMarkers(g);
}

void SequencerGrid::drawLaneLabels(juce::Graphics &g)
{
    g.setFont(juce::FontOptions("Courier New", 16.0f, juce::Font::bold));

    for (int lane = 0; lane < kLanes; ++lane)
    {
        auto &button = stepButtons[lane][0];
        int y = button.getY();

        juce::Rectangle<int> labelArea(0, y, laneLabelWidth - 10, buttonSize);

        // Couleur selon la lane
        g.setColour(Colors::getLaneColor(lane));
        g.drawText(LaneNames[lane], labelArea, juce::Justification::centredRight, false);
    }
}

void SequencerGrid::drawMeasureMarkers(juce::Graphics &g)
{
    g.setColour(Colors::measureMarker);

    // Marquer tous les 4 steps
    for (int step = 0; step < kSteps; step += 4)
    {
        auto &button = stepButtons[0][step];
        int x = button.getX() + (buttonSize / 2);

        // Ligne verticale légère
        g.drawLine(static_cast<float>(x),
                   static_cast<float>(stepButtons[0][step].getY()),
                   static_cast<float>(x),
                   static_cast<float>(stepButtons[kLanes - 1][step].getBottom()),
                   1.0f);
    }
}

void SequencerGrid::resized()
{
    auto area = getLocalBounds();

    laneLabelWidth = Layout::laneLabelWidth;
    buttonSpacing = 8; // Espacement entre les boutons

    // Calculer la taille des boutons
    int availableWidth = area.getWidth() - laneLabelWidth;
    int totalSpacing = buttonSpacing * (kSteps - 1);
    buttonSize = (availableWidth - totalSpacing) / kSteps;

    // Positionner les boutons
    int startX = laneLabelWidth;
    int startY = 10;
    int rowSpacing = 10;

    for (int lane = 0; lane < kLanes; ++lane)
    {
        int y = startY + lane * (buttonSize + rowSpacing);

        for (int step = 0; step < kSteps; ++step)
        {
            int x = startX + step * (buttonSize + buttonSpacing);
            stepButtons[lane][step].setBounds(x, y, buttonSize, buttonSize);
        }
    }
}
