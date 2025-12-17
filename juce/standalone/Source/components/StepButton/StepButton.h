// DrumBox/juce/standalone/Source/components/stepButton/StepButton.h

#pragma once
#include <JuceHeader.h>

/**
 * @brief Bouton de step pour le séquenceur DrumBox
 * 
 * Affiche un step individuel dans la grille 3x16 avec :
 * - État ON/OFF (toggle)
 * - Indicateur de playhead (step en cours de lecture)
 * - Effet hover au survol
 * - Style visuel moderne avec dégradés et LED
 */
class StepButton : public juce::ToggleButton
{
public:
    StepButton() = default;
    ~StepButton() override = default;

    /**
     * @brief Active/désactive l'indicateur de playhead
     * @param isPlayhead true si ce step est actuellement en cours de lecture
     */
    void setPlayhead(bool isPlayhead);

    /**
     * @brief Vérifie si le step est actif (ON)
     * @return true si le step est activé
     */
    bool isActive() const { return getToggleState(); }

    // Overrides JUCE
    void mouseEnter(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

private:
    bool playhead = false;  ///< Indicateur de playhead actif
    bool hover = false;     ///< État de survol de la souris

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StepButton)
};
