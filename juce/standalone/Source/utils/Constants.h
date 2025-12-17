// DrumBox/juce/standalone/Source/utils/Constants.h

#pragma once
#include <JuceHeader.h>

/**
 * @brief Constantes globales de l'application DrumBox
 * 
 * Ce fichier centralise toutes les constantes magiques pour faciliter
 * la maintenance et garantir la cohérence visuelle.
 */
namespace DrumBoxConstants
{
    // === SEQUENCEUR ===
    
    /** Nombre de pistes (lanes) dans le séquenceur */
    constexpr int kLanes = 3;
    
    /** Nombre de steps par pattern */
    constexpr int kSteps = 16;
    
    /** Noms des pistes */
    static const char* const LaneNames[kLanes] = { "KICK", "SNARE", "HAT" };
    
    /** Index des drums */
    enum DrumIndex
    {
        Kick = 0,
        Snare = 1,
        Hat = 2
    };
    
    // === COULEURS ===
    
    namespace Colors
    {
        /** Couleur accent principale (orange vif) */
        inline const juce::Colour accent(0xffff6b35);
        
        /** Couleur Kick (rouge) */
        inline const juce::Colour kick(0xffff4444);
        
        /** Couleur Snare (vert) */
        inline const juce::Colour snare(0xff44ff44);
        
        /** Couleur Hat (bleu) */
        inline const juce::Colour hat(0xff4444ff);
        
        /** Dégradé de fond - haut */
        inline const juce::Colour backgroundTop(0xff1a1a1a);
        
        /** Dégradé de fond - bas */
        inline const juce::Colour backgroundBottom(0xff0f0f0f);
        
        /** Couleur des séparateurs */
        inline const juce::Colour separator(0xff2a2a2a);
        
        /** Couleur des marqueurs de mesure */
        inline const juce::Colour measureMarker(0xff666666);
        
        /** Couleur bouton inactif */
        inline const juce::Colour buttonInactive(0xff3a3a3a);
        
        /**
         * @brief Récupère la couleur d'une piste
         * @param laneIndex Index de la piste (0-2)
         * @return Couleur correspondante
         */
        inline juce::Colour getLaneColor(int laneIndex)
        {
            switch (laneIndex)
            {
                case Kick:  return kick;
                case Snare: return snare;
                case Hat:   return hat;
                default:    return accent;
            }
        }
    }
    
    // === LAYOUT / DIMENSIONS ===
    
    namespace Layout
    {
        /** Marges extérieures de la fenêtre */
        constexpr int windowMargin = 16;
        
        /** Hauteur de la barre de transport */
        constexpr int transportHeight = 75;
        
        /** Hauteur de la zone de contrôles drums */
        constexpr int drumControlHeight = 760;
        
        /** Hauteur des boutons de sélection de drum */
        constexpr int drumSelectorHeight = 45;
        
        /** Largeur des labels de lanes (grille) */
        constexpr int laneLabelWidth = 110;
        
        /** Largeur d'un bouton de sélection de drum */
        constexpr int drumButtonWidth = 120;
        
        /** Espacement entre les boutons de sélection */
        constexpr int drumButtonSpacing = 10;
        
        /** Hauteur d'un slider */
        constexpr int sliderHeight = 35;
        
        /** Espacement entre sliders */
        constexpr int sliderSpacing = 8;
        
        /** Marge intérieure des groupes */
        constexpr int groupMargin = 12;
        
        /** Marge verticale du titre des groupes */
        constexpr int groupTitleMargin = 26;
    }
    
    // === TAILLES DE FENÊTRE ===
    
    namespace Window
    {
        /** Largeur par défaut de la fenêtre */
        constexpr int defaultWidth = 1400;
        
        /** Hauteur par défaut de la fenêtre */
        constexpr int defaultHeight = 1150;
    }
    
    // === AUDIO / PARAMÈTRES ===
    
    namespace Audio
    {
        /** Taille de la queue de commandes UI -> Audio */
        constexpr int commandQueueSize = 1024;
        
        /** Fréquence de rafraîchissement de l'UI (Hz) */
        constexpr int uiRefreshRate = 30;
    }
}
