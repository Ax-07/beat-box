// DrumBox/juce/standalone/Source/MainComponent.h

#pragma once
#include <JuceHeader.h>
#include "drumbox_core/Engine.h"
#include "components/stepButton/StepButton.h"
#include "components/sequencerGrid/SequencerGrid.h"
#include "components/drumControlPanel/DrumControlPanel.h"
#include "components/drumSelector/DrumSelector.h"
#include "components/DrumWavePreviewComponent/DrumWavePreviewComponent.h"
#include "utils/Command.h"
#include "utils/Constants.h"
#include <array>
#include <vector>

class MainComponent : public juce::AudioAppComponent,
                      public juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    void pushToggle(int lane, int step, bool on);
    void refreshGridFromPattern();
    int lastPlayheadStep = -1;
    void updatePlayheadOutline();
    // UI
    juce::TextButton playButton { "Stop" };
    juce::Slider bpmSlider;
    juce::Label bpmLabel;

    juce::Slider masterSlider;
    juce::Label masterLabel;

    // Sélecteur de drum
    juce::TextButton kickSelectButton { "KICK" };
    juce::TextButton snareSelectButton { "SNARE" };
    juce::TextButton hatSelectButton { "HAT" };
    int selectedDrum = 0; // 0=Kick, 1=Snare, 2=Hat

    // Anciens contrôles - commentés
    /*
    juce::GroupComponent kickGroup, snareGroup, hatGroup;
    juce::Slider kickDecay, kickAttack, kickBase;
    juce::Slider snareDecay, snareTone, snareNoiseMix;
    juce::Slider hatDecay, hatCutoff;
    */
    
    // Nouveaux composants
    DrumSelector drumSelector;
    juce::Viewport drumControlViewport;
    DrumControlPanel drumControlPanel;
    std::unique_ptr<DrumWavePreviewComponent> drumWavePreview;

    // Grille 3x16
    static constexpr int kLanes = DrumBoxConstants::kLanes;
    static constexpr int kSteps = DrumBoxConstants::kSteps;
    // std::array<std::array<StepButton, kSteps>, kLanes> grid; // Ancien code commenté
    SequencerGrid sequencerGrid; // Nouveau composant


    // Core
    drumbox_core::Engine engine;
    SpscQueue<DrumBoxConstants::Audio::commandQueueSize> queue;

    std::vector<float> interleavedTmp;

    std::atomic<bool> playing{true};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
