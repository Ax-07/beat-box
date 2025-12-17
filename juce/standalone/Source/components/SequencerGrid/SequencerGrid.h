#pragma once
#include <JuceHeader.h>
#include "../../utils/Constants.h"
#include "../stepButton/StepButton.h"
#include <functional>
#include <array>

class SequencerGrid : public juce::Component
{
public:
    SequencerGrid();
    ~SequencerGrid() override = default;

    // Callback pour toggle d'un step
    std::function<void(int lane, int step, bool state)> onStepToggled;

    // Gestion de l'état des steps
    void setStepState(int lane, int step, bool state);
    bool getStepState(int lane, int step) const;

    // Gestion du playhead
    void setPlayheadPosition(int step);
    int getPlayheadPosition() const { return currentPlayheadPos; }

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    // Grille de boutons (3 lanes x 16 steps)
    std::array<std::array<StepButton, DrumBoxConstants::kSteps>, DrumBoxConstants::kLanes> stepButtons;

    // Position du playhead
    int currentPlayheadPos = -1;

    // Dimensions calculées
    int laneLabelWidth = 0;
    int buttonSize = 0;
    int buttonSpacing = 0;

    void drawLaneLabels(juce::Graphics& g);
    void drawMeasureMarkers(juce::Graphics& g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequencerGrid)
};
