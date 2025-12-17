// DrumBox/juce/standalone/Source/components/DrumControlPanel/DrumControlPanel.h

#pragma once
#include <JuceHeader.h>
#include "../../utils/Constants.h"
#include <functional>

// LookAndFeel personnalisé pour contrôler l'épaisseur des knobs
class ThinKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider &slider) override
    {
        auto bounds = juce::Rectangle<float>(x, y, width, height).reduced(10.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        auto lineW = 2.5f; // Épaisseur de la ligne (réduisez cette valeur pour des knobs plus fins)
        auto arcRadius = radius - lineW * 0.5f;

        // Arc de fond
        juce::Path backgroundArc;
        backgroundArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(),
                                    arcRadius, arcRadius, 0.0f,
                                    rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(slider.findColour(juce::Slider::rotarySliderOutlineColourId));
        g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Arc de valeur
        if (slider.isEnabled())
        {
            juce::Path valueArc;
            valueArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(),
                                   arcRadius, arcRadius, 0.0f,
                                   rotaryStartAngle, toAngle, true);
            g.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId));
            g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Indicateur (petit cercle)
        auto thumbWidth = lineW * 2.0f;
        juce::Point<float> thumbPoint(
            bounds.getCentreX() + arcRadius * std::cos(toAngle - juce::MathConstants<float>::halfPi),
            bounds.getCentreY() + arcRadius * std::sin(toAngle - juce::MathConstants<float>::halfPi));
        g.setColour(slider.findColour(juce::Slider::thumbColourId));
        g.fillEllipse(juce::Rectangle<float>(thumbWidth, thumbWidth).withCentre(thumbPoint));

        // Afficher la valeur au centre du knob
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(9.5f, juce::Font::plain));

        // Utilise le formatage du slider (textFromValueFunction)
        auto valueText = slider.getTextFromValue(slider.getValue());
        if (valueText.isEmpty())
            valueText = juce::String(slider.getValue(), 2);

        // Raccourcit un peu les unités pour éviter le débordement
        valueText = valueText.replace(" Hz", "Hz")
                        .replace(" ms", "ms")
                        .replace(" dB", "dB")
                        .replace(" %", "%")
                        .replace(" x", "x");

        auto textBounds = bounds.toNearestInt().reduced(4);
        g.drawFittedText(valueText, textBounds, juce::Justification::centred, 1, 0.55f);
    }
};

class DrumControlPanel : public juce::Component
{
public:
    DrumControlPanel();
    ~DrumControlPanel() override = default;

    // Hauteur recommandée du contenu (utile quand le panel est dans un Viewport)
    int getPreferredHeightForWidth(int width) const;

    // Nécessaire pour convertir ms <-> coefficient de decay (DSP)
    void setSampleRate(double sampleRate);

    // Callbacks pour changements de paramètres
    std::function<void(int lane, float value)> onDecayChanged;
    std::function<void(float value)> onKickAttackChanged;
    std::function<void(float value)> onKickPitchChanged;
    std::function<void(float value)> onKickPitchDecayChanged;
    std::function<void(float value)> onKickDriveChanged;
    std::function<void(float value)> onKickDriveDecayChanged;
    std::function<void(float value)> onKickClickChanged;
    std::function<void(float value)> onKickHpChanged;
    std::function<void(float value)> onKickPostGainChanged;
    std::function<void(float value)> onKickPostHpChanged;
    std::function<void(float value)> onKickPostLpChanged;
    std::function<void(int mode)> onKickClipModeChanged;
    std::function<void(int presetId)> onKickPresetSelected;

    // knobs (Kick)
    std::function<void(float value)> onKickChain1MixChanged;
    std::function<void(float value)> onKickChain1DriveMulChanged;
    std::function<void(float value)> onKickChain1LpHzChanged;
    std::function<void(float value)> onKickChain1AsymChanged;
    std::function<void(int mode)> onKickChain1ClipModeChanged;

    std::function<void(float value)> onKickChain2MixChanged;
    std::function<void(float value)> onKickChain2DriveMulChanged;
    std::function<void(float value)> onKickChain2LpHzChanged;
    std::function<void(float value)> onKickChain2AsymChanged;
    std::function<void(int mode)> onKickChain2ClipModeChanged;

    std::function<void(float value)> onKickTokAmountChanged;
    std::function<void(float value)> onKickTokHpHzChanged;
    std::function<void(float value)> onKickCrunchAmountChanged;

