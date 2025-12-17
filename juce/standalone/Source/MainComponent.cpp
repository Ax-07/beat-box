// DrumBox/juce/standalone/Source/MainComponent.cpp

#include "MainComponent.h"
#include <vector>

using namespace DrumBoxConstants;

static const char* laneName(int lane)
{
    switch (lane)
    {
        case 0: return "KICK";
        case 1: return "SNARE";
        default: return "HAT";
    }
}

MainComponent::MainComponent()
{
    // Audio
    setAudioChannels(0, 2);

    // Play / BPM
    addAndMakeVisible(playButton);
    playButton.onClick = [this]
    {
        const bool newPlay = !playing.load();
        playing.store(newPlay);

        queue.push(Command::setPlaying(newPlay));

        playButton.setButtonText(newPlay ? "Stop" : "Play");
    };

    bpmLabel.setText("BPM", juce::dontSendNotification);
    addAndMakeVisible(bpmLabel);

    bpmSlider.setRange(40.0, 240.0, 0.1);
    bpmSlider.setValue(120.0);
    bpmSlider.onValueChange = [this]
    {
        queue.push(Command::setBpm((float)bpmSlider.getValue()));
    };
    addAndMakeVisible(bpmSlider);

    // Master
    masterLabel.setText("Master", juce::dontSendNotification);
    addAndMakeVisible(masterLabel);

    // UI en dB (converti vers gain linéaire pour le core)
    auto dbFromLinear = [](double lin) {
        lin = std::max(1.0e-9, lin);
        return 20.0 * std::log10(lin);
    };
    auto linearFromDb = [](double db) {
        return std::pow(10.0, db / 20.0);
    };

    masterSlider.setRange(-60.0, 0.0, 0.1);
    masterSlider.setValue(dbFromLinear(0.6));
    masterSlider.textFromValueFunction = [](double v) {
        return juce::String(v, 1) + " dB";
    };
    masterSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.-").getDoubleValue();
    };
    masterSlider.onValueChange = [this]
    {
        const double lin = std::pow(10.0, masterSlider.getValue() / 20.0);
        engine.params().masterGain.store((float)lin, std::memory_order_relaxed);
    };
    addAndMakeVisible(masterSlider);

    // Boutons de sélection de drum
    addAndMakeVisible(kickSelectButton);
    addAndMakeVisible(snareSelectButton);
    addAndMakeVisible(hatSelectButton);
    
    kickSelectButton.onClick = [this] {
        selectedDrum = 0;
        kickSelectButton.setColour(juce::TextButton::buttonColourId, DrumBoxConstants::Colors::kick);
        snareSelectButton.setColour(juce::TextButton::buttonColourId, DrumBoxConstants::Colors::buttonInactive);
        hatSelectButton.setColour(juce::TextButton::buttonColourId, DrumBoxConstants::Colors::buttonInactive);
        resized();
    };
    
    snareSelectButton.onClick = [this] {
        selectedDrum = 1;
        kickSelectButton.setColour(juce::TextButton::buttonColourId, DrumBoxConstants::Colors::buttonInactive);
        snareSelectButton.setColour(juce::TextButton::buttonColourId, DrumBoxConstants::Colors::snare);
        hatSelectButton.setColour(juce::TextButton::buttonColourId, DrumBoxConstants::Colors::buttonInactive);
        resized();
    };
    
    hatSelectButton.onClick = [this] {
        selectedDrum = 2;
        kickSelectButton.setColour(juce::TextButton::buttonColourId, DrumBoxConstants::Colors::buttonInactive);
        snareSelectButton.setColour(juce::TextButton::buttonColourId, DrumBoxConstants::Colors::buttonInactive);
        hatSelectButton.setColour(juce::TextButton::buttonColourId, DrumBoxConstants::Colors::hat);
        resized();
    };
    
    // État initial: Kick sélectionné
    kickSelectButton.setColour(juce::TextButton::buttonColourId, DrumBoxConstants::Colors::kick);
    snareSelectButton.setColour(juce::TextButton::buttonColourId, DrumBoxConstants::Colors::buttonInactive);
    hatSelectButton.setColour(juce::TextButton::buttonColourId, DrumBoxConstants::Colors::buttonInactive);

    // === DRUM SELECTOR ===
    addAndMakeVisible(drumSelector);
    drumSelector.onDrumSelected = [this](int drum) {
        selectedDrum = drum;
        drumControlPanel.setSelectedDrum(drum);
        
        // Mettre à jour le preview avec le nouveau drum
        if (drumWavePreview)
            drumWavePreview->rerender();
        
        // Synchroniser avec les anciens boutons
        kickSelectButton.setColour(juce::TextButton::buttonColourId, 
            drum == 0 ? DrumBoxConstants::Colors::kick : DrumBoxConstants::Colors::buttonInactive);
        snareSelectButton.setColour(juce::TextButton::buttonColourId, 
            drum == 1 ? DrumBoxConstants::Colors::snare : DrumBoxConstants::Colors::buttonInactive);
        hatSelectButton.setColour(juce::TextButton::buttonColourId, 
            drum == 2 ? DrumBoxConstants::Colors::hat : DrumBoxConstants::Colors::buttonInactive);
    };

    // === DRUM CONTROL PANEL ===
    addAndMakeVisible(drumControlViewport);
    drumControlViewport.setViewedComponent(&drumControlPanel, false);
    drumControlViewport.setScrollBarsShown(true, false);
    drumControlViewport.setScrollBarThickness(10);
    
    drumControlPanel.onDecayChanged = [this](int lane, float value) {
        if (lane == 0)
            engine.params().kickDecay.store(value, std::memory_order_relaxed);
        else if (lane == 1)
            engine.params().snareDecay.store(value, std::memory_order_relaxed);
        else if (lane == 2)
            engine.params().hatDecay.store(value, std::memory_order_relaxed);
        
        if (drumWavePreview)
            drumWavePreview->rerender();
    };
    
    drumControlPanel.onKickAttackChanged = [this](float value) {
        engine.params().kickAttackFreq.store(value, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    
    drumControlPanel.onKickPitchChanged = [this](float value) {
        engine.params().kickBaseFreq.store(value, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickPitchDecayChanged = [this](float v) {
        engine.params().kickPitchDecay.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickDriveChanged = [this](float v) {
        engine.params().kickDriveAmount.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickDriveDecayChanged = [this](float v) {
        engine.params().kickDriveDecay.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickClickChanged = [this](float v) {
        engine.params().kickClickGain.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickHpChanged = [this](float v) {
        engine.params().kickPreHpHz.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickPostGainChanged = [this](float v) {
        engine.params().kickPostGain.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickPostHpChanged = [this](float v) {
        engine.params().kickPostHpHz.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickPostLpChanged = [this](float v) {
        engine.params().kickPostLpHz.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickChain1MixChanged = [this](float v) {
        engine.params().kickChain1Mix.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickChain1DriveMulChanged = [this](float v) {
        engine.params().kickChain1DriveMul.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickChain1LpHzChanged = [this](float v) {
        engine.params().kickChain1LpHz.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickChain1AsymChanged = [this](float v) {
        engine.params().kickChain1Asym.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickChain1ClipModeChanged = [this](int mode) {
        engine.params().kickChain1ClipMode.store((float)mode, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickChain2MixChanged = [this](float v) {
        engine.params().kickChain2Mix.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickChain2DriveMulChanged = [this](float v) {
        engine.params().kickChain2DriveMul.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickChain2LpHzChanged = [this](float v) {
        engine.params().kickChain2LpHz.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickChain2AsymChanged = [this](float v) {
        engine.params().kickChain2Asym.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickChain2ClipModeChanged = [this](int mode) {
        engine.params().kickChain2ClipMode.store((float)mode, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickTokAmountChanged = [this](float v) {
        engine.params().kickTokAmount.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickTokHpHzChanged = [this](float v) {
        engine.params().kickTokHpHz.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickCrunchAmountChanged = [this](float v) {
        engine.params().kickCrunchAmount.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickTailDecayChanged = [this](float v) {
        engine.params().kickTailDecay.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickTailMixChanged = [this](float v) {
        engine.params().kickTailMix.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickTailFreqMulChanged = [this](float v) {
        engine.params().kickTailFreqMul.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickSubMixChanged = [this](float v) {
        engine.params().kickSubMix.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickSubLpHzChanged = [this](float v) {
        engine.params().kickSubLpHz.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickFeedbackChanged = [this](float v) {
        engine.params().kickFeedback.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    // === Kick Layers (1/2) ===
    drumControlPanel.onKickLayer1EnabledChanged = [this](float v) {
        engine.params().kickLayer1Enabled.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer1TypeChanged = [this](float v) {
        engine.params().kickLayer1Type.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer1FreqHzChanged = [this](float v) {
        engine.params().kickLayer1FreqHz.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer1Phase01Changed = [this](float v) {
        engine.params().kickLayer1Phase01.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer1DriveChanged = [this](float v) {
        engine.params().kickLayer1Drive.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer1AttackCoeffChanged = [this](float v) {
        engine.params().kickLayer1AttackCoeff.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer1DecayCoeffChanged = [this](float v) {
        engine.params().kickLayer1DecayCoeff.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer1VolChanged = [this](float v) {
        engine.params().kickLayer1Vol.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickLayer2EnabledChanged = [this](float v) {
        engine.params().kickLayer2Enabled.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer2TypeChanged = [this](float v) {
        engine.params().kickLayer2Type.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer2FreqHzChanged = [this](float v) {
        engine.params().kickLayer2FreqHz.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer2Phase01Changed = [this](float v) {
        engine.params().kickLayer2Phase01.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer2DriveChanged = [this](float v) {
        engine.params().kickLayer2Drive.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer2AttackCoeffChanged = [this](float v) {
        engine.params().kickLayer2AttackCoeff.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer2DecayCoeffChanged = [this](float v) {
        engine.params().kickLayer2DecayCoeff.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer2VolChanged = [this](float v) {
        engine.params().kickLayer2Vol.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    // === Kick LFO ===
    drumControlPanel.onKickLfoAmountChanged = [this](float v) {
        engine.params().kickLfoAmount.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLfoRateHzChanged = [this](float v) {
        engine.params().kickLfoRateHz.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLfoShapeChanged = [this](float v) {
        engine.params().kickLfoShape.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLfoTargetChanged = [this](float v) {
        engine.params().kickLfoTarget.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLfoPulseChanged = [this](float v) {
        engine.params().kickLfoPulse.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    // === Kick Reverb + Quality ===
    drumControlPanel.onKickReverbAmountChanged = [this](float v) {
        engine.params().kickReverbAmount.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickReverbSizeChanged = [this](float v) {
        engine.params().kickReverbSize.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickReverbToneChanged = [this](float v) {
        engine.params().kickReverbTone.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickOversample2xChanged = [this](float v) {
        engine.params().kickOversample2x.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickFxShiftHzChanged = [this](float v) {
        engine.params().kickFxShiftHz.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickFxStereoChanged = [this](float v) {
        engine.params().kickFxStereo.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickFxDiffusionChanged = [this](float v) {
        engine.params().kickFxDiffusion.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickFxCleanDirtyChanged = [this](float v) {
        engine.params().kickFxCleanDirty.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickFxToneChanged = [this](float v) {
        engine.params().kickFxTone.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickFxEnvAttackCoeffChanged = [this](float v) {
        engine.params().kickFxEnvAttackCoeff.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickFxEnvDecayCoeffChanged = [this](float v) {
        engine.params().kickFxEnvDecayCoeff.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickFxEnvVolChanged = [this](float v) {
        engine.params().kickFxEnvVol.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickFxDisperseChanged = [this](float v) {
        engine.params().kickFxDisperse.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickFxOttAmountChanged = [this](float v) {
        engine.params().kickFxOttAmount.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickFxInflatorChanged = [this](float v) {
        engine.params().kickFxInflator.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickFxInflatorMixChanged = [this](float v) {
        engine.params().kickFxInflatorMix.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    // === Master ===
    drumControlPanel.onMasterEqLowDbChanged = [this](float v) {
        engine.params().masterEqLowDb.store(v, std::memory_order_relaxed);
        if (drumWavePreview)
            drumWavePreview->rerender();
    };
    drumControlPanel.onMasterEqMidDbChanged = [this](float v) {
        engine.params().masterEqMidDb.store(v, std::memory_order_relaxed);
        if (drumWavePreview)
            drumWavePreview->rerender();
    };
    drumControlPanel.onMasterEqHighDbChanged = [this](float v) {
        engine.params().masterEqHighDb.store(v, std::memory_order_relaxed);
        if (drumWavePreview)
            drumWavePreview->rerender();
    };
    drumControlPanel.onMasterClipOnChanged = [this](float v) {
        engine.params().masterClipOn.store(v, std::memory_order_relaxed);
        if (drumWavePreview)
            drumWavePreview->rerender();
    };
    drumControlPanel.onMasterClipModeChanged = [this](float v) {
        engine.params().masterClipMode.store(v, std::memory_order_relaxed);
        if (drumWavePreview)
            drumWavePreview->rerender();
    };

    // === Kick layers ===
    drumControlPanel.onKickLayer1EnabledChanged = [this](float v) {
        engine.params().kickLayer1Enabled.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer1TypeChanged = [this](float v) {
        engine.params().kickLayer1Type.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer1FreqHzChanged = [this](float v) {
        engine.params().kickLayer1FreqHz.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer1Phase01Changed = [this](float v) {
        engine.params().kickLayer1Phase01.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer1DriveChanged = [this](float v) {
        engine.params().kickLayer1Drive.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer1AttackCoeffChanged = [this](float v) {
        engine.params().kickLayer1AttackCoeff.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer1DecayCoeffChanged = [this](float v) {
        engine.params().kickLayer1DecayCoeff.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer1VolChanged = [this](float v) {
        engine.params().kickLayer1Vol.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickLayer2EnabledChanged = [this](float v) {
        engine.params().kickLayer2Enabled.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer2TypeChanged = [this](float v) {
        engine.params().kickLayer2Type.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer2FreqHzChanged = [this](float v) {
        engine.params().kickLayer2FreqHz.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer2Phase01Changed = [this](float v) {
        engine.params().kickLayer2Phase01.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer2DriveChanged = [this](float v) {
        engine.params().kickLayer2Drive.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer2AttackCoeffChanged = [this](float v) {
        engine.params().kickLayer2AttackCoeff.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer2DecayCoeffChanged = [this](float v) {
        engine.params().kickLayer2DecayCoeff.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };
    drumControlPanel.onKickLayer2VolChanged = [this](float v) {
        engine.params().kickLayer2Vol.store(v, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickClipModeChanged = [this](int mode) {
        engine.params().kickClipMode.store((float)mode, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onKickPresetSelected = [this](int presetId) {
        // presetId: 1=Gabber, 2=Hardstyle, 3=Tribecore
        if (presetId == 1)
        {
            engine.params().kickPostHpHz.store(28.0f, std::memory_order_relaxed);
            engine.params().kickTailMix.store(0.65f, std::memory_order_relaxed);
            engine.params().kickTailDecay.store(0.99935f, std::memory_order_relaxed);
            engine.params().kickTailFreqMul.store(1.0f, std::memory_order_relaxed);
            engine.params().kickSubMix.store(0.25f, std::memory_order_relaxed);
            engine.params().kickSubLpHz.store(160.0f, std::memory_order_relaxed);
            engine.params().kickFeedback.store(0.18f, std::memory_order_relaxed);

            engine.params().kickChain1Mix.store(0.45f, std::memory_order_relaxed);
            engine.params().kickChain1DriveMul.store(1.30f, std::memory_order_relaxed);
            engine.params().kickChain1LpHz.store(8500.0f, std::memory_order_relaxed);
            engine.params().kickChain1Asym.store(0.05f, std::memory_order_relaxed);

            engine.params().kickChain2Mix.store(0.55f, std::memory_order_relaxed);
            engine.params().kickChain2DriveMul.store(2.20f, std::memory_order_relaxed);
            engine.params().kickChain2LpHz.store(4200.0f, std::memory_order_relaxed);
            engine.params().kickChain2Asym.store(0.35f, std::memory_order_relaxed);

            engine.params().kickTokAmount.store(0.25f, std::memory_order_relaxed);
            engine.params().kickTokHpHz.store(200.0f, std::memory_order_relaxed);
            engine.params().kickCrunchAmount.store(0.35f, std::memory_order_relaxed);
        }
        else if (presetId == 2)
        {
            engine.params().kickPostHpHz.store(24.0f, std::memory_order_relaxed);
            engine.params().kickTailMix.store(0.55f, std::memory_order_relaxed);
            engine.params().kickTailDecay.store(0.99925f, std::memory_order_relaxed);
            engine.params().kickTailFreqMul.store(1.0f, std::memory_order_relaxed);
            engine.params().kickSubMix.store(0.32f, std::memory_order_relaxed);
            engine.params().kickSubLpHz.store(180.0f, std::memory_order_relaxed);
            engine.params().kickFeedback.store(0.12f, std::memory_order_relaxed);

            engine.params().kickChain1Mix.store(0.70f, std::memory_order_relaxed);
            engine.params().kickChain1DriveMul.store(1.20f, std::memory_order_relaxed);
            engine.params().kickChain1LpHz.store(9000.0f, std::memory_order_relaxed);
            engine.params().kickChain1Asym.store(0.05f, std::memory_order_relaxed);

            engine.params().kickChain2Mix.store(0.30f, std::memory_order_relaxed);
            engine.params().kickChain2DriveMul.store(1.60f, std::memory_order_relaxed);
            engine.params().kickChain2LpHz.store(5200.0f, std::memory_order_relaxed);
            engine.params().kickChain2Asym.store(0.20f, std::memory_order_relaxed);

            engine.params().kickTokAmount.store(0.18f, std::memory_order_relaxed);
            engine.params().kickTokHpHz.store(160.0f, std::memory_order_relaxed);
            engine.params().kickCrunchAmount.store(0.18f, std::memory_order_relaxed);
        }
        else
        {
            engine.params().kickPostHpHz.store(35.0f, std::memory_order_relaxed);
            engine.params().kickTailMix.store(0.40f, std::memory_order_relaxed);
            engine.params().kickTailDecay.store(0.99910f, std::memory_order_relaxed);
            engine.params().kickTailFreqMul.store(1.0f, std::memory_order_relaxed);
            engine.params().kickSubMix.store(0.38f, std::memory_order_relaxed);
            engine.params().kickSubLpHz.store(200.0f, std::memory_order_relaxed);
            engine.params().kickFeedback.store(0.06f, std::memory_order_relaxed);

            engine.params().kickChain1Mix.store(0.75f, std::memory_order_relaxed);
            engine.params().kickChain1DriveMul.store(1.05f, std::memory_order_relaxed);
            engine.params().kickChain1LpHz.store(10000.0f, std::memory_order_relaxed);
            engine.params().kickChain1Asym.store(-0.05f, std::memory_order_relaxed);

            engine.params().kickChain2Mix.store(0.25f, std::memory_order_relaxed);
            engine.params().kickChain2DriveMul.store(1.30f, std::memory_order_relaxed);
            engine.params().kickChain2LpHz.store(6500.0f, std::memory_order_relaxed);
            engine.params().kickChain2Asym.store(0.10f, std::memory_order_relaxed);

            engine.params().kickTokAmount.store(0.35f, std::memory_order_relaxed);
            engine.params().kickTokHpHz.store(260.0f, std::memory_order_relaxed);
            engine.params().kickCrunchAmount.store(0.10f, std::memory_order_relaxed);
        }

        if (drumWavePreview && selectedDrum == 0)
            drumWavePreview->rerender();
    };

    drumControlPanel.onSnareToneChanged = [this](float value) {
        engine.params().snareToneFreq.store(value, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 1)
            drumWavePreview->rerender();
    };
    
    drumControlPanel.onSnareNoiseMixChanged = [this](float value) {
        engine.params().snareNoiseMix.store(value, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 1)
            drumWavePreview->rerender();
    };
    
    drumControlPanel.onHatCutoffChanged = [this](float value) {
        engine.params().hatCutoff.store(value, std::memory_order_relaxed);
        if (drumWavePreview && selectedDrum == 2)
            drumWavePreview->rerender();
    };

    // Grid - Nouveau composant
    addAndMakeVisible(sequencerGrid);
    sequencerGrid.onStepToggled = [this](int lane, int step, bool state) {
        pushToggle(lane, step, state);
    };

    // === DRUM WAVE PREVIEW ===
    drumWavePreview = std::make_unique<DrumWavePreviewComponent>(deviceManager);
    addAndMakeVisible(*drumWavePreview);
    drumWavePreview->setTitle("Drum Preview");
    drumWavePreview->setSampleRate(48000);
    drumWavePreview->setDurationMs(400);
    
    // Configure le render pour le drum sélectionné
    drumWavePreview->setRenderFn([this](juce::AudioBuffer<float>& out, int sr, int durMs) {
        // Créer un engine temporaire pour le rendu offline
        drumbox_core::Engine tempEngine;
        tempEngine.prepare((double)sr, out.getNumSamples());
        
        // Copier les paramètres actuels (manuellement car std::atomic n'est pas copiable)
        auto& src = engine.params();
        auto& dst = tempEngine.params();
        
        dst.masterGain.store(src.masterGain.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickDecay.store(src.kickDecay.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickPitchDecay.store(src.kickPitchDecay.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickDriveDecay.store(src.kickDriveDecay.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickAttackFreq.store(src.kickAttackFreq.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickBaseFreq.store(src.kickBaseFreq.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickDriveAmount.store(src.kickDriveAmount.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickClickGain.store(src.kickClickGain.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickPreHpHz.store(src.kickPreHpHz.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickPostGain.store(src.kickPostGain.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickPostHpHz.store(src.kickPostHpHz.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickPostLpHz.store(src.kickPostLpHz.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickClipMode.store(src.kickClipMode.load(std::memory_order_relaxed), std::memory_order_relaxed);

        dst.kickTailDecay.store(src.kickTailDecay.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickTailMix.store(src.kickTailMix.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickTailFreqMul.store(src.kickTailFreqMul.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickSubMix.store(src.kickSubMix.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickSubLpHz.store(src.kickSubLpHz.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickFeedback.store(src.kickFeedback.load(std::memory_order_relaxed), std::memory_order_relaxed);

        dst.kickChain1Mix.store(src.kickChain1Mix.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickChain1DriveMul.store(src.kickChain1DriveMul.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickChain1LpHz.store(src.kickChain1LpHz.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickChain1Asym.store(src.kickChain1Asym.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickChain1ClipMode.store(src.kickChain1ClipMode.load(std::memory_order_relaxed), std::memory_order_relaxed);

        dst.kickChain2Mix.store(src.kickChain2Mix.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickChain2DriveMul.store(src.kickChain2DriveMul.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickChain2LpHz.store(src.kickChain2LpHz.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickChain2Asym.store(src.kickChain2Asym.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickChain2ClipMode.store(src.kickChain2ClipMode.load(std::memory_order_relaxed), std::memory_order_relaxed);

        dst.kickTokAmount.store(src.kickTokAmount.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickTokHpHz.store(src.kickTokHpHz.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickCrunchAmount.store(src.kickCrunchAmount.load(std::memory_order_relaxed), std::memory_order_relaxed);

        // Kick layers
        dst.kickLayer1Enabled.store(src.kickLayer1Enabled.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickLayer1Type.store(src.kickLayer1Type.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickLayer1FreqHz.store(src.kickLayer1FreqHz.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickLayer1Phase01.store(src.kickLayer1Phase01.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickLayer1Drive.store(src.kickLayer1Drive.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickLayer1AttackCoeff.store(src.kickLayer1AttackCoeff.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickLayer1DecayCoeff.store(src.kickLayer1DecayCoeff.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickLayer1Vol.store(src.kickLayer1Vol.load(std::memory_order_relaxed), std::memory_order_relaxed);

        dst.kickLayer2Enabled.store(src.kickLayer2Enabled.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickLayer2Type.store(src.kickLayer2Type.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickLayer2FreqHz.store(src.kickLayer2FreqHz.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickLayer2Phase01.store(src.kickLayer2Phase01.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickLayer2Drive.store(src.kickLayer2Drive.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickLayer2AttackCoeff.store(src.kickLayer2AttackCoeff.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickLayer2DecayCoeff.store(src.kickLayer2DecayCoeff.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickLayer2Vol.store(src.kickLayer2Vol.load(std::memory_order_relaxed), std::memory_order_relaxed);

        dst.kickLfoAmount.store(src.kickLfoAmount.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickLfoRateHz.store(src.kickLfoRateHz.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickLfoShape.store(src.kickLfoShape.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickLfoTarget.store(src.kickLfoTarget.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickLfoPulse.store(src.kickLfoPulse.load(std::memory_order_relaxed), std::memory_order_relaxed);

        // Kick Reverb + quality
        dst.kickReverbAmount.store(src.kickReverbAmount.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickReverbSize.store(src.kickReverbSize.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickReverbTone.store(src.kickReverbTone.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickOversample2x.store(src.kickOversample2x.load(std::memory_order_relaxed), std::memory_order_relaxed);

        dst.kickFxDisperse.store(src.kickFxDisperse.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickFxInflator.store(src.kickFxInflator.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickFxInflatorMix.store(src.kickFxInflatorMix.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickFxOttAmount.store(src.kickFxOttAmount.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickFxShiftHz.store(src.kickFxShiftHz.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickFxStereo.store(src.kickFxStereo.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickFxDiffusion.store(src.kickFxDiffusion.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickFxCleanDirty.store(src.kickFxCleanDirty.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickFxTone.store(src.kickFxTone.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickFxEnvAttackCoeff.store(src.kickFxEnvAttackCoeff.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickFxEnvDecayCoeff.store(src.kickFxEnvDecayCoeff.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.kickFxEnvVol.store(src.kickFxEnvVol.load(std::memory_order_relaxed), std::memory_order_relaxed);

        // Master
        dst.masterEqLowDb.store(src.masterEqLowDb.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.masterEqMidDb.store(src.masterEqMidDb.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.masterEqHighDb.store(src.masterEqHighDb.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.masterClipOn.store(src.masterClipOn.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.masterClipMode.store(src.masterClipMode.load(std::memory_order_relaxed), std::memory_order_relaxed);

        dst.snareDecay.store(src.snareDecay.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.snareToneFreq.store(src.snareToneFreq.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.snareNoiseMix.store(src.snareNoiseMix.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.hatDecay.store(src.hatDecay.load(std::memory_order_relaxed), std::memory_order_relaxed);
        dst.hatCutoff.store(src.hatCutoff.load(std::memory_order_relaxed), std::memory_order_relaxed);
        
        // Activer le step 0 pour le drum sélectionné
        tempEngine.setStep(selectedDrum, 0, true, 1.0f);
        tempEngine.setPlaying(true);
        
        // Render le buffer
        std::vector<float> interleaved((size_t)out.getNumSamples() * 2u);
        tempEngine.process(interleaved.data(), out.getNumSamples(), 2);
        
        // Copie canal gauche vers mono
        for (int i = 0; i < out.getNumSamples(); ++i)
            out.setSample(0, i, interleaved[i * 2]);
    });
    
    drumWavePreview->rerender();

    setSize(DrumBoxConstants::Window::defaultWidth, 
            DrumBoxConstants::Window::defaultHeight);
    startTimerHz(DrumBoxConstants::Audio::uiRefreshRate);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    engine.prepare(sampleRate, samplesPerBlockExpected);
    drumControlPanel.setSampleRate(sampleRate);
    engine.setBpm((float)bpmSlider.getValue());
    engine.setPlaying(false);
    playing.store(false);
    playButton.setButtonText("Play");

    lastPlayheadStep = -1;
    updatePlayheadOutline();

    // masterSlider est en dB côté UI
    engine.params().masterGain.store((float)std::pow(10.0, masterSlider.getValue() / 20.0), std::memory_order_relaxed);

    interleavedTmp.resize((size_t)samplesPerBlockExpected * 2);

    // sync UI pattern (le core a un pattern demo)
    refreshGridFromPattern();
}

void MainComponent::releaseResources() {}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& info)
{
    info.clearActiveBufferRegion();

    Command cmd;
    while (queue.pop(cmd))
    {
        switch (cmd.type)
        {
            case Command::ToggleStep:
                engine.setStep(cmd.lane, cmd.step, cmd.on, 1.0f);
                break;
            case Command::SetBpm:
                engine.setBpm(cmd.f);
                break;
            case Command::SetPlaying:
                engine.setPlaying(cmd.on);
                break;
        }
    }

    auto* buffer = info.buffer;
    const int ch = buffer->getNumChannels();
    const int n  = info.numSamples;

    if ((int)interleavedTmp.size() < n * ch)
        interleavedTmp.resize((size_t)n * (size_t)ch);

    engine.process(interleavedTmp.data(), n, ch);

    for (int c = 0; c < ch; ++c)
    {
        float* dst = buffer->getWritePointer(c, info.startSample);
        for (int i = 0; i < n; ++i)
            dst[i] = interleavedTmp[(size_t)i * (size_t)ch + (size_t)c];
    }
}

void MainComponent::pushToggle(int lane, int step, bool on)
{
    queue.push(Command::toggleStep(lane, step, on));
}

void MainComponent::refreshGridFromPattern()
{
    // Pour l’instant on n’a pas d’accès public au pattern dans Engine.
    // Donc on ne resync pas depuis le core (on pourra l’ajouter plus tard).
    // Ici on laisse l’UI telle quelle.
}

void MainComponent::updatePlayheadOutline()
{
    const int step = engine.getStepIndex();
    if (step == lastPlayheadStep) return;
    lastPlayheadStep = step;

    sequencerGrid.setPlayheadPosition(step);
}

void MainComponent::timerCallback()
{
    updatePlayheadOutline();
    repaint();
}

void MainComponent::paint (juce::Graphics& g)
{
    // Background dégradé
    auto bounds = getLocalBounds();
    juce::ColourGradient gradient(
        DrumBoxConstants::Colors::backgroundTop, 0, 0,
        DrumBoxConstants::Colors::backgroundBottom, 0, (float)bounds.getHeight(), 
        false
    );
    g.setGradientFill(gradient);
    g.fillAll();

    // Titre principal
    g.setColour(DrumBoxConstants::Colors::accent);
    g.setFont(juce::Font(24.0f, juce::Font::bold));
    g.drawText("DrumBox", 20, 12, 200, 40, juce::Justification::left);

    // Step indicator
    const int step = engine.getStepIndex();
    g.setColour(juce::Colours::lightgrey);
    g.setFont(14.0f);
    g.drawText("Step: " + juce::String(step + 1) + "/" + juce::String(kSteps), 
               20, 50, 150, 20, juce::Justification::left);
}

void MainComponent::resized()
{
    using namespace DrumBoxConstants::Layout;
    
    auto area = getLocalBounds().reduced(windowMargin);

    // === TOP BAR : Transport + Master ===
    auto topBar = area.removeFromTop(transportHeight);
    topBar.removeFromLeft(200); // Espace pour le titre
    
    // Play button
    playButton.setBounds(topBar.removeFromLeft(100).reduced(4, 12));
    topBar.removeFromLeft(8);
    
    // BPM
    bpmLabel.setBounds(topBar.removeFromLeft(50).reduced(4, 12));
    bpmSlider.setBounds(topBar.removeFromLeft(220).reduced(4, 12));
    topBar.removeFromLeft(20);
    
    // Master (à droite)
    auto masterArea = topBar.removeFromRight(260);
    masterLabel.setBounds(masterArea.removeFromLeft(60).reduced(4, 12));
    masterSlider.setBounds(masterArea.reduced(4, 12));

    // === MILIEU : Contrôles des instruments (un seul affiché) ===
    auto drumControls = area.removeFromTop(drumControlHeight);
    
    // Boutons de sélection en haut de la zone de contrôle
    auto selectorBar = drumControls.removeFromTop(drumSelectorHeight);
    drumSelector.setBounds(selectorBar);
    
    // Zone de contrôle empilée verticalement : Preview en haut, Panneau en bas
    auto controlArea = drumControls.reduced(8, 4);
    
    // En haut: preview waveform
    auto previewArea = controlArea.removeFromTop(100);
    if (drumWavePreview)
    {
        drumWavePreview->setBounds(previewArea);
        DBG("DrumWavePreview bounds: " << previewArea.toString());
    }
    
    controlArea.removeFromTop(6); // Marge
    
    // En bas: panneau de contrôle (knobs)
    drumControlViewport.setBounds(controlArea);

    // Le panel contient beaucoup de contrôles (Kick). On calcule une hauteur de contenu
    // cohérente avec le layout pour éviter le clipping et limiter le scroll.
    const int preferredH = drumControlPanel.getPreferredHeightForWidth(controlArea.getWidth());
    drumControlPanel.setSize(controlArea.getWidth(), std::max(controlArea.getHeight(), preferredH));

    // === BAS : GRILLE SÉQUENCEUR ===
    auto gridArea = area.reduced(0, 10);
    
    // Positionner le composant SequencerGrid
    sequencerGrid.setBounds(gridArea);
}
