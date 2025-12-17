// DrumBox/juce/standalone/Source/components/transportBar/TransportBar.h

#pragma once
#include <JuceHeader.h>
#include <functional>

/**
 * @brief Barre de transport pour contrôler la lecture et les paramètres globaux
 * 
 * Contient :
 * - Bouton Play/Stop
 * - Contrôle BPM (tempo)
 * - Contrôle Master (volume global)
 */
class TransportBar : public juce::Component
{
public:
    TransportBar();
    ~TransportBar() override = default;

    // === Callbacks ===
    
    /** Appelé quand l'état Play/Stop change */
    std::function<void(bool playing)> onPlayChanged;
    
    /** Appelé quand le BPM change */
    std::function<void(float bpm)> onBpmChanged;
    
    /** Appelé quand le Master gain change */
    std::function<void(float gain)> onMasterChanged;

    // === Setters (pour synchroniser l'UI depuis l'extérieur) ===
    
    /**
     * @brief Met à jour l'état du bouton Play/Stop
     * @param playing true si en lecture, false si arrêté
     */
    void setPlaying(bool playing);
    
    /**
     * @brief Met à jour le BPM affiché
     * @param bpm Tempo en battements par minute
     */
    void setBpm(float bpm);
    
    /**
     * @brief Met à jour le gain master affiché
     * @param gain Gain entre 0.0 et 1.0
     */
    void setMasterGain(float gain);

    // === Overrides JUCE ===
    
    void resized() override;

private:
    // Composants UI
    juce::TextButton playButton;
    juce::Slider bpmSlider;
    juce::Label bpmLabel;
    juce::Slider masterSlider;
    juce::Label masterLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)
};
