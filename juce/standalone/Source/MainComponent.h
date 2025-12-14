// DrumBox/juce/standalone/Source/MainComponent.h

#pragma once
#include <JuceHeader.h>
#include "drumbox_core/Engine.h"
#include <array>
#include <atomic>
#include <vector>

// Petite queue SPSC (UI thread -> audio thread)
struct Command
{
    enum Type : uint8_t { ToggleStep, SetBpm, SetPlaying } type{};
    int lane = 0;
    int step = 0;
    bool on = false;
    float f = 0.0f;
};

template <size_t N>
class SpscQueue
{
public:
    bool push(const Command& c)
    {
        auto h = head.load(std::memory_order_relaxed);
        auto n = (h + 1) % N;
        if (n == tail.load(std::memory_order_acquire)) return false;
        buf[h] = c;
        head.store(n, std::memory_order_release);
        return true;
    }

    bool pop(Command& out)
    {
        auto t = tail.load(std::memory_order_relaxed);
        if (t == head.load(std::memory_order_acquire)) return false;
        out = buf[t];
        tail.store((t + 1) % N, std::memory_order_release);
        return true;
    }

private:
    std::array<Command, N> buf{};
    std::atomic<size_t> head{0}, tail{0};
};

struct StepButton : public juce::ToggleButton
{
    void setPlayhead(bool isPlayhead)
    {
        if (playhead != isPlayhead)
        {
            playhead = isPlayhead;
            repaint();
        }
    }

    void mouseEnter(const juce::MouseEvent&) override
    {
        hover = true;
        repaint();
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        hover = false;
        repaint();
    }

    void paintButton(juce::Graphics& g, bool, bool) override
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

private:
    bool playhead = false;
    bool hover = false;
};

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

    juce::GroupComponent kickGroup, snareGroup, hatGroup;
    juce::Slider kickDecay, kickAttack, kickBase;
    juce::Slider snareDecay, snareTone, snareNoiseMix;
    juce::Slider hatDecay, hatCutoff;

    // Grille 3x16
    static constexpr int kLanes = 3;
    static constexpr int kSteps = 16;
    std::array<std::array<StepButton, kSteps>, kLanes> grid;


    // Core
    drumbox_core::Engine engine;
    SpscQueue<1024> queue;

    std::vector<float> interleavedTmp;

    std::atomic<bool> playing{true};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