    // Kickbass controls (Kick)
    std::function<void(float value)> onKickTailDecayChanged;
    std::function<void(float value)> onKickTailMixChanged;
    std::function<void(float value)> onKickTailFreqMulChanged;
    std::function<void(float value)> onKickSubMixChanged;
    std::function<void(float value)> onKickSubLpHzChanged;
    std::function<void(float value)> onKickFeedbackChanged;

    // Kick layers (Layer 1)
    std::function<void(float value)> onKickLayer1EnabledChanged;
    std::function<void(float value)> onKickLayer1TypeChanged;
    std::function<void(float value)> onKickLayer1FreqHzChanged;
    std::function<void(float value)> onKickLayer1Phase01Changed;
    std::function<void(float value)> onKickLayer1DriveChanged;
    std::function<void(float value)> onKickLayer1AttackCoeffChanged;
    std::function<void(float value)> onKickLayer1DecayCoeffChanged;
    std::function<void(float value)> onKickLayer1VolChanged;

    // Kick layers (Layer 2)
    std::function<void(float value)> onKickLayer2EnabledChanged;
    std::function<void(float value)> onKickLayer2TypeChanged;
    std::function<void(float value)> onKickLayer2FreqHzChanged;
    std::function<void(float value)> onKickLayer2Phase01Changed;
    std::function<void(float value)> onKickLayer2DriveChanged;
    std::function<void(float value)> onKickLayer2AttackCoeffChanged;
    std::function<void(float value)> onKickLayer2DecayCoeffChanged;
    std::function<void(float value)> onKickLayer2VolChanged;

    // Kick LFO
    std::function<void(float value)> onKickLfoAmountChanged;
    std::function<void(float value)> onKickLfoRateHzChanged;
    std::function<void(float value)> onKickLfoShapeChanged;
    std::function<void(float value)> onKickLfoTargetChanged;
    std::function<void(float value)> onKickLfoPulseChanged;

    // Kick Reverb + Quality
    std::function<void(float value)> onKickReverbAmountChanged; // 0..1
    std::function<void(float value)> onKickReverbSizeChanged;   // 0..1
    std::function<void(float value)> onKickReverbToneChanged;   // 0..1
    std::function<void(float value)> onKickOversample2xChanged; // 0/1

    // Kick FX
    std::function<void(float value)> onKickFxShiftHzChanged;        // Hz
    std::function<void(float value)> onKickFxStereoChanged;         // 0..1
    std::function<void(float value)> onKickFxDiffusionChanged;      // 0..1
    std::function<void(float value)> onKickFxCleanDirtyChanged;     // 0..1
    std::function<void(float value)> onKickFxToneChanged;           // 0..1
    std::function<void(float value)> onKickFxEnvAttackCoeffChanged; // coeff 0..1
    std::function<void(float value)> onKickFxEnvDecayCoeffChanged;  // coeff 0..1
    std::function<void(float value)> onKickFxEnvVolChanged;         // 0..1
    std::function<void(float value)> onKickFxDisperseChanged;       // 0..1
    std::function<void(float value)> onKickFxInflatorChanged;       // 0..1
    std::function<void(float value)> onKickFxInflatorMixChanged;    // 0..1
    std::function<void(float value)> onKickFxOttAmountChanged;      // 0..1

    // Master
    std::function<void(float value)> onMasterEqLowDbChanged;
    std::function<void(float value)> onMasterEqMidDbChanged;
    std::function<void(float value)> onMasterEqHighDbChanged;
    std::function<void(float value)> onMasterClipOnChanged;   // 0/1
    std::function<void(float value)> onMasterClipModeChanged; // 0/1

    std::function<void(float value)> onSnareToneChanged;
    std::function<void(float value)> onSnareNoiseMixChanged;

    std::function<void(float value)> onHatCutoffChanged;

    // Sélection du drum à afficher
    void setSelectedDrum(int lane);
    int getSelectedDrum() const { return selectedDrum; }

    void resized() override;

private:
    int selectedDrum = 0; // 0=Kick, 1=Snare, 2=Hat

    // UI mode (Kick): Performance = essentials, Sound design = tout
    bool soundDesignMode = false;
    juce::TextButton uiModeButton{"Mode: PERF"};

    double currentSampleRate = 48000.0;

