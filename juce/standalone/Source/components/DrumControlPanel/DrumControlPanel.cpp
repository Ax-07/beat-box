// DrumBox/juce/standalone/Source/components/DrumControlPanel/DrumControlPanel.cpp

#include "DrumControlPanel.h"

#include <cmath>
#include <algorithm>
#include <vector>

using namespace DrumBoxConstants;

namespace
{
    static inline double clampMin(double v, double mn) { return (v < mn) ? mn : v; }

    // Mappe un decay "one-pole" (y[n]=a*y[n-1]) vers un temps en ms.
    // endLevel = 0.001 correspond à -60 dB.
    static inline double decayMsFromCoeff(double a, double sampleRate, double endLevel = 0.001)
    {
        a = std::clamp(a, 1.0e-9, 0.999999999);
        sampleRate = clampMin(sampleRate, 8000.0);

        const double N = std::log(endLevel) / std::log(a); // samples
        const double ms = (N / sampleRate) * 1000.0;
        return std::clamp(ms, 1.0, 10000.0);
    }

    static inline double decayCoeffFromMs(double ms, double sampleRate, double endLevel = 0.001)
    {
        ms = std::clamp(ms, 1.0, 10000.0);
        sampleRate = clampMin(sampleRate, 8000.0);

        const double N = sampleRate * (ms / 1000.0);
        const double a = std::exp(std::log(endLevel) / N);
        return std::clamp(a, 0.0, 0.999999999);
    }

    // Attack de EnvelopeADExp: l'erreur (1-value) est multipliée par attack à chaque sample.
    // On mappe donc un temps en ms vers le même coefficient one-pole, mais on autorise < 1ms.
    static inline double attackMsFromCoeff(double a, double sampleRate, double endLevel = 0.001)
    {
        a = std::clamp(a, 1.0e-9, 0.999999999);
        sampleRate = clampMin(sampleRate, 8000.0);

        const double N = std::log(endLevel) / std::log(a); // samples
        const double ms = (N / sampleRate) * 1000.0;
        return std::clamp(ms, 0.01, 1000.0);
    }

    static inline double attackCoeffFromMs(double ms, double sampleRate, double endLevel = 0.001)
    {
        ms = std::clamp(ms, 0.01, 1000.0);
        sampleRate = clampMin(sampleRate, 8000.0);

        const double N = sampleRate * (ms / 1000.0);
        const double a = std::exp(std::log(endLevel) / N);
        return std::clamp(a, 0.0, 0.999999999);
    }

    static inline double dbFromLinear(double lin)
    {
        lin = std::max(1.0e-9, lin);
        return 20.0 * std::log10(lin);
    }

    static inline double linearFromDb(double db)
    {
        return std::pow(10.0, db / 20.0);
    }
}

void DrumControlPanel::setSampleRate(double sampleRate)
{
    currentSampleRate = std::max(8000.0, sampleRate);
}

