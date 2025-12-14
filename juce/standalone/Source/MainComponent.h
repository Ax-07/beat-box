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
    std::array<std::array<juce::ToggleButton, kSteps>, kLanes> grid;

    // Core
    drumbox_core::Engine engine;
    SpscQueue<1024> queue;

    std::vector<float> interleavedTmp;

    std::atomic<bool> playing{true};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