    // Groupes UI (catégories)
    juce::GroupComponent kickOscGroup;
    juce::GroupComponent kickClickGroup; // Added for click separation
    juce::GroupComponent kickPostGroup;
    juce::GroupComponent kickChain1Group;
    juce::GroupComponent kickChain2Group;
    juce::GroupComponent kickTokCrunchGroup;
    juce::GroupComponent kickBassGroup;

    juce::GroupComponent kickLayer1Group;
    juce::GroupComponent kickLayer2Group;

    juce::GroupComponent kickLfoGroup;
    juce::GroupComponent kickReverbGroup;
    juce::GroupComponent kickFxGroup;
    juce::GroupComponent masterGroup;

    juce::GroupComponent snareGroup;
    juce::GroupComponent hatGroup;

    // Sliders communs
    juce::Slider decaySlider;
    juce::Label decayLabel;

    // Sliders Kick
    juce::Slider kickAttackSlider;
    juce::Label kickAttackLabel;
    juce::Slider kickPitchSlider;
    juce::Label kickPitchLabel;
    juce::Slider kickPitchDecaySlider;
    juce::Label kickPitchDecayLabel;
    juce::Slider kickDriveSlider;
    juce::Label kickDriveLabel;
    juce::Slider kickDriveDecaySlider;
    juce::Label kickDriveDecayLabel;
    juce::Slider kickClickSlider;
    juce::Label kickClickLabel;
    juce::Slider kickHpSlider;
    juce::Label kickHpLabel;
    juce::Slider kickPostGainSlider;
    juce::Label kickPostGainLabel;

    // Kick shaping extras
    juce::Slider kickPostHpSlider;
    juce::Label kickPostHpLabel;
    juce::Slider kickPostLpSlider;
    juce::Label kickPostLpLabel;
    juce::ComboBox kickClipModeBox;
    juce::Label kickClipModeLabel;
    juce::ComboBox kickPresetBox;
    juce::Label kickPresetLabel;

    juce::Slider kickOversampleSlider;
    juce::Label kickOversampleLabel;

    // Kick chains + TOK/CRUNCH
    juce::ComboBox kickChain1TypeBox;
    juce::Label kickChain1TypeLabel;

    juce::Slider kickChain1MixSlider;
    juce::Label kickChain1MixLabel;
    juce::Slider kickChain1DriveMulSlider;
    juce::Label kickChain1DriveMulLabel;
    juce::Slider kickChain1LpSlider;
    juce::Label kickChain1LpLabel;
    juce::Slider kickChain1AsymSlider;
    juce::Label kickChain1AsymLabel;

    juce::ComboBox kickChain2TypeBox;
    juce::Label kickChain2TypeLabel;

    juce::Slider kickChain2MixSlider;
    juce::Label kickChain2MixLabel;
    juce::Slider kickChain2DriveMulSlider;
    juce::Label kickChain2DriveMulLabel;
    juce::Slider kickChain2LpSlider;
    juce::Label kickChain2LpLabel;
    juce::Slider kickChain2AsymSlider;
    juce::Label kickChain2AsymLabel;

    juce::Slider kickTokAmountSlider;
    juce::Label kickTokAmountLabel;
    juce::Slider kickTokHpSlider;
    juce::Label kickTokHpLabel;
    juce::Slider kickCrunchAmountSlider;
    juce::Label kickCrunchAmountLabel;

    // Kickbass knobs
    juce::Slider kickTailDecaySlider;
    juce::Label kickTailDecayLabel;
    juce::Slider kickTailMixSlider;
    juce::Label kickTailMixLabel;
    juce::Slider kickTailFreqMulSlider;
    juce::Label kickTailFreqMulLabel;
    juce::Slider kickSubMixSlider;
    juce::Label kickSubMixLabel;
    juce::Slider kickSubLpSlider;
    juce::Label kickSubLpLabel;
    juce::Slider kickFeedbackSlider;
    juce::Label kickFeedbackLabel;

    // Kick layers - Layer 1
    juce::Slider kickLayer1EnabledSlider;
    juce::Label kickLayer1EnabledLabel;
    juce::Slider kickLayer1TypeSlider;
    juce::Label kickLayer1TypeLabel;
    juce::Slider kickLayer1FreqSlider;
    juce::Label kickLayer1FreqLabel;
    juce::Slider kickLayer1PhaseSlider;
    juce::Label kickLayer1PhaseLabel;
    juce::Slider kickLayer1DriveSlider;
    juce::Label kickLayer1DriveLabel;
    juce::Slider kickLayer1AttackSlider;
    juce::Label kickLayer1AttackLabel;
    juce::Slider kickLayer1DecaySlider;
    juce::Label kickLayer1DecayLabel;
    juce::Slider kickLayer1VolSlider;
    juce::Label kickLayer1VolLabel;

