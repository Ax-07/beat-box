// DrumBox/juce/standalone/Source/components/drumSelector/DrumSelector.h

#pragma once
#include <JuceHeader.h>
#include "../../utils/Constants.h"
#include <functional>

/**
 * @brief Sélecteur de drum (3 boutons : Kick / Snare / Hat)
 * 
 * Permet de choisir quel drum éditer dans le DrumControlPanel.
 * Un seul bouton peut être sélectionné à la fois.
 */
class DrumSelector : public juce::Component
{
public:
    DrumSelector();
    ~DrumSelector() override = default;

    // === Callbacks ===
    
    /**
     * @brief Appelé quand l'utilisateur sélectionne un drum
     * @param drumIndex Index du drum (0=Kick, 1=Snare, 2=Hat)
     */
    std::function<void(int drumIndex)> onDrumSelected;

    // === Setters ===
    
    /**
     * @brief Sélectionne un drum par programmation
     * @param drumIndex Index du drum (0=Kick, 1=Snare, 2=Hat)
     */
    void selectDrum(int drumIndex);
    
    /**
     * @brief Récupère l'index du drum actuellement sélectionné
     * @return Index du drum (0=Kick, 1=Snare, 2=Hat)
     */
    int getSelectedDrum() const { return selectedDrum; }

    // === Overrides JUCE ===
    
    void resized() override;

private:
    // Composants UI
    juce::TextButton kickButton;
    juce::TextButton snareButton;
    juce::TextButton hatButton;
    
    int selectedDrum = 0; // 0=Kick, 1=Snare, 2=Hat
    
    // Méthode interne pour mettre à jour les couleurs
    void updateButtonColors();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumSelector)
};