DrumControlPanel::DrumControlPanel()
{
    auto initGroup = [this](juce::GroupComponent& group, const juce::String& text)
    {
        addAndMakeVisible(group);
        group.setText(text);
        group.setColour(juce::GroupComponent::textColourId, juce::Colours::white);
        group.setColour(juce::GroupComponent::outlineColourId, Colors::separator);
    };

    initGroup(kickOscGroup, "KICK - OSC");
    initGroup(kickClickGroup, "KICK - CLICK");
    initGroup(kickPostGroup, "KICK - POST");
    initGroup(kickChain1Group, "KICK - CHAIN 1");
    initGroup(kickChain2Group, "KICK - CHAIN 2");
    initGroup(kickTokCrunchGroup, "KICK - TOK / CRUNCH");
    initGroup(kickBassGroup, "KICK - BASS");
    initGroup(kickLayer1Group, "KICK - LAYER 1");
    initGroup(kickLayer2Group, "KICK - LAYER 2");
    initGroup(kickLfoGroup, "KICK - LFO");
    initGroup(kickFxGroup, "KICK - FX");
        initGroup(kickReverbGroup, "KICK - REVERB"); // Added KICK - REVERB group
        initGroup(masterGroup, "MASTER"); // Added MASTER group
    initGroup(snareGroup, "SNARE");
    initGroup(hatGroup, "HAT");

    // Mode UI (Kick): PERF <-> SOUND
    addAndMakeVisible(uiModeButton);
    uiModeButton.setClickingTogglesState(true);
    uiModeButton.setToggleState(false, juce::dontSendNotification);
    uiModeButton.onClick = [this]()
    {
        soundDesignMode = uiModeButton.getToggleState();
        uiModeButton.setButtonText(soundDesignMode ? "Mode: SOUND" : "Mode: PERF");
        updateVisibility();
        resized();
    };

    // Presets kick
    addAndMakeVisible(kickPresetLabel);
    kickPresetLabel.setText("Preset", juce::dontSendNotification);
    kickPresetLabel.setJustificationType(juce::Justification::centredLeft);
    kickPresetLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    addAndMakeVisible(kickPresetBox);
    kickPresetBox.addItem("Gabber", 1);
    kickPresetBox.addItem("Hardstyle", 2);
    kickPresetBox.addItem("Tribecore", 3);
    kickPresetBox.setSelectedId(2, juce::dontSendNotification);

    addAndMakeVisible(kickClipModeLabel);
    kickClipModeLabel.setText("Clip", juce::dontSendNotification);
    kickClipModeLabel.setJustificationType(juce::Justification::centredLeft);
    kickClipModeLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    addAndMakeVisible(kickClipModeBox);
    kickClipModeBox.addItem("Tanh", 1);
    kickClipModeBox.addItem("Hard", 2);
    kickClipModeBox.addItem("Fold", 3);
    kickClipModeBox.setSelectedId(1, juce::dontSendNotification);
    kickClipModeBox.onChange = [this]() {
        if (onKickClipModeChanged)
            onKickClipModeChanged(kickClipModeBox.getSelectedId() - 1);
    };

    // Types de disto par chaîne (Kick)
    addAndMakeVisible(kickChain1TypeLabel);
    kickChain1TypeLabel.setText("C1 Type", juce::dontSendNotification);
    kickChain1TypeLabel.setJustificationType(juce::Justification::centredLeft);
    kickChain1TypeLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    addAndMakeVisible(kickChain1TypeBox);
    kickChain1TypeBox.addItem("Global", 1);
    kickChain1TypeBox.addItem("Tanh", 2);
    kickChain1TypeBox.addItem("Hard", 3);
    kickChain1TypeBox.addItem("Fold", 4);
    kickChain1TypeBox.setSelectedId(1, juce::dontSendNotification);
    kickChain1TypeBox.onChange = [this]() {
        if (onKickChain1ClipModeChanged)
            onKickChain1ClipModeChanged(kickChain1TypeBox.getSelectedId() == 1 ? -1 : (kickChain1TypeBox.getSelectedId() - 2));
    };

    addAndMakeVisible(kickChain2TypeLabel);
    kickChain2TypeLabel.setText("C2 Type", juce::dontSendNotification);
    kickChain2TypeLabel.setJustificationType(juce::Justification::centredLeft);
    kickChain2TypeLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    addAndMakeVisible(kickChain2TypeBox);
    kickChain2TypeBox.addItem("Global", 1);
    kickChain2TypeBox.addItem("Tanh", 2);
    kickChain2TypeBox.addItem("Hard", 3);
    kickChain2TypeBox.addItem("Fold", 4);
    kickChain2TypeBox.setSelectedId(1, juce::dontSendNotification);
    kickChain2TypeBox.onChange = [this]() {
        if (onKickChain2ClipModeChanged)
            onKickChain2ClipModeChanged(kickChain2TypeBox.getSelectedId() == 1 ? -1 : (kickChain2TypeBox.getSelectedId() - 2));
    };

    // Setup sliders communs - Decay (valeurs correctes pour le DSP)
    // Affiché dans la section du drum sélectionné (Kick->OSC, Snare->SNARE, Hat->HAT)
    setupSlider(kickOscGroup, decaySlider, decayLabel, "Decay");
    // UI en millisecondes (converti vers coefficient DSP)
    decaySlider.setRange(5.0, 3000.0, 1.0);
    decaySlider.setValue(decayMsFromCoeff(0.9995, currentSampleRate), juce::dontSendNotification); // Kick default
    decaySlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v)) + " ms";
    };
    decaySlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    decaySlider.onValueChange = [this]() {
        if (onDecayChanged)
            onDecayChanged(selectedDrum, (float)decayCoeffFromMs(decaySlider.getValue(), currentSampleRate));
    };

    // Setup Kick
    setupSlider(kickOscGroup, kickAttackSlider, kickAttackLabel, "Attack Hz");
    kickAttackSlider.setRange(40.0, 300.0, 1.0);
    kickAttackSlider.setValue(120.0);
    kickAttackSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v)) + " Hz";
    };
    kickAttackSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickAttackSlider.onValueChange = [this]() {
        if (onKickAttackChanged)
            onKickAttackChanged(static_cast<float>(kickAttackSlider.getValue()));
    };

    setupSlider(kickOscGroup, kickPitchSlider, kickPitchLabel, "Base Hz");
    kickPitchSlider.setRange(30.0, 120.0, 1.0);
    kickPitchSlider.setValue(55.0);
    kickPitchSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v)) + " Hz";
    };
    kickPitchSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickPitchSlider.onValueChange = [this]() {
        if (onKickPitchChanged)
            onKickPitchChanged(static_cast<float>(kickPitchSlider.getValue()));
    };

    setupSlider(kickOscGroup, kickPitchDecaySlider, kickPitchDecayLabel, "Pitch ms");
    // UI en ms (converti vers coefficient DSP)
    kickPitchDecaySlider.setRange(5.0, 2000.0, 1.0);
    kickPitchDecaySlider.setValue(decayMsFromCoeff(0.9930, currentSampleRate), juce::dontSendNotification);
    kickPitchDecaySlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v)) + " ms";
    };
    kickPitchDecaySlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickPitchDecaySlider.onValueChange = [this]() {
        if (onKickPitchDecayChanged)
            onKickPitchDecayChanged((float)decayCoeffFromMs(kickPitchDecaySlider.getValue(), currentSampleRate));
    };

    setupSlider(kickOscGroup, kickDriveSlider, kickDriveLabel, "Drive");
    kickDriveSlider.setRange(1.0, 25.0, 0.1);
    kickDriveSlider.setValue(14.0);
    kickDriveSlider.textFromValueFunction = [](double v) {
        return juce::String(v, 1);
    };
    kickDriveSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickDriveSlider.onValueChange = [this]() {
        if (onKickDriveChanged)
            onKickDriveChanged((float)kickDriveSlider.getValue());
    };

    setupSlider(kickOscGroup, kickDriveDecaySlider, kickDriveDecayLabel, "Drive ms");
    // UI en ms (converti vers coefficient DSP)
    kickDriveDecaySlider.setRange(5.0, 2000.0, 1.0);
    kickDriveDecaySlider.setValue(decayMsFromCoeff(0.9900, currentSampleRate), juce::dontSendNotification);
    kickDriveDecaySlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v)) + " ms";
    };
    kickDriveDecaySlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickDriveDecaySlider.onValueChange = [this]() {
        if (onKickDriveDecayChanged)
            onKickDriveDecayChanged((float)decayCoeffFromMs(kickDriveDecaySlider.getValue(), currentSampleRate));
    };

    setupSlider(kickClickGroup, kickClickSlider, kickClickLabel, "Click dB");
    // UI en dB (converti vers gain linéaire)
    kickClickSlider.setRange(-60.0, 6.0, 0.1);
    kickClickSlider.setValue(dbFromLinear(0.70), juce::dontSendNotification);
    kickClickSlider.textFromValueFunction = [](double v) {
        return juce::String(v, 1) + " dB";
    };
    kickClickSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.-").getDoubleValue();
    };
    kickClickSlider.onValueChange = [this]() {
        if (onKickClickChanged)
            onKickClickChanged((float)linearFromDb(kickClickSlider.getValue()));
    };

    setupSlider(kickClickGroup, kickHpSlider, kickHpLabel, "HPF Hz");
    kickHpSlider.setRange(10.0, 90.0, 1.0);
    kickHpSlider.setValue(30.0);
    kickHpSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v)) + " Hz";
    };
    kickHpSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickHpSlider.onValueChange = [this]() {
        if (onKickHpChanged)
            onKickHpChanged((float)kickHpSlider.getValue());
    };

    setupSlider(kickPostGroup, kickPostGainSlider, kickPostGainLabel, "Post dB");
    // UI en dB (converti vers gain linéaire)
    kickPostGainSlider.setRange(-60.0, 6.0, 0.1);
    kickPostGainSlider.setValue(dbFromLinear(0.85), juce::dontSendNotification);
    kickPostGainSlider.textFromValueFunction = [](double v) {
        return juce::String(v, 1) + " dB";
    };
    kickPostGainSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.-").getDoubleValue();
    };
    kickPostGainSlider.onValueChange = [this]() {
        if (onKickPostGainChanged)
            onKickPostGainChanged((float)linearFromDb(kickPostGainSlider.getValue()));
    };

    setupSlider(kickPostGroup, kickPostHpSlider, kickPostHpLabel, "PostHP Hz");
    kickPostHpSlider.setRange(10.0, 200.0, 1.0);
    kickPostHpSlider.setSkewFactorFromMidPoint(35.0);
    kickPostHpSlider.setValue(25.0);
    kickPostHpSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v)) + " Hz";
    };
    kickPostHpSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickPostHpSlider.onValueChange = [this]() {
        if (onKickPostHpChanged)
            onKickPostHpChanged((float)kickPostHpSlider.getValue());
    };

    setupSlider(kickPostGroup, kickPostLpSlider, kickPostLpLabel, "PostLP Hz");
    kickPostLpSlider.setRange(200.0, 20000.0, 10.0);
    kickPostLpSlider.setSkewFactorFromMidPoint(2000.0);
    kickPostLpSlider.setValue(8000.0);
    kickPostLpSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v)) + " Hz";
    };
    kickPostLpSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickPostLpSlider.onValueChange = [this]() {
        if (onKickPostLpChanged)
            onKickPostLpChanged((float)kickPostLpSlider.getValue());
    };

    // Oversampling (Kick)
    setupSlider(kickPostGroup, kickOversampleSlider, kickOversampleLabel, "OS x");
    kickOversampleSlider.setRange(1.0, 2.0, 1.0);
    kickOversampleSlider.setValue(1.0, juce::dontSendNotification);
    kickOversampleSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v)) + "x";
    };
    kickOversampleSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789").getIntValue();
    };
    kickOversampleSlider.onValueChange = [this]() {
        if (onKickOversample2xChanged)
            onKickOversample2xChanged(kickOversampleSlider.getValue() >= 2.0 ? 1.0f : 0.0f);
    };

    // === Chain 1 ===
    setupSlider(kickChain1Group, kickChain1MixSlider, kickChain1MixLabel, "C1 Mix %");
    kickChain1MixSlider.setRange(0.0, 1.0, 0.01);
    kickChain1MixSlider.setValue(0.70);
    kickChain1MixSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v * 100.0)) + "%";
    };
    kickChain1MixSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    kickChain1MixSlider.onValueChange = [this]() {
        if (onKickChain1MixChanged)
            onKickChain1MixChanged((float)kickChain1MixSlider.getValue());
    };

    setupSlider(kickChain1Group, kickChain1DriveMulSlider, kickChain1DriveMulLabel, "C1 Drive x");
    kickChain1DriveMulSlider.setRange(0.50, 4.00, 0.01);
    kickChain1DriveMulSlider.setValue(1.00);
    kickChain1DriveMulSlider.textFromValueFunction = [](double v) {
        return juce::String(v, 2) + "x";
    };
    kickChain1DriveMulSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickChain1DriveMulSlider.onValueChange = [this]() {
        if (onKickChain1DriveMulChanged)
            onKickChain1DriveMulChanged((float)kickChain1DriveMulSlider.getValue());
    };

    setupSlider(kickChain1Group, kickChain1LpSlider, kickChain1LpLabel, "C1 LP Hz");
    kickChain1LpSlider.setRange(500.0, 20000.0, 10.0);
    kickChain1LpSlider.setSkewFactorFromMidPoint(3000.0);
    kickChain1LpSlider.setValue(9000.0);
    kickChain1LpSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v)) + " Hz";
    };
    kickChain1LpSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickChain1LpSlider.onValueChange = [this]() {
        if (onKickChain1LpHzChanged)
            onKickChain1LpHzChanged((float)kickChain1LpSlider.getValue());
    };

    setupSlider(kickChain1Group, kickChain1AsymSlider, kickChain1AsymLabel, "C1 Asym");
    kickChain1AsymSlider.setRange(-1.0, 1.0, 0.01);
    kickChain1AsymSlider.setValue(0.0);
    kickChain1AsymSlider.textFromValueFunction = [](double v) {
        return juce::String(v, 2);
    };
    kickChain1AsymSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.-").getDoubleValue();
    };
    kickChain1AsymSlider.onValueChange = [this]() {
        if (onKickChain1AsymChanged)
            onKickChain1AsymChanged((float)kickChain1AsymSlider.getValue());
    };

    // === Chain 2 ===
    setupSlider(kickChain2Group, kickChain2MixSlider, kickChain2MixLabel, "C2 Mix %");
    kickChain2MixSlider.setRange(0.0, 1.0, 0.01);
    kickChain2MixSlider.setValue(0.30);
    kickChain2MixSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v * 100.0)) + "%";
    };
    kickChain2MixSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    kickChain2MixSlider.onValueChange = [this]() {
        if (onKickChain2MixChanged)
            onKickChain2MixChanged((float)kickChain2MixSlider.getValue());
    };

    setupSlider(kickChain2Group, kickChain2DriveMulSlider, kickChain2DriveMulLabel, "C2 Drive x");
    kickChain2DriveMulSlider.setRange(0.50, 4.00, 0.01);
    kickChain2DriveMulSlider.setValue(1.60);
    kickChain2DriveMulSlider.textFromValueFunction = [](double v) {
        return juce::String(v, 2) + "x";
    };
    kickChain2DriveMulSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickChain2DriveMulSlider.onValueChange = [this]() {
        if (onKickChain2DriveMulChanged)
            onKickChain2DriveMulChanged((float)kickChain2DriveMulSlider.getValue());
    };

    setupSlider(kickChain2Group, kickChain2LpSlider, kickChain2LpLabel, "C2 LP Hz");
    kickChain2LpSlider.setRange(500.0, 20000.0, 10.0);
    kickChain2LpSlider.setSkewFactorFromMidPoint(2500.0);
    kickChain2LpSlider.setValue(5200.0);
    kickChain2LpSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v)) + " Hz";
    };
    kickChain2LpSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickChain2LpSlider.onValueChange = [this]() {
        if (onKickChain2LpHzChanged)
            onKickChain2LpHzChanged((float)kickChain2LpSlider.getValue());
    };

    setupSlider(kickChain2Group, kickChain2AsymSlider, kickChain2AsymLabel, "C2 Asym");
    kickChain2AsymSlider.setRange(-1.0, 1.0, 0.01);
    kickChain2AsymSlider.setValue(0.20);
    kickChain2AsymSlider.textFromValueFunction = [](double v) {
        return juce::String(v, 2);
    };
    kickChain2AsymSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.-").getDoubleValue();
    };
    kickChain2AsymSlider.onValueChange = [this]() {
        if (onKickChain2AsymChanged)
            onKickChain2AsymChanged((float)kickChain2AsymSlider.getValue());
    };

    // === TOK / CRUNCH ===
    setupSlider(kickTokCrunchGroup, kickTokAmountSlider, kickTokAmountLabel, "TOK %");
    kickTokAmountSlider.setRange(0.0, 1.0, 0.01);
    kickTokAmountSlider.setValue(0.20);
    kickTokAmountSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v * 100.0)) + "%";
    };
    kickTokAmountSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    kickTokAmountSlider.onValueChange = [this]() {
        if (onKickTokAmountChanged)
            onKickTokAmountChanged((float)kickTokAmountSlider.getValue());
    };

    setupSlider(kickTokCrunchGroup, kickTokHpSlider, kickTokHpLabel, "TOK HP Hz");
    kickTokHpSlider.setRange(50.0, 1000.0, 1.0);
    kickTokHpSlider.setSkewFactorFromMidPoint(220.0);
    kickTokHpSlider.setValue(180.0);
    kickTokHpSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v)) + " Hz";
    };
    kickTokHpSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickTokHpSlider.onValueChange = [this]() {
        if (onKickTokHpHzChanged)
            onKickTokHpHzChanged((float)kickTokHpSlider.getValue());
    };

    setupSlider(kickTokCrunchGroup, kickCrunchAmountSlider, kickCrunchAmountLabel, "Crunch %");
    kickCrunchAmountSlider.setRange(0.0, 1.0, 0.01);
    kickCrunchAmountSlider.setValue(0.15);
    kickCrunchAmountSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v * 100.0)) + "%";
    };
    kickCrunchAmountSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    kickCrunchAmountSlider.onValueChange = [this]() {
        if (onKickCrunchAmountChanged)
            onKickCrunchAmountChanged((float)kickCrunchAmountSlider.getValue());
    };

    // === Kickbass knobs ===
    setupSlider(kickBassGroup, kickTailDecaySlider, kickTailDecayLabel, "Tail ms");
    kickTailDecaySlider.setRange(20.0, 2000.0, 1.0);
    kickTailDecaySlider.setValue(decayMsFromCoeff(0.9992, currentSampleRate), juce::dontSendNotification);
    kickTailDecaySlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v)) + " ms";
    };
    kickTailDecaySlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickTailDecaySlider.onValueChange = [this]() {
        if (onKickTailDecayChanged)
            onKickTailDecayChanged((float)decayCoeffFromMs(kickTailDecaySlider.getValue(), currentSampleRate));
    };

    setupSlider(kickBassGroup, kickTailMixSlider, kickTailMixLabel, "Tail %");
    kickTailMixSlider.setRange(0.0, 1.0, 0.01);
    kickTailMixSlider.setValue(0.45);
    kickTailMixSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v * 100.0)) + "%";
    };
    kickTailMixSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    kickTailMixSlider.onValueChange = [this]() {
        if (onKickTailMixChanged)
            onKickTailMixChanged((float)kickTailMixSlider.getValue());
    };

    setupSlider(kickBassGroup, kickTailFreqMulSlider, kickTailFreqMulLabel, "Tail x");
    kickTailFreqMulSlider.setRange(1.0, 4.0, 0.01);
    kickTailFreqMulSlider.setValue(1.0);
    kickTailFreqMulSlider.textFromValueFunction = [](double v) {
        return juce::String(v, 2) + "x";
    };
    kickTailFreqMulSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickTailFreqMulSlider.onValueChange = [this]() {
        if (onKickTailFreqMulChanged)
            onKickTailFreqMulChanged((float)kickTailFreqMulSlider.getValue());
    };

    setupSlider(kickBassGroup, kickSubMixSlider, kickSubMixLabel, "Sub %");
    kickSubMixSlider.setRange(0.0, 1.0, 0.01);
    kickSubMixSlider.setValue(0.35);
    kickSubMixSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v * 100.0)) + "%";
    };
    kickSubMixSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    kickSubMixSlider.onValueChange = [this]() {
        if (onKickSubMixChanged)
            onKickSubMixChanged((float)kickSubMixSlider.getValue());
    };

    setupSlider(kickBassGroup, kickSubLpSlider, kickSubLpLabel, "SubLP Hz");
    kickSubLpSlider.setRange(40.0, 400.0, 1.0);
    kickSubLpSlider.setSkewFactorFromMidPoint(140.0);
    kickSubLpSlider.setValue(180.0);
    kickSubLpSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v)) + " Hz";
    };
    kickSubLpSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickSubLpSlider.onValueChange = [this]() {
        if (onKickSubLpHzChanged)
            onKickSubLpHzChanged((float)kickSubLpSlider.getValue());
    };

    setupSlider(kickBassGroup, kickFeedbackSlider, kickFeedbackLabel, "FB %");
    kickFeedbackSlider.setRange(0.0, 0.5, 0.001);
    kickFeedbackSlider.setValue(0.08);
    kickFeedbackSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v * 100.0)) + "%";
    };
    kickFeedbackSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    kickFeedbackSlider.onValueChange = [this]() {
        if (onKickFeedbackChanged)
            onKickFeedbackChanged((float)kickFeedbackSlider.getValue());
    };

    // === Kick layers (Layer 1) ===
    setupSlider(kickLayer1Group, kickLayer1EnabledSlider, kickLayer1EnabledLabel, "On");
    kickLayer1EnabledSlider.setRange(0.0, 1.0, 1.0);
    kickLayer1EnabledSlider.setValue(0.0);
    kickLayer1EnabledSlider.textFromValueFunction = [](double v) {
        return (v >= 0.5) ? "On" : "Off";
    };
    kickLayer1EnabledSlider.onValueChange = [this]() {
        if (onKickLayer1EnabledChanged)
            onKickLayer1EnabledChanged((float)kickLayer1EnabledSlider.getValue());
    };

    setupSlider(kickLayer1Group, kickLayer1TypeSlider, kickLayer1TypeLabel, "Type");
    kickLayer1TypeSlider.setRange(0.0, 3.0, 1.0);
    kickLayer1TypeSlider.setValue(0.0);
    kickLayer1TypeSlider.textFromValueFunction = [](double v) {
        const int t = (int)std::round(v);
        switch (t) {
            case 0: return juce::String("Sine");
            case 1: return juce::String("Tri");
            case 2: return juce::String("Square");
            default: return juce::String("Noise");
        }
    };
    kickLayer1TypeSlider.onValueChange = [this]() {
        if (onKickLayer1TypeChanged)
            onKickLayer1TypeChanged((float)kickLayer1TypeSlider.getValue());
    };

    setupSlider(kickLayer1Group, kickLayer1FreqSlider, kickLayer1FreqLabel, "Freq Hz");
    kickLayer1FreqSlider.setRange(10.0, 2000.0, 0.1);
    kickLayer1FreqSlider.setSkewFactorFromMidPoint(120.0);
    kickLayer1FreqSlider.setValue(110.0);
    kickLayer1FreqSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v)) + " Hz";
    };
    kickLayer1FreqSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickLayer1FreqSlider.onValueChange = [this]() {
        if (onKickLayer1FreqHzChanged)
            onKickLayer1FreqHzChanged((float)kickLayer1FreqSlider.getValue());
    };

    setupSlider(kickLayer1Group, kickLayer1PhaseSlider, kickLayer1PhaseLabel, "Phase");
    kickLayer1PhaseSlider.setRange(0.0, 1.0, 0.001);
    kickLayer1PhaseSlider.setValue(0.0);
    kickLayer1PhaseSlider.textFromValueFunction = [](double v) {
        return juce::String(v, 3);
    };
    kickLayer1PhaseSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickLayer1PhaseSlider.onValueChange = [this]() {
        if (onKickLayer1Phase01Changed)
            onKickLayer1Phase01Changed((float)kickLayer1PhaseSlider.getValue());
    };

    setupSlider(kickLayer1Group, kickLayer1DriveSlider, kickLayer1DriveLabel, "Drive %");
    kickLayer1DriveSlider.setRange(0.0, 1.0, 0.01);
    kickLayer1DriveSlider.setValue(0.0);
    kickLayer1DriveSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v * 100.0)) + "%";
    };
    kickLayer1DriveSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    kickLayer1DriveSlider.onValueChange = [this]() {
        if (onKickLayer1DriveChanged)
            onKickLayer1DriveChanged((float)kickLayer1DriveSlider.getValue());
    };

    setupSlider(kickLayer1Group, kickLayer1AttackSlider, kickLayer1AttackLabel, "Atk ms");
    kickLayer1AttackSlider.setRange(0.01, 200.0, 0.01);
    kickLayer1AttackSlider.setValue(attackMsFromCoeff(0.05, currentSampleRate), juce::dontSendNotification);
    kickLayer1AttackSlider.textFromValueFunction = [](double v) {
        if (v < 1.0)
            return juce::String(v, 2) + " ms";
        return juce::String((int)std::round(v)) + " ms";
    };
    kickLayer1AttackSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickLayer1AttackSlider.onValueChange = [this]() {
        if (onKickLayer1AttackCoeffChanged)
            onKickLayer1AttackCoeffChanged((float)attackCoeffFromMs(kickLayer1AttackSlider.getValue(), currentSampleRate));
    };

    setupSlider(kickLayer1Group, kickLayer1DecaySlider, kickLayer1DecayLabel, "Dec ms");
    kickLayer1DecaySlider.setRange(5.0, 5000.0, 1.0);
    kickLayer1DecaySlider.setValue(decayMsFromCoeff(0.9992, currentSampleRate), juce::dontSendNotification);
    kickLayer1DecaySlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v)) + " ms";
    };
    kickLayer1DecaySlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickLayer1DecaySlider.onValueChange = [this]() {
        if (onKickLayer1DecayCoeffChanged)
            onKickLayer1DecayCoeffChanged((float)decayCoeffFromMs(kickLayer1DecaySlider.getValue(), currentSampleRate));
    };

    setupSlider(kickLayer1Group, kickLayer1VolSlider, kickLayer1VolLabel, "Vol dB");
    kickLayer1VolSlider.setRange(-60.0, 6.0, 0.1);
    kickLayer1VolSlider.setValue(-60.0);
    kickLayer1VolSlider.textFromValueFunction = [](double v) {
        return juce::String(v, 1) + " dB";
    };
    kickLayer1VolSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.-").getDoubleValue();
    };
    kickLayer1VolSlider.onValueChange = [this]() {
        if (onKickLayer1VolChanged)
            onKickLayer1VolChanged((float)linearFromDb(kickLayer1VolSlider.getValue()));
    };

    // === Kick layers (Layer 2) ===
    setupSlider(kickLayer2Group, kickLayer2EnabledSlider, kickLayer2EnabledLabel, "On");
    kickLayer2EnabledSlider.setRange(0.0, 1.0, 1.0);
    kickLayer2EnabledSlider.setValue(0.0);
    kickLayer2EnabledSlider.textFromValueFunction = [](double v) {
        return (v >= 0.5) ? "On" : "Off";
    };
    kickLayer2EnabledSlider.onValueChange = [this]() {
        if (onKickLayer2EnabledChanged)
            onKickLayer2EnabledChanged((float)kickLayer2EnabledSlider.getValue());
    };

    setupSlider(kickLayer2Group, kickLayer2TypeSlider, kickLayer2TypeLabel, "Type");
    kickLayer2TypeSlider.setRange(0.0, 3.0, 1.0);
    kickLayer2TypeSlider.setValue(1.0);
    kickLayer2TypeSlider.textFromValueFunction = [](double v) {
        const int t = (int)std::round(v);
        switch (t) {
            case 0: return juce::String("Sine");
            case 1: return juce::String("Tri");
            case 2: return juce::String("Square");
            default: return juce::String("Noise");
        }
    };
    kickLayer2TypeSlider.onValueChange = [this]() {
        if (onKickLayer2TypeChanged)
            onKickLayer2TypeChanged((float)kickLayer2TypeSlider.getValue());
    };

    setupSlider(kickLayer2Group, kickLayer2FreqSlider, kickLayer2FreqLabel, "Freq Hz");
    kickLayer2FreqSlider.setRange(10.0, 2000.0, 0.1);
    kickLayer2FreqSlider.setSkewFactorFromMidPoint(220.0);
    kickLayer2FreqSlider.setValue(220.0);
    kickLayer2FreqSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v)) + " Hz";
    };
    kickLayer2FreqSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickLayer2FreqSlider.onValueChange = [this]() {
        if (onKickLayer2FreqHzChanged)
            onKickLayer2FreqHzChanged((float)kickLayer2FreqSlider.getValue());
    };

    setupSlider(kickLayer2Group, kickLayer2PhaseSlider, kickLayer2PhaseLabel, "Phase");
    kickLayer2PhaseSlider.setRange(0.0, 1.0, 0.001);
    kickLayer2PhaseSlider.setValue(0.0);
    kickLayer2PhaseSlider.textFromValueFunction = [](double v) {
        return juce::String(v, 3);
    };
    kickLayer2PhaseSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickLayer2PhaseSlider.onValueChange = [this]() {
        if (onKickLayer2Phase01Changed)
            onKickLayer2Phase01Changed((float)kickLayer2PhaseSlider.getValue());
    };

    setupSlider(kickLayer2Group, kickLayer2DriveSlider, kickLayer2DriveLabel, "Drive %");
    kickLayer2DriveSlider.setRange(0.0, 1.0, 0.01);
    kickLayer2DriveSlider.setValue(0.0);
    kickLayer2DriveSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v * 100.0)) + "%";
    };
    kickLayer2DriveSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    kickLayer2DriveSlider.onValueChange = [this]() {
        if (onKickLayer2DriveChanged)
            onKickLayer2DriveChanged((float)kickLayer2DriveSlider.getValue());
    };

    setupSlider(kickLayer2Group, kickLayer2AttackSlider, kickLayer2AttackLabel, "Atk ms");
    kickLayer2AttackSlider.setRange(0.01, 200.0, 0.01);
    kickLayer2AttackSlider.setValue(attackMsFromCoeff(0.05, currentSampleRate), juce::dontSendNotification);
    kickLayer2AttackSlider.textFromValueFunction = [](double v) {
        if (v < 1.0)
            return juce::String(v, 2) + " ms";
        return juce::String((int)std::round(v)) + " ms";
    };
    kickLayer2AttackSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickLayer2AttackSlider.onValueChange = [this]() {
        if (onKickLayer2AttackCoeffChanged)
            onKickLayer2AttackCoeffChanged((float)attackCoeffFromMs(kickLayer2AttackSlider.getValue(), currentSampleRate));
    };

    setupSlider(kickLayer2Group, kickLayer2DecaySlider, kickLayer2DecayLabel, "Dec ms");
    kickLayer2DecaySlider.setRange(5.0, 5000.0, 1.0);
    kickLayer2DecaySlider.setValue(decayMsFromCoeff(0.9992, currentSampleRate), juce::dontSendNotification);
    kickLayer2DecaySlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v)) + " ms";
    };
    kickLayer2DecaySlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickLayer2DecaySlider.onValueChange = [this]() {
        if (onKickLayer2DecayCoeffChanged)
            onKickLayer2DecayCoeffChanged((float)decayCoeffFromMs(kickLayer2DecaySlider.getValue(), currentSampleRate));
    };

    setupSlider(kickLayer2Group, kickLayer2VolSlider, kickLayer2VolLabel, "Vol dB");
    kickLayer2VolSlider.setRange(-60.0, 6.0, 0.1);
    kickLayer2VolSlider.setValue(-60.0);
    kickLayer2VolSlider.textFromValueFunction = [](double v) {
        return juce::String(v, 1) + " dB";
    };
    kickLayer2VolSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.-").getDoubleValue();
    };
    kickLayer2VolSlider.onValueChange = [this]() {
        if (onKickLayer2VolChanged)
            onKickLayer2VolChanged((float)linearFromDb(kickLayer2VolSlider.getValue()));
    };

    // === Kick LFO ===
    setupSlider(kickLfoGroup, kickLfoAmountSlider, kickLfoAmountLabel, "Amt %");
    kickLfoAmountSlider.setRange(0.0, 1.0, 0.01);
    kickLfoAmountSlider.setValue(0.0);
    kickLfoAmountSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v * 100.0)) + "%";
    };
    kickLfoAmountSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    kickLfoAmountSlider.onValueChange = [this]() {
        if (onKickLfoAmountChanged)
            onKickLfoAmountChanged((float)kickLfoAmountSlider.getValue());
    };

    setupSlider(kickLfoGroup, kickLfoRateSlider, kickLfoRateLabel, "Rate Hz");
    kickLfoRateSlider.setRange(0.1, 30.0, 0.01);
    kickLfoRateSlider.setSkewFactorFromMidPoint(2.0);
    kickLfoRateSlider.setValue(2.0);
    kickLfoRateSlider.textFromValueFunction = [](double v) {
        return juce::String(v, (v < 10.0 ? 2 : 1)) + " Hz";
    };
    kickLfoRateSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickLfoRateSlider.onValueChange = [this]() {
        if (onKickLfoRateHzChanged)
            onKickLfoRateHzChanged((float)kickLfoRateSlider.getValue());
    };

    setupSlider(kickLfoGroup, kickLfoShapeSlider, kickLfoShapeLabel, "Shape");
    kickLfoShapeSlider.setRange(0.0, 2.0, 1.0);
    kickLfoShapeSlider.setValue(0.0);
    kickLfoShapeSlider.textFromValueFunction = [](double v) {
        const int t = (int)std::round(v);
        switch (t) {
            case 1: return juce::String("Tri");
            case 2: return juce::String("Square");
            default: return juce::String("Sine");
        }
    };
    kickLfoShapeSlider.onValueChange = [this]() {
        if (onKickLfoShapeChanged)
            onKickLfoShapeChanged((float)kickLfoShapeSlider.getValue());
    };

    setupSlider(kickLfoGroup, kickLfoTargetSlider, kickLfoTargetLabel, "Target");
    kickLfoTargetSlider.setRange(0.0, 3.0, 1.0);
    kickLfoTargetSlider.setValue(0.0);
    kickLfoTargetSlider.textFromValueFunction = [](double v) {
        const int t = (int)std::round(v);
        switch (t) {
            case 1: return juce::String("Drive");
            case 2: return juce::String("Cutoff");
            case 3: return juce::String("Phase");
            default: return juce::String("Pitch");
        }
    };
    kickLfoTargetSlider.onValueChange = [this]() {
        if (onKickLfoTargetChanged)
            onKickLfoTargetChanged((float)kickLfoTargetSlider.getValue());
    };

    setupSlider(kickLfoGroup, kickLfoPulseSlider, kickLfoPulseLabel, "Pulse %");
    kickLfoPulseSlider.setRange(0.01, 0.99, 0.01);
    kickLfoPulseSlider.setValue(0.5);
    kickLfoPulseSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v * 100.0)) + "%";
    };
    kickLfoPulseSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    kickLfoPulseSlider.onValueChange = [this]() {
        if (onKickLfoPulseChanged)
            onKickLfoPulseChanged((float)kickLfoPulseSlider.getValue());
    };

    // === Kick Reverb ===
    setupSlider(kickReverbGroup, kickReverbAmountSlider, kickReverbAmountLabel, "Amt %");
    kickReverbAmountSlider.setRange(0.0, 1.0, 0.01);
    kickReverbAmountSlider.setValue(0.0);
    kickReverbAmountSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v * 100.0)) + "%";
    };
    kickReverbAmountSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    kickReverbAmountSlider.onValueChange = [this]() {
        if (onKickReverbAmountChanged)
            onKickReverbAmountChanged((float)kickReverbAmountSlider.getValue());
    };

    setupSlider(kickReverbGroup, kickReverbSizeSlider, kickReverbSizeLabel, "Size %");
    kickReverbSizeSlider.setRange(0.0, 1.0, 0.01);
    kickReverbSizeSlider.setValue(0.35);
    kickReverbSizeSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v * 100.0)) + "%";
    };
    kickReverbSizeSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    kickReverbSizeSlider.onValueChange = [this]() {
        if (onKickReverbSizeChanged)
            onKickReverbSizeChanged((float)kickReverbSizeSlider.getValue());
    };

    setupSlider(kickReverbGroup, kickReverbToneSlider, kickReverbToneLabel, "Tone %");
    kickReverbToneSlider.setRange(0.0, 1.0, 0.01);
    kickReverbToneSlider.setValue(0.55);
    kickReverbToneSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v * 100.0)) + "%";
    };
    kickReverbToneSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    kickReverbToneSlider.onValueChange = [this]() {
        if (onKickReverbToneChanged)
            onKickReverbToneChanged((float)kickReverbToneSlider.getValue());
    };

    // === Kick FX ===
    setupSlider(kickFxGroup, kickFxShiftHzSlider, kickFxShiftHzLabel, "Shift Hz");
    kickFxShiftHzSlider.setRange(-2000.0, 2000.0, 1.0);
    kickFxShiftHzSlider.setValue(0.0);
    kickFxShiftHzSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v)) + " Hz";
    };
    kickFxShiftHzSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.-").getDoubleValue();
    };
    kickFxShiftHzSlider.onValueChange = [this]() {
        if (onKickFxShiftHzChanged)
            onKickFxShiftHzChanged((float)kickFxShiftHzSlider.getValue());
    };

    setupSlider(kickFxGroup, kickFxStereoSlider, kickFxStereoLabel, "Width %");
    kickFxStereoSlider.setRange(0.0, 1.0, 0.01);
    kickFxStereoSlider.setValue(0.0);
    kickFxStereoSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v * 100.0)) + "%";
    };
    kickFxStereoSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    kickFxStereoSlider.onValueChange = [this]() {
        if (onKickFxStereoChanged)
            onKickFxStereoChanged((float)kickFxStereoSlider.getValue());
    };

    setupSlider(kickFxGroup, kickFxDiffusionSlider, kickFxDiffusionLabel, "Diffusion %");
    kickFxDiffusionSlider.setRange(0.0, 1.0, 0.01);
    kickFxDiffusionSlider.setValue(0.0);
    kickFxDiffusionSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v * 100.0)) + "%";
    };
    kickFxDiffusionSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    kickFxDiffusionSlider.onValueChange = [this]() {
        if (onKickFxDiffusionChanged)
            onKickFxDiffusionChanged((float)kickFxDiffusionSlider.getValue());
    };

    setupSlider(kickFxGroup, kickFxCleanDirtySlider, kickFxCleanDirtyLabel, "Dirty %");
    kickFxCleanDirtySlider.setRange(0.0, 1.0, 0.01);
    kickFxCleanDirtySlider.setValue(1.0);
    kickFxCleanDirtySlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v * 100.0)) + "%";
    };
    kickFxCleanDirtySlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    kickFxCleanDirtySlider.onValueChange = [this]() {
        if (onKickFxCleanDirtyChanged)
            onKickFxCleanDirtyChanged((float)kickFxCleanDirtySlider.getValue());
    };

    setupSlider(kickFxGroup, kickFxToneSlider, kickFxToneLabel, "Tone %");
    kickFxToneSlider.setRange(0.0, 1.0, 0.01);
    kickFxToneSlider.setValue(0.5);
    kickFxToneSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v * 100.0)) + "%";
    };
    kickFxToneSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    kickFxToneSlider.onValueChange = [this]() {
        if (onKickFxToneChanged)
            onKickFxToneChanged((float)kickFxToneSlider.getValue());
    };

    // === FX Envelope (Att/Dec/Vol) ===
    setupSlider(kickFxGroup, kickFxEnvAttackSlider, kickFxEnvAttackLabel, "Env Att ms");
    kickFxEnvAttackSlider.setRange(0.05, 200.0, 0.05);
    kickFxEnvAttackSlider.setValue(attackMsFromCoeff(0.05, currentSampleRate), juce::dontSendNotification);
    kickFxEnvAttackSlider.textFromValueFunction = [](double v) {
        return juce::String(v, (v < 10.0 ? 2 : 1)) + " ms";
    };
    kickFxEnvAttackSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickFxEnvAttackSlider.onValueChange = [this]() {
        if (onKickFxEnvAttackCoeffChanged)
            onKickFxEnvAttackCoeffChanged((float)attackCoeffFromMs(kickFxEnvAttackSlider.getValue(), currentSampleRate));
    };

    setupSlider(kickFxGroup, kickFxEnvDecaySlider, kickFxEnvDecayLabel, "Env Dec ms");
    kickFxEnvDecaySlider.setRange(5.0, 2000.0, 1.0);
    kickFxEnvDecaySlider.setValue(decayMsFromCoeff(0.995, currentSampleRate), juce::dontSendNotification);
    kickFxEnvDecaySlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v)) + " ms";
    };
    kickFxEnvDecaySlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    kickFxEnvDecaySlider.onValueChange = [this]() {
        if (onKickFxEnvDecayCoeffChanged)
            onKickFxEnvDecayCoeffChanged((float)decayCoeffFromMs(kickFxEnvDecaySlider.getValue(), currentSampleRate));
    };

    setupSlider(kickFxGroup, kickFxEnvVolSlider, kickFxEnvVolLabel, "Env Vol %");
    kickFxEnvVolSlider.setRange(0.0, 1.0, 0.01);
    kickFxEnvVolSlider.setValue(0.0);
    kickFxEnvVolSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v * 100.0)) + "%";
    };
    kickFxEnvVolSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    kickFxEnvVolSlider.onValueChange = [this]() {
        if (onKickFxEnvVolChanged)
            onKickFxEnvVolChanged((float)kickFxEnvVolSlider.getValue());
    };

    setupSlider(kickFxGroup, kickFxDisperseSlider, kickFxDisperseLabel, "Disperse %");
    kickFxDisperseSlider.setRange(0.0, 1.0, 0.01);
    kickFxDisperseSlider.setValue(0.0);
    kickFxDisperseSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v * 100.0)) + "%";
    };
    kickFxDisperseSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    kickFxDisperseSlider.onValueChange = [this]() {
        if (onKickFxDisperseChanged)
            onKickFxDisperseChanged((float)kickFxDisperseSlider.getValue());
    };

    setupSlider(kickFxGroup, kickFxOttAmountSlider, kickFxOttAmountLabel, "OTT %");
    kickFxOttAmountSlider.setRange(0.0, 1.0, 0.01);
    kickFxOttAmountSlider.setValue(0.0);
    kickFxOttAmountSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v * 100.0)) + "%";
    };
    kickFxOttAmountSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    kickFxOttAmountSlider.onValueChange = [this]() {
        if (onKickFxOttAmountChanged)
            onKickFxOttAmountChanged((float)kickFxOttAmountSlider.getValue());
    };

    setupSlider(kickFxGroup, kickFxInflatorSlider, kickFxInflatorLabel, "Inflator %");
    kickFxInflatorSlider.setRange(0.0, 1.0, 0.01);
    kickFxInflatorSlider.setValue(0.0);
    kickFxInflatorSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v * 100.0)) + "%";
    };
    kickFxInflatorSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    kickFxInflatorSlider.onValueChange = [this]() {
        if (onKickFxInflatorChanged)
            onKickFxInflatorChanged((float)kickFxInflatorSlider.getValue());
    };

    setupSlider(kickFxGroup, kickFxInflatorMixSlider, kickFxInflatorMixLabel, "Infl Mix %");
    kickFxInflatorMixSlider.setRange(0.0, 1.0, 0.01);
    kickFxInflatorMixSlider.setValue(0.5);
    kickFxInflatorMixSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v * 100.0)) + "%";
    };
    kickFxInflatorMixSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    kickFxInflatorMixSlider.onValueChange = [this]() {
        if (onKickFxInflatorMixChanged)
            onKickFxInflatorMixChanged((float)kickFxInflatorMixSlider.getValue());
    };

    // === Master EQ + clip ===
    setupSlider(masterGroup, masterEqLowSlider, masterEqLowLabel, "Low dB");
    masterEqLowSlider.setRange(-24.0, 24.0, 0.1);
    masterEqLowSlider.setValue(0.0);
    masterEqLowSlider.textFromValueFunction = [](double v) { return juce::String(v, 1) + " dB"; };
    masterEqLowSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.-").getDoubleValue();
    };
    masterEqLowSlider.onValueChange = [this]() {
        if (onMasterEqLowDbChanged)
            onMasterEqLowDbChanged((float)masterEqLowSlider.getValue());
    };

    setupSlider(masterGroup, masterEqMidSlider, masterEqMidLabel, "Mid dB");
    masterEqMidSlider.setRange(-24.0, 24.0, 0.1);
    masterEqMidSlider.setValue(0.0);
    masterEqMidSlider.textFromValueFunction = [](double v) { return juce::String(v, 1) + " dB"; };
    masterEqMidSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.-").getDoubleValue();
    };
    masterEqMidSlider.onValueChange = [this]() {
        if (onMasterEqMidDbChanged)
            onMasterEqMidDbChanged((float)masterEqMidSlider.getValue());
    };

    setupSlider(masterGroup, masterEqHighSlider, masterEqHighLabel, "High dB");
    masterEqHighSlider.setRange(-24.0, 24.0, 0.1);
    masterEqHighSlider.setValue(0.0);
    masterEqHighSlider.textFromValueFunction = [](double v) { return juce::String(v, 1) + " dB"; };
    masterEqHighSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.-").getDoubleValue();
    };
    masterEqHighSlider.onValueChange = [this]() {
        if (onMasterEqHighDbChanged)
            onMasterEqHighDbChanged((float)masterEqHighSlider.getValue());
    };

    setupSlider(masterGroup, masterClipOnSlider, masterClipOnLabel, "Clip");
    masterClipOnSlider.setRange(0.0, 1.0, 1.0);
    masterClipOnSlider.setValue(1.0);
    masterClipOnSlider.textFromValueFunction = [](double v) { return (v >= 0.5) ? "On" : "Off"; };
    masterClipOnSlider.onValueChange = [this]() {
        if (onMasterClipOnChanged)
            onMasterClipOnChanged(masterClipOnSlider.getValue() >= 0.5 ? 1.0f : 0.0f);
    };

    setupSlider(masterGroup, masterClipModeSlider, masterClipModeLabel, "Mode");
    masterClipModeSlider.setRange(0.0, 1.0, 1.0);
    masterClipModeSlider.setValue(0.0);
    masterClipModeSlider.textFromValueFunction = [](double v) { return (v >= 0.5) ? "Hard" : "Soft"; };
    masterClipModeSlider.onValueChange = [this]() {
        if (onMasterClipModeChanged)
            onMasterClipModeChanged((float)masterClipModeSlider.getValue());
    };

    // Setup Snare
    setupSlider(snareGroup, snareToneSlider, snareToneLabel, "Tone Hz");
    snareToneSlider.setRange(60.0, 400.0, 1.0);
    snareToneSlider.setValue(180.0);
    snareToneSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v)) + " Hz";
    };
    snareToneSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    snareToneSlider.onValueChange = [this]() {
        if (onSnareToneChanged)
            onSnareToneChanged(static_cast<float>(snareToneSlider.getValue()));
    };

    setupSlider(snareGroup, snareNoiseMixSlider, snareNoiseMixLabel, "Noise %");
    snareNoiseMixSlider.setRange(0.0, 1.0, 0.01);
    snareNoiseMixSlider.setValue(0.75);
    snareNoiseMixSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v * 100.0)) + "%";
    };
    snareNoiseMixSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue() / 100.0;
    };
    snareNoiseMixSlider.onValueChange = [this]() {
        if (onSnareNoiseMixChanged)
            onSnareNoiseMixChanged(static_cast<float>(snareNoiseMixSlider.getValue()));
    };

    // Setup Hat
    setupSlider(hatGroup, hatCutoffSlider, hatCutoffLabel, "Cutoff Hz");
    hatCutoffSlider.setRange(1000.0, 12000.0, 100.0);
    hatCutoffSlider.setValue(7000.0);
    hatCutoffSlider.textFromValueFunction = [](double v) {
        return juce::String((int)std::round(v)) + " Hz";
    };
    hatCutoffSlider.valueFromTextFunction = [](const juce::String& s) {
        return (double)s.retainCharacters("0123456789.").getDoubleValue();
    };
    hatCutoffSlider.onValueChange = [this]() {
        if (onHatCutoffChanged)
            onHatCutoffChanged(static_cast<float>(hatCutoffSlider.getValue()));
    };

    // Initialiser la visibilité
    updateVisibility();

    auto applyKickPreset = [this](int presetId) {
        // Valeurs en unités UI: ms / Hz / dB
        // presetId: 1=Gabber, 2=Hardstyle, 3=Tribecore
        if (presetId == 1)
        {
            // Gabber: très agressif
            decaySlider.setValue(420.0, juce::sendNotificationSync);
            kickAttackSlider.setValue(280.0, juce::sendNotificationSync);
            kickPitchSlider.setValue(45.0, juce::sendNotificationSync);
            kickPitchDecaySlider.setValue(28.0, juce::sendNotificationSync);
            kickDriveSlider.setValue(22.0, juce::sendNotificationSync);
            kickDriveDecaySlider.setValue(45.0, juce::sendNotificationSync);
            kickClickSlider.setValue(-6.0, juce::sendNotificationSync);
            kickHpSlider.setValue(35.0, juce::sendNotificationSync);
            kickPostGainSlider.setValue(-3.0, juce::sendNotificationSync);
            kickPostHpSlider.setValue(28.0, juce::sendNotificationSync);
            kickPostLpSlider.setValue(6500.0, juce::sendNotificationSync);
            kickClipModeBox.setSelectedId(3, juce::sendNotificationSync); // Fold

            // Par défaut on suit le mode global pour les 2 chaînes
            kickChain1TypeBox.setSelectedId(1, juce::sendNotificationSync); // Global
            kickChain2TypeBox.setSelectedId(1, juce::sendNotificationSync); // Global

            kickChain1MixSlider.setValue(0.45, juce::sendNotificationSync);
            kickChain1DriveMulSlider.setValue(1.30, juce::sendNotificationSync);
            kickChain1LpSlider.setValue(8500.0, juce::sendNotificationSync);
            kickChain1AsymSlider.setValue(0.05, juce::sendNotificationSync);

            kickChain2MixSlider.setValue(0.55, juce::sendNotificationSync);
            kickChain2DriveMulSlider.setValue(2.20, juce::sendNotificationSync);
            kickChain2LpSlider.setValue(4200.0, juce::sendNotificationSync);
            kickChain2AsymSlider.setValue(0.35, juce::sendNotificationSync);

            kickTokAmountSlider.setValue(0.25, juce::sendNotificationSync);
            kickTokHpSlider.setValue(200.0, juce::sendNotificationSync);
            kickCrunchAmountSlider.setValue(0.35, juce::sendNotificationSync);

            kickTailDecaySlider.setValue(decayMsFromCoeff(0.99935, currentSampleRate), juce::sendNotificationSync);
            kickTailMixSlider.setValue(0.65, juce::sendNotificationSync);
            kickTailFreqMulSlider.setValue(1.0, juce::sendNotificationSync);
            kickSubMixSlider.setValue(0.25, juce::sendNotificationSync);
            kickSubLpSlider.setValue(160.0, juce::sendNotificationSync);
            kickFeedbackSlider.setValue(0.18, juce::sendNotificationSync);
        }
        else if (presetId == 2)
        {
            // Hardstyle: punch + tail plus contrôlée
            decaySlider.setValue(520.0, juce::sendNotificationSync);
            kickAttackSlider.setValue(240.0, juce::sendNotificationSync);
            kickPitchSlider.setValue(52.0, juce::sendNotificationSync);
            kickPitchDecaySlider.setValue(35.0, juce::sendNotificationSync);
            kickDriveSlider.setValue(18.0, juce::sendNotificationSync);
            kickDriveDecaySlider.setValue(60.0, juce::sendNotificationSync);
            kickClickSlider.setValue(-9.0, juce::sendNotificationSync);
            kickHpSlider.setValue(30.0, juce::sendNotificationSync);
            kickPostGainSlider.setValue(-4.5, juce::sendNotificationSync);
            kickPostHpSlider.setValue(24.0, juce::sendNotificationSync);
            kickPostLpSlider.setValue(8000.0, juce::sendNotificationSync);
            kickClipModeBox.setSelectedId(2, juce::sendNotificationSync); // Hard

            kickChain1TypeBox.setSelectedId(1, juce::sendNotificationSync); // Global
            kickChain2TypeBox.setSelectedId(1, juce::sendNotificationSync); // Global

            kickChain1MixSlider.setValue(0.70, juce::sendNotificationSync);
            kickChain1DriveMulSlider.setValue(1.20, juce::sendNotificationSync);
            kickChain1LpSlider.setValue(9000.0, juce::sendNotificationSync);
            kickChain1AsymSlider.setValue(0.05, juce::sendNotificationSync);

            kickChain2MixSlider.setValue(0.30, juce::sendNotificationSync);
            kickChain2DriveMulSlider.setValue(1.60, juce::sendNotificationSync);
            kickChain2LpSlider.setValue(5200.0, juce::sendNotificationSync);
            kickChain2AsymSlider.setValue(0.20, juce::sendNotificationSync);

            kickTokAmountSlider.setValue(0.18, juce::sendNotificationSync);
            kickTokHpSlider.setValue(160.0, juce::sendNotificationSync);
            kickCrunchAmountSlider.setValue(0.18, juce::sendNotificationSync);

            kickTailDecaySlider.setValue(decayMsFromCoeff(0.99925, currentSampleRate), juce::sendNotificationSync);
            kickTailMixSlider.setValue(0.55, juce::sendNotificationSync);
            kickTailFreqMulSlider.setValue(1.0, juce::sendNotificationSync);
            kickSubMixSlider.setValue(0.32, juce::sendNotificationSync);
            kickSubLpSlider.setValue(180.0, juce::sendNotificationSync);
            kickFeedbackSlider.setValue(0.12, juce::sendNotificationSync);
        }
        else
        {
            // Tribecore: plus court, plus click
            decaySlider.setValue(220.0, juce::sendNotificationSync);
            kickAttackSlider.setValue(200.0, juce::sendNotificationSync);
            kickPitchSlider.setValue(55.0, juce::sendNotificationSync);
            kickPitchDecaySlider.setValue(18.0, juce::sendNotificationSync);
            kickDriveSlider.setValue(12.0, juce::sendNotificationSync);
            kickDriveDecaySlider.setValue(35.0, juce::sendNotificationSync);
            kickClickSlider.setValue(-3.0, juce::sendNotificationSync);
            kickHpSlider.setValue(25.0, juce::sendNotificationSync);
            kickPostGainSlider.setValue(-3.0, juce::sendNotificationSync);
            kickPostHpSlider.setValue(35.0, juce::sendNotificationSync);
            kickPostLpSlider.setValue(9500.0, juce::sendNotificationSync);
            kickClipModeBox.setSelectedId(1, juce::sendNotificationSync); // Tanh

            kickChain1TypeBox.setSelectedId(1, juce::sendNotificationSync); // Global
            kickChain2TypeBox.setSelectedId(1, juce::sendNotificationSync); // Global

            kickChain1MixSlider.setValue(0.75, juce::sendNotificationSync);
            kickChain1DriveMulSlider.setValue(1.05, juce::sendNotificationSync);
            kickChain1LpSlider.setValue(10000.0, juce::sendNotificationSync);
            kickChain1AsymSlider.setValue(-0.05, juce::sendNotificationSync);

            kickChain2MixSlider.setValue(0.25, juce::sendNotificationSync);
            kickChain2DriveMulSlider.setValue(1.30, juce::sendNotificationSync);
            kickChain2LpSlider.setValue(6500.0, juce::sendNotificationSync);
            kickChain2AsymSlider.setValue(0.10, juce::sendNotificationSync);

            kickTokAmountSlider.setValue(0.35, juce::sendNotificationSync);
            kickTokHpSlider.setValue(260.0, juce::sendNotificationSync);
            kickCrunchAmountSlider.setValue(0.10, juce::sendNotificationSync);

            kickTailDecaySlider.setValue(decayMsFromCoeff(0.99910, currentSampleRate), juce::sendNotificationSync);
            kickTailMixSlider.setValue(0.40, juce::sendNotificationSync);
            kickTailFreqMulSlider.setValue(1.0, juce::sendNotificationSync);
            kickSubMixSlider.setValue(0.38, juce::sendNotificationSync);
            kickSubLpSlider.setValue(200.0, juce::sendNotificationSync);
            kickFeedbackSlider.setValue(0.06, juce::sendNotificationSync);
        }
    };

    kickPresetBox.onChange = [this, applyKickPreset]() {
        applyKickPreset(kickPresetBox.getSelectedId());

        if (onKickPresetSelected)
            onKickPresetSelected(kickPresetBox.getSelectedId());
    };

    // Applique le preset par défaut (Hardstyle)
    applyKickPreset(kickPresetBox.getSelectedId());
    
    // Déclencher les callbacks initiaux pour initialiser l'engine (après que les callbacks soient connectés dans MainComponent)
    juce::MessageManager::callAsync([this]() {
        // Kick (UI -> core)
        if (onDecayChanged)
            onDecayChanged(0, (float)decayCoeffFromMs(decaySlider.getValue(), currentSampleRate));
        if (onKickAttackChanged)
            onKickAttackChanged((float)kickAttackSlider.getValue());
        if (onKickPitchChanged)
            onKickPitchChanged((float)kickPitchSlider.getValue());
        if (onKickPitchDecayChanged)
            onKickPitchDecayChanged((float)decayCoeffFromMs(kickPitchDecaySlider.getValue(), currentSampleRate));
        if (onKickDriveChanged)
            onKickDriveChanged((float)kickDriveSlider.getValue());
        if (onKickDriveDecayChanged)
            onKickDriveDecayChanged((float)decayCoeffFromMs(kickDriveDecaySlider.getValue(), currentSampleRate));
        if (onKickClickChanged)
            onKickClickChanged((float)linearFromDb(kickClickSlider.getValue()));
        if (onKickHpChanged)
            onKickHpChanged((float)kickHpSlider.getValue());
        if (onKickPostGainChanged)
            onKickPostGainChanged((float)linearFromDb(kickPostGainSlider.getValue()));
        if (onKickPostHpChanged)
            onKickPostHpChanged((float)kickPostHpSlider.getValue());
        if (onKickPostLpChanged)
            onKickPostLpChanged((float)kickPostLpSlider.getValue());
        if (onKickClipModeChanged)
            onKickClipModeChanged(kickClipModeBox.getSelectedId() - 1);

        if (onKickChain1ClipModeChanged)
            onKickChain1ClipModeChanged(kickChain1TypeBox.getSelectedId() == 1 ? -1 : (kickChain1TypeBox.getSelectedId() - 2));
        if (onKickChain2ClipModeChanged)
            onKickChain2ClipModeChanged(kickChain2TypeBox.getSelectedId() == 1 ? -1 : (kickChain2TypeBox.getSelectedId() - 2));

        if (onKickChain1MixChanged)
            onKickChain1MixChanged((float)kickChain1MixSlider.getValue());
        if (onKickChain1DriveMulChanged)
            onKickChain1DriveMulChanged((float)kickChain1DriveMulSlider.getValue());
        if (onKickChain1LpHzChanged)
            onKickChain1LpHzChanged((float)kickChain1LpSlider.getValue());
        if (onKickChain1AsymChanged)
            onKickChain1AsymChanged((float)kickChain1AsymSlider.getValue());

        if (onKickChain2MixChanged)
            onKickChain2MixChanged((float)kickChain2MixSlider.getValue());
        if (onKickChain2DriveMulChanged)
            onKickChain2DriveMulChanged((float)kickChain2DriveMulSlider.getValue());
        if (onKickChain2LpHzChanged)
            onKickChain2LpHzChanged((float)kickChain2LpSlider.getValue());
        if (onKickChain2AsymChanged)
            onKickChain2AsymChanged((float)kickChain2AsymSlider.getValue());

        if (onKickTokAmountChanged)
            onKickTokAmountChanged((float)kickTokAmountSlider.getValue());
        if (onKickTokHpHzChanged)
            onKickTokHpHzChanged((float)kickTokHpSlider.getValue());
        if (onKickCrunchAmountChanged)
            onKickCrunchAmountChanged((float)kickCrunchAmountSlider.getValue());

        if (onKickTailDecayChanged)
            onKickTailDecayChanged((float)decayCoeffFromMs(kickTailDecaySlider.getValue(), currentSampleRate));
        if (onKickTailMixChanged)
            onKickTailMixChanged((float)kickTailMixSlider.getValue());
        if (onKickTailFreqMulChanged)
            onKickTailFreqMulChanged((float)kickTailFreqMulSlider.getValue());
        if (onKickSubMixChanged)
            onKickSubMixChanged((float)kickSubMixSlider.getValue());
        if (onKickSubLpHzChanged)
            onKickSubLpHzChanged((float)kickSubLpSlider.getValue());
        if (onKickFeedbackChanged)
            onKickFeedbackChanged((float)kickFeedbackSlider.getValue());

        // Snare
        if (onDecayChanged)
            onDecayChanged(1, (float)decayCoeffFromMs(decaySlider.getValue(), currentSampleRate));
        if (onSnareToneChanged)
            onSnareToneChanged((float)snareToneSlider.getValue());
        if (onSnareNoiseMixChanged)
            onSnareNoiseMixChanged((float)snareNoiseMixSlider.getValue());

        // Hat
        if (onDecayChanged)
            onDecayChanged(2, (float)decayCoeffFromMs(decaySlider.getValue(), currentSampleRate));
        if (onHatCutoffChanged)
            onHatCutoffChanged((float)hatCutoffSlider.getValue());
    });
}