    // Kick layers - Layer 2
    juce::Slider kickLayer2EnabledSlider;
    juce::Label kickLayer2EnabledLabel;
    juce::Slider kickLayer2TypeSlider;
    juce::Label kickLayer2TypeLabel;
    juce::Slider kickLayer2FreqSlider;
    juce::Label kickLayer2FreqLabel;
    juce::Slider kickLayer2PhaseSlider;
    juce::Label kickLayer2PhaseLabel;
    juce::Slider kickLayer2DriveSlider;
    juce::Label kickLayer2DriveLabel;
    juce::Slider kickLayer2AttackSlider;
    juce::Label kickLayer2AttackLabel;
    juce::Slider kickLayer2DecaySlider;
    juce::Label kickLayer2DecayLabel;
    juce::Slider kickLayer2VolSlider;
    juce::Label kickLayer2VolLabel;

    // Kick LFO
    juce::Slider kickLfoAmountSlider;
    juce::Label kickLfoAmountLabel;
    juce::Slider kickLfoRateSlider;
    juce::Label kickLfoRateLabel;
    juce::Slider kickLfoShapeSlider;
    juce::Label kickLfoShapeLabel;
    juce::Slider kickLfoTargetSlider;
    juce::Label kickLfoTargetLabel;
    juce::Slider kickLfoPulseSlider;
    juce::Label kickLfoPulseLabel;

    // Kick Reverb
    juce::Slider kickReverbAmountSlider;
    juce::Label kickReverbAmountLabel;
    juce::Slider kickReverbSizeSlider;
    juce::Label kickReverbSizeLabel;
    juce::Slider kickReverbToneSlider;
    juce::Label kickReverbToneLabel;

    // Kick FX
    juce::Slider kickFxShiftHzSlider;
    juce::Label kickFxShiftHzLabel;
    juce::Slider kickFxStereoSlider;
    juce::Label kickFxStereoLabel;
    juce::Slider kickFxDiffusionSlider;
    juce::Label kickFxDiffusionLabel;
    juce::Slider kickFxCleanDirtySlider;
    juce::Label kickFxCleanDirtyLabel;
    juce::Slider kickFxToneSlider;
    juce::Label kickFxToneLabel;
    juce::Slider kickFxEnvAttackSlider;
    juce::Label kickFxEnvAttackLabel;
    juce::Slider kickFxEnvDecaySlider;
    juce::Label kickFxEnvDecayLabel;
    juce::Slider kickFxEnvVolSlider;
    juce::Label kickFxEnvVolLabel;
    juce::Slider kickFxDisperseSlider;
    juce::Label kickFxDisperseLabel;
    juce::Slider kickFxOttAmountSlider;
    juce::Label kickFxOttAmountLabel;
    juce::Slider kickFxInflatorSlider;
    juce::Label kickFxInflatorLabel;
    juce::Slider kickFxInflatorMixSlider;
    juce::Label kickFxInflatorMixLabel;

    // Master (EQ + clip)
    juce::Slider masterEqLowSlider;
    juce::Label masterEqLowLabel;
    juce::Slider masterEqMidSlider;
    juce::Label masterEqMidLabel;
    juce::Slider masterEqHighSlider;
    juce::Label masterEqHighLabel;
    juce::Slider masterClipOnSlider;
    juce::Label masterClipOnLabel;
    juce::Slider masterClipModeSlider;
    juce::Label masterClipModeLabel;

    // Sliders Snare
    juce::Slider snareToneSlider;
    juce::Label snareToneLabel;
    juce::Slider snareNoiseMixSlider;
    juce::Label snareNoiseMixLabel;

    // Sliders Hat
    juce::Slider hatCutoffSlider;
    juce::Label hatCutoffLabel;

    void updateVisibility();
    void setupSlider(juce::Component &parent, juce::Slider &slider, juce::Label &label, const juce::String &labelText);
    void setupSlider(juce::Slider &slider, juce::Label &label, const juce::String &labelText);

    ThinKnobLookAndFeel knobLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumControlPanel)
};
