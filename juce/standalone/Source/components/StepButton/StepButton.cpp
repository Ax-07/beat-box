// DrumBox/juce/standalone/Source/components/stepButton/StepButton.cpp

#include "StepButton.h"

void StepButton::setPlayhead(bool isPlayhead)
{
    if (playhead != isPlayhead)
    {
        playhead = isPlayhead;
        repaint();
    }
}

void StepButton::mouseEnter(const juce::MouseEvent&)
{
    hover = true;
    repaint();
}

void StepButton::mouseExit(const juce::MouseEvent&)
{
    hover = false;
    repaint();
}

void StepButton::paintButton(juce::Graphics& g, bool, bool)
{
    auto bounds = getLocalBounds().toFloat();
    const float cornerSize = 6.0f;
    
    // États de couleur
    const bool isOn = getToggleState();
    
    // Couleurs de base
    juce::Colour bgColor, borderColor;
    
    if (isOn)
    {
        // Actif : orange/rouge vif avec dégradé
        bgColor = playhead 
            ? juce::Colour(0xffff8844)  // Orange clair si playhead
            : juce::Colour(0xffff6b35); // Orange standard
        borderColor = juce::Colour(0xffffaa77);
    }
    else
    {
        // Inactif : gris sombre
        bgColor = hover 
            ? juce::Colour(0xff3a3a3a)  // Gris moyen au survol
            : juce::Colour(0xff2a2a2a); // Gris foncé standard
        borderColor = juce::Colour(0xff404040);
    }
    
    // Dégradé subtil
    auto innerBounds = bounds.reduced(1.0f);
    juce::ColourGradient gradient(
        bgColor.brighter(0.15f), innerBounds.getCentreX(), innerBounds.getY(),
        bgColor.darker(0.1f), innerBounds.getCentreX(), innerBounds.getBottom(),
        false
    );
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(innerBounds, cornerSize);
    
    // Bordure
    if (isOn)
    {
        g.setColour(borderColor);
        g.drawRoundedRectangle(innerBounds, cornerSize, 1.5f);
    }
    else
    {
        g.setColour(borderColor);
        g.drawRoundedRectangle(innerBounds, cornerSize, 1.0f);
    }
    
    // Indicateur playhead : anneau lumineux épais
    if (playhead)
    {
        auto playheadRing = bounds.reduced(0.5f);
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.drawRoundedRectangle(playheadRing, cornerSize + 1.0f, 2.5f);
        
        // Effet de glow
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.drawRoundedRectangle(playheadRing.reduced(-1.0f), cornerSize + 2.0f, 1.0f);
    }
    
    // LED indicator au centre si actif
    if (isOn)
    {
        auto ledSize = 4.0f;
        auto ledBounds = juce::Rectangle<float>(ledSize, ledSize)
            .withCentre(bounds.getCentre());
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.fillEllipse(ledBounds);
    }
}