void DrumControlPanel::setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText)
{
    setupSlider(*this, slider, label, labelText);
}

void DrumControlPanel::setupSlider(juce::Component& parent, juce::Slider& slider, juce::Label& label, const juce::String& labelText)
{
    parent.addAndMakeVisible(slider);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setLookAndFeel(&knobLookAndFeel);

    // Style knob amélioré
    slider.setColour(juce::Slider::thumbColourId, Colors::accent);
    slider.setColour(juce::Slider::rotarySliderFillColourId, Colors::accent);
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff2a2a2a));
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff1a1a1a));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

    parent.addAndMakeVisible(label);
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    label.setFont(juce::Font(10.0f, juce::Font::plain));
}

void DrumControlPanel::setSelectedDrum(int lane)
{
    if (lane >= 0 && lane < kLanes)
    {
        selectedDrum = lane;
        
        // Ajuster la valeur de decay selon le drum sélectionné
        if (lane == 0) // Kick
            decaySlider.setValue(decayMsFromCoeff(0.9995, currentSampleRate), juce::dontSendNotification);
        else if (lane == 1) // Snare
            decaySlider.setValue(decayMsFromCoeff(0.9975, currentSampleRate), juce::dontSendNotification);
        else if (lane == 2) // Hat
            decaySlider.setValue(decayMsFromCoeff(0.96, currentSampleRate), juce::dontSendNotification);
        
        updateVisibility();
        resized();
    }
}

void DrumControlPanel::updateVisibility()
{
    const bool isKick = (selectedDrum == 0);
    const bool isSnare = (selectedDrum == 1);
    const bool isHat = (selectedDrum == 2);

    const bool showSound = (isKick && soundDesignMode);

    // Re-router Decay dans la section active (un seul slider partagé)
    juce::Component* targetParent = nullptr;
    if (isKick)
        targetParent = &kickOscGroup;
    else if (isSnare)
        targetParent = &snareGroup;
    else
        targetParent = &hatGroup;

    if (decaySlider.getParentComponent() != targetParent)
        targetParent->addAndMakeVisible(decaySlider);
    if (decayLabel.getParentComponent() != targetParent)
        targetParent->addAndMakeVisible(decayLabel);

    // Barre de preset (Kick uniquement)
    kickClipModeBox.setVisible(isKick);
    kickClipModeLabel.setVisible(isKick);
    kickPresetBox.setVisible(isKick);
    kickPresetLabel.setVisible(isKick);
    kickChain1TypeBox.setVisible(isKick);
    kickChain1TypeLabel.setVisible(isKick);
    kickChain2TypeBox.setVisible(isKick);
    kickChain2TypeLabel.setVisible(isKick);

    uiModeButton.setVisible(isKick);

    // FX Env: cachée en PERF (mode essentials)
    const bool showFxEnv = showSound;
    kickFxEnvAttackSlider.setVisible(showFxEnv);
    kickFxEnvAttackLabel.setVisible(showFxEnv);
    kickFxEnvDecaySlider.setVisible(showFxEnv);
    kickFxEnvDecayLabel.setVisible(showFxEnv);
    kickFxEnvVolSlider.setVisible(showFxEnv);
    kickFxEnvVolLabel.setVisible(showFxEnv);

    // Groupes
    kickOscGroup.setVisible(isKick);
    kickClickGroup.setVisible(isKick);
    kickPostGroup.setVisible(isKick);
    kickChain1Group.setVisible(isKick);
    kickChain2Group.setVisible(isKick);
    kickTokCrunchGroup.setVisible(isKick);
    kickBassGroup.setVisible(isKick);
    kickLayer1Group.setVisible(showSound);
    kickLayer2Group.setVisible(showSound);
    kickLfoGroup.setVisible(showSound);
    kickReverbGroup.setVisible(isKick);
    kickFxGroup.setVisible(isKick);
    masterGroup.setVisible(isKick);
    snareGroup.setVisible(isSnare);
    hatGroup.setVisible(isHat);
}

int DrumControlPanel::getPreferredHeightForWidth(int width) const
{
    // Doit rester cohérent avec DrumControlPanel::resized()
    const int outerMargin = 8;
    const int modeBarH = (selectedDrum == 0 ? (22 + 4) : 0);
    const int presetBarH = (selectedDrum == 0 ? (46 + 4) : 0);

    const int knobSize = 50;
    const int labelHeight = 12;
    const int spacing = 4;
    const int rowSpacing = 6;
    const int groupSpacing = 4;
    const int columnSpacing = 6;

    const int groupMargin = 6;
    const int groupTitleMargin = 16;

    auto groupHeightFor = [&](int columnWidth, int itemCount) -> int
    {
        const int probeW = std::max(1, columnWidth - 2 * groupMargin);
        const int perRow = std::max(1, (probeW + spacing) / (knobSize + spacing));
        const int rows = std::max(1, (itemCount + perRow - 1) / perRow);
        const int contentH = rows * (knobSize + labelHeight) + (rows - 1) * rowSpacing;
        return groupMargin * 2 + groupTitleMargin + contentH + groupSpacing;
    };

    const int usableW = std::max(1, width - 2 * outerMargin);

    if (selectedDrum == 0)
    {
        const int colW = std::max(1, (usableW - 2 * columnSpacing) / 3);

        const bool showSound = soundDesignMode;

        int col1H = modeBarH + presetBarH;
        col1H += groupHeightFor(colW, 6); // kickOscGroup
        col1H += groupHeightFor(colW, 2); // kickClickGroup
        col1H += groupHeightFor(colW, 4); // kickPostGroup

        int col2H = modeBarH + presetBarH;
        col2H += groupHeightFor(colW, 4); // chain1
        col2H += groupHeightFor(colW, 4); // chain2
        col2H += groupHeightFor(colW, 3); // tok/crunch
        if (!showSound)
            col2H += groupHeightFor(colW, 6); // bass (PERF)

        int col3H = modeBarH + presetBarH;
        if (showSound)
        {
            col3H += groupHeightFor(colW, 8);  // layer1
            col3H += groupHeightFor(colW, 8);  // layer2
            col3H += groupHeightFor(colW, 5);  // lfo
            col3H += groupHeightFor(colW, 6);  // bass
            col3H += groupHeightFor(colW, 12); // fx (avec env)
        }
        else
        {
            col3H += groupHeightFor(colW, 9);  // fx (sans env)
        }
        col3H += groupHeightFor(colW, 3); // reverb
        col3H += groupHeightFor(colW, 5); // master

        return outerMargin * 2 + std::max({ col1H, col2H, col3H });
    }

    // Snare / Hat : 1 colonne
    const int oneColW = usableW;
    if (selectedDrum == 1)
        return outerMargin * 2 + groupHeightFor(oneColW, 3);
    return outerMargin * 2 + groupHeightFor(oneColW, 2);
}

void DrumControlPanel::resized()
{
    auto area = getLocalBounds().reduced(8);

    // Bouton mode (Kick uniquement)
    if (selectedDrum == 0)
    {
        auto modeBar = area.removeFromTop(22);
        uiModeButton.setBounds(modeBar.removeFromRight(130).reduced(2, 1));
        area.removeFromTop(4);
    }

    // Barre de preset (Kick uniquement)
    if (selectedDrum == 0)
    {
        auto bar = area.removeFromTop(46);
        auto row1 = bar.removeFromTop(22);
        auto row2 = bar;

        {
            auto left = row1.removeFromLeft(row1.getWidth() / 2);
            kickPresetLabel.setBounds(left.removeFromLeft(55));
            kickPresetBox.setBounds(left.reduced(3, 0));

            kickClipModeLabel.setBounds(row1.removeFromLeft(40));
            kickClipModeBox.setBounds(row1.reduced(3, 0));
        }

        {
            auto left = row2.removeFromLeft(row2.getWidth() / 2);
            kickChain1TypeLabel.setBounds(left.removeFromLeft(55));
            kickChain1TypeBox.setBounds(left.reduced(3, 0));

            kickChain2TypeLabel.setBounds(row2.removeFromLeft(55));
            kickChain2TypeBox.setBounds(row2.reduced(3, 0));
        }

        area.removeFromTop(4);
    }

    const int knobSize = 50;
    const int labelHeight = 12;
    const int spacing = 4;
    const int rowSpacing = 6;
    const int groupSpacing = 4;
    const int columnSpacing = 6;

    const int groupMargin = 6;
    const int groupTitleMargin = 16;

    struct Pair { juce::Slider* s; juce::Label* l; };

    auto layoutItems = [&](juce::Rectangle<int> contentArea, std::initializer_list<Pair> items)
    {
        const int perRow = std::max(1, (contentArea.getWidth() + spacing) / (knobSize + spacing));

        int y = contentArea.getY();
        auto it = items.begin();
        const auto end = items.end();
        while (it != end)
        {
            int n = 0;
            auto tmp = it;
            for (; tmp != end && n < perRow; ++tmp)
                ++n;

            const int totalW = n * knobSize + (n - 1) * spacing;
            int x = contentArea.getX() + (contentArea.getWidth() - totalW) / 2;

            for (int j = 0; j < n; ++j, ++it)
            {
                it->s->setBounds(x, y, knobSize, knobSize);
                it->l->setBounds(x, y + knobSize, knobSize, labelHeight);
                x += knobSize + spacing;
            }

            y += knobSize + labelHeight + rowSpacing;
        }
    };

    auto layoutGroupIn = [&](juce::Rectangle<int>& columnArea, juce::GroupComponent& group, std::initializer_list<Pair> items)
    {
        if (!group.isVisible())
            return;

        auto probe = columnArea.reduced(groupMargin);
        probe.removeFromTop(groupTitleMargin);
        const int perRow = std::max(1, (probe.getWidth() + spacing) / (knobSize + spacing));
        const int itemCount = (int)items.size();
        const int rows = std::max(1, (itemCount + perRow - 1) / perRow);
        const int contentH = rows * (knobSize + labelHeight) + (rows - 1) * rowSpacing;
        const int groupH = groupMargin * 2 + groupTitleMargin + contentH;

        auto groupArea = columnArea.removeFromTop(groupH);
        group.setBounds(groupArea);

        auto inner = group.getLocalBounds().reduced(groupMargin);
        inner.removeFromTop(groupTitleMargin);
        layoutItems(inner, items);

        columnArea.removeFromTop(groupSpacing);
    };

    if (selectedDrum == 0)
    {
        const bool showSound = soundDesignMode;

        // Kick : 3 colonnes pour éviter des sections trop hautes
        auto col1 = area;
        auto col2 = area;
        auto col3 = area;

        const int colW = (area.getWidth() - 2 * columnSpacing) / 3;
        col1.setWidth(colW);
        col2.removeFromLeft(colW + columnSpacing);
        col2.setWidth(colW);
        col3.removeFromLeft(colW * 2 + 2 * columnSpacing);

        // Colonne 1 : Fondations (OSC / CLICK / POST)
        layoutGroupIn(col1, kickOscGroup, {
            { &decaySlider, &decayLabel },
            { &kickAttackSlider, &kickAttackLabel },
            { &kickPitchSlider, &kickPitchLabel },
            { &kickPitchDecaySlider, &kickPitchDecayLabel },
            { &kickDriveSlider, &kickDriveLabel },
            { &kickDriveDecaySlider, &kickDriveDecayLabel },
        });

        layoutGroupIn(col1, kickClickGroup, {
            { &kickClickSlider, &kickClickLabel },
            { &kickHpSlider, &kickHpLabel },
        });

        layoutGroupIn(col1, kickPostGroup, {
            { &kickPostGainSlider, &kickPostGainLabel },
            { &kickPostHpSlider, &kickPostHpLabel },
            { &kickPostLpSlider, &kickPostLpLabel },
            { &kickOversampleSlider, &kickOversampleLabel },
        });

        // Colonne 2 : Caractère / distorsions / transient
        layoutGroupIn(col2, kickChain1Group, {
            { &kickChain1MixSlider, &kickChain1MixLabel },
            { &kickChain1DriveMulSlider, &kickChain1DriveMulLabel },
            { &kickChain1LpSlider, &kickChain1LpLabel },
            { &kickChain1AsymSlider, &kickChain1AsymLabel },
        });

        layoutGroupIn(col2, kickChain2Group, {
            { &kickChain2MixSlider, &kickChain2MixLabel },
            { &kickChain2DriveMulSlider, &kickChain2DriveMulLabel },
            { &kickChain2LpSlider, &kickChain2LpLabel },
            { &kickChain2AsymSlider, &kickChain2AsymLabel },
        });

        layoutGroupIn(col2, kickTokCrunchGroup, {
            { &kickTokAmountSlider, &kickTokAmountLabel },
            { &kickTokHpSlider, &kickTokHpLabel },
            { &kickCrunchAmountSlider, &kickCrunchAmountLabel },
        });

        // PERF: on met le Bass ici pour avoir un panel "live" plus court
        if (!showSound)
        {
            layoutGroupIn(col2, kickBassGroup, {
                { &kickTailDecaySlider, &kickTailDecayLabel },
                { &kickTailMixSlider, &kickTailMixLabel },
                { &kickTailFreqMulSlider, &kickTailFreqMulLabel },
                { &kickSubMixSlider, &kickSubMixLabel },
                { &kickSubLpSlider, &kickSubLpLabel },
                { &kickFeedbackSlider, &kickFeedbackLabel },
            });
        }

        // Colonne 3 : (SOUND) layers + modulations, puis FX/mix
        if (showSound)
        {
            layoutGroupIn(col3, kickLayer1Group, {
                { &kickLayer1EnabledSlider, &kickLayer1EnabledLabel },
                { &kickLayer1TypeSlider, &kickLayer1TypeLabel },
                { &kickLayer1FreqSlider, &kickLayer1FreqLabel },
                { &kickLayer1PhaseSlider, &kickLayer1PhaseLabel },
                { &kickLayer1DriveSlider, &kickLayer1DriveLabel },
                { &kickLayer1AttackSlider, &kickLayer1AttackLabel },
                { &kickLayer1DecaySlider, &kickLayer1DecayLabel },
                { &kickLayer1VolSlider, &kickLayer1VolLabel },
            });

            layoutGroupIn(col3, kickLayer2Group, {
                { &kickLayer2EnabledSlider, &kickLayer2EnabledLabel },
                { &kickLayer2TypeSlider, &kickLayer2TypeLabel },
                { &kickLayer2FreqSlider, &kickLayer2FreqLabel },
                { &kickLayer2PhaseSlider, &kickLayer2PhaseLabel },
                { &kickLayer2DriveSlider, &kickLayer2DriveLabel },
                { &kickLayer2AttackSlider, &kickLayer2AttackLabel },
                { &kickLayer2DecaySlider, &kickLayer2DecayLabel },
                { &kickLayer2VolSlider, &kickLayer2VolLabel },
            });

            layoutGroupIn(col3, kickLfoGroup, {
                { &kickLfoAmountSlider, &kickLfoAmountLabel },
                { &kickLfoRateSlider, &kickLfoRateLabel },
                { &kickLfoShapeSlider, &kickLfoShapeLabel },
                { &kickLfoTargetSlider, &kickLfoTargetLabel },
                { &kickLfoPulseSlider, &kickLfoPulseLabel },
            });

            layoutGroupIn(col3, kickBassGroup, {
                { &kickTailDecaySlider, &kickTailDecayLabel },
                { &kickTailMixSlider, &kickTailMixLabel },
                { &kickTailFreqMulSlider, &kickTailFreqMulLabel },
                { &kickSubMixSlider, &kickSubMixLabel },
                { &kickSubLpSlider, &kickSubLpLabel },
                { &kickFeedbackSlider, &kickFeedbackLabel },
            });

            layoutGroupIn(col3, kickFxGroup, {
                { &kickFxShiftHzSlider, &kickFxShiftHzLabel },
                { &kickFxDiffusionSlider, &kickFxDiffusionLabel },
                { &kickFxStereoSlider, &kickFxStereoLabel },
                { &kickFxCleanDirtySlider, &kickFxCleanDirtyLabel },
                { &kickFxToneSlider, &kickFxToneLabel },
                { &kickFxEnvAttackSlider, &kickFxEnvAttackLabel },
                { &kickFxEnvDecaySlider, &kickFxEnvDecayLabel },
                { &kickFxEnvVolSlider, &kickFxEnvVolLabel },
                { &kickFxDisperseSlider, &kickFxDisperseLabel },
                { &kickFxOttAmountSlider, &kickFxOttAmountLabel },
                { &kickFxInflatorSlider, &kickFxInflatorLabel },
                { &kickFxInflatorMixSlider, &kickFxInflatorMixLabel },
            });
        }
        else
        {
            // PERF: essentials FX (sans enveloppe)
            layoutGroupIn(col3, kickFxGroup, {
                { &kickFxShiftHzSlider, &kickFxShiftHzLabel },
                { &kickFxDiffusionSlider, &kickFxDiffusionLabel },
                { &kickFxStereoSlider, &kickFxStereoLabel },
                { &kickFxCleanDirtySlider, &kickFxCleanDirtyLabel },
                { &kickFxToneSlider, &kickFxToneLabel },
                { &kickFxDisperseSlider, &kickFxDisperseLabel },
                { &kickFxOttAmountSlider, &kickFxOttAmountLabel },
                { &kickFxInflatorSlider, &kickFxInflatorLabel },
                { &kickFxInflatorMixSlider, &kickFxInflatorMixLabel },
            });
        }

        layoutGroupIn(col3, kickReverbGroup, {
            { &kickReverbAmountSlider, &kickReverbAmountLabel },
            { &kickReverbSizeSlider, &kickReverbSizeLabel },
            { &kickReverbToneSlider, &kickReverbToneLabel },
        });

        layoutGroupIn(col3, masterGroup, {
            { &masterEqLowSlider, &masterEqLowLabel },
            { &masterEqMidSlider, &masterEqMidLabel },
            { &masterEqHighSlider, &masterEqHighLabel },
            { &masterClipOnSlider, &masterClipOnLabel },
            { &masterClipModeSlider, &masterClipModeLabel },
        });
    }
    else if (selectedDrum == 1)
    {
        layoutGroupIn(area, snareGroup, {
            { &decaySlider, &decayLabel },
            { &snareToneSlider, &snareToneLabel },
            { &snareNoiseMixSlider, &snareNoiseMixLabel },
        });
    }
    else
    {
        layoutGroupIn(area, hatGroup, {
            { &decaySlider, &decayLabel },
            { &hatCutoffSlider, &hatCutoffLabel },
        });
    }
}
