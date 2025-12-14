// DrumBox/juce/standalone/Source/MainComponent.cpp

#include "MainComponent.h"
#include <vector>

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

        Command c;
        c.type = Command::SetPlaying;
        c.on = newPlay;
        queue.push(c);

        playButton.setButtonText(newPlay ? "Stop" : "Play");
    };

    bpmLabel.setText("BPM", juce::dontSendNotification);
    addAndMakeVisible(bpmLabel);

    bpmSlider.setRange(40.0, 240.0, 0.1);
    bpmSlider.setValue(120.0);
    bpmSlider.onValueChange = [this]
    {
        Command c;
        c.type = Command::SetBpm;
        c.f = (float)bpmSlider.getValue();
        queue.push(c);
    };
    addAndMakeVisible(bpmSlider);

    // Master
    masterLabel.setText("Master", juce::dontSendNotification);
    addAndMakeVisible(masterLabel);

    masterSlider.setRange(0.0, 1.0, 0.001);
    masterSlider.setValue(0.6);
    masterSlider.onValueChange = [this]
    {
        engine.params().masterGain.store((float)masterSlider.getValue(), std::memory_order_relaxed);
    };
    addAndMakeVisible(masterSlider);

    // Groups
    kickGroup.setText("Kick"); addAndMakeVisible(kickGroup);
    snareGroup.setText("Snare"); addAndMakeVisible(snareGroup);
    hatGroup.setText("Hat"); addAndMakeVisible(hatGroup);

    auto setupSlider = [this](juce::Slider& s, double min, double max, double init, auto onChange)
    {
        s.setRange(min, max, 0.0001);
        s.setValue(init);
        s.onValueChange = std::move(onChange);
        addAndMakeVisible(s);
    };

    setupSlider(kickDecay, 0.95, 0.99995, 0.9995, [this]{
        engine.params().kickDecay.store((float)kickDecay.getValue(), std::memory_order_relaxed);
    });
    setupSlider(kickAttack, 40.0, 300.0, 120.0, [this]{
        engine.params().kickAttackFreq.store((float)kickAttack.getValue(), std::memory_order_relaxed);
    });
    setupSlider(kickBase, 30.0, 120.0, 55.0, [this]{
        engine.params().kickBaseFreq.store((float)kickBase.getValue(), std::memory_order_relaxed);
    });

    setupSlider(snareDecay, 0.95, 0.99995, 0.9975, [this]{
        engine.params().snareDecay.store((float)snareDecay.getValue(), std::memory_order_relaxed);
    });
    setupSlider(snareTone, 60.0, 400.0, 180.0, [this]{
        engine.params().snareToneFreq.store((float)snareTone.getValue(), std::memory_order_relaxed);
    });
    setupSlider(snareNoiseMix, 0.0, 1.0, 0.75, [this]{
        engine.params().snareNoiseMix.store((float)snareNoiseMix.getValue(), std::memory_order_relaxed);
    });

    setupSlider(hatDecay, 0.80, 0.999, 0.96, [this]{
        engine.params().hatDecay.store((float)hatDecay.getValue(), std::memory_order_relaxed);
    });
    setupSlider(hatCutoff, 1000.0, 12000.0, 7000.0, [this]{
        engine.params().hatCutoff.store((float)hatCutoff.getValue(), std::memory_order_relaxed);
    });

    // Grid
    for (int lane = 0; lane < kLanes; ++lane)
    {
        for (int step = 0; step < kSteps; ++step)
        {
            auto& b = grid[lane][step];
            b.setButtonText("");
            addAndMakeVisible(b);

            b.onClick = [this, lane, step]
            {
                pushToggle(lane, step, grid[lane][step].getToggleState());
            };
        }
    }

    setSize(900, 520);
    startTimerHz(30);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    engine.prepare(sampleRate, samplesPerBlockExpected);
    engine.clearPattern();
    engine.setBpm((float)bpmSlider.getValue());
    engine.setPlaying(true);

    engine.params().masterGain.store((float)masterSlider.getValue(), std::memory_order_relaxed);

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
    Command c;
    c.type = Command::ToggleStep;
    c.lane = lane;
    c.step = step;
    c.on = on;
    queue.push(c);
}

void MainComponent::refreshGridFromPattern()
{
    // Pour l’instant on n’a pas d’accès public au pattern dans Engine.
    // Donc on ne resync pas depuis le core (on pourra l’ajouter plus tard).
    // Ici on laisse l’UI telle quelle.
}

void MainComponent::timerCallback()
{
    repaint();
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);

    g.setColour(juce::Colours::white);
    g.setFont(18.0f);
    g.drawText("DrumBox Standalone", 20, 10, 400, 30, juce::Justification::left);

    // Affiche le step courant (playhead) en surbrillance
    const int step = engine.getStepIndex();
    g.setFont(14.0f);
    g.drawText("Step: " + juce::String(step + 1), 20, 40, 200, 20, juce::Justification::left);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(16);

    auto top = area.removeFromTop(70);
    playButton.setBounds(top.removeFromLeft(120).reduced(0, 10));
    top.removeFromLeft(12);
    bpmLabel.setBounds(top.removeFromLeft(40).reduced(0, 10));
    bpmSlider.setBounds(top.removeFromLeft(260).reduced(0, 10));
    top.removeFromLeft(12);
    masterLabel.setBounds(top.removeFromLeft(60).reduced(0, 10));
    masterSlider.setBounds(top.removeFromLeft(220).reduced(0, 10));

    auto controls = area.removeFromRight(320);

    auto groupH = 140;
    kickGroup.setBounds(controls.removeFromTop(groupH).reduced(0, 6));
    snareGroup.setBounds(controls.removeFromTop(groupH).reduced(0, 6));
    hatGroup.setBounds(controls.removeFromTop(groupH).reduced(0, 6));

    // sliders in groups (simple layout)
    auto place3 = [](juce::Rectangle<int> r, juce::Slider& a, juce::Slider& b, juce::Slider& c)
    {
        auto row = r.reduced(12, 26);
        auto h = 28;
        a.setBounds(row.removeFromTop(h));
        row.removeFromTop(10);
        b.setBounds(row.removeFromTop(h));
        row.removeFromTop(10);
        c.setBounds(row.removeFromTop(h));
    };

    place3(kickGroup.getBounds(), kickDecay, kickAttack, kickBase);
    place3(snareGroup.getBounds(), snareDecay, snareTone, snareNoiseMix);

    // Hat a 2 sliders
    {
        auto r = hatGroup.getBounds().reduced(12, 26);
        auto h = 28;
        hatDecay.setBounds(r.removeFromTop(h));
        r.removeFromTop(10);
        hatCutoff.setBounds(r.removeFromTop(h));
    }

    // Grid
    auto gridArea = area.reduced(0, 10);
    const int rowH = 50;
    const int cellW = gridArea.getWidth() / kSteps;

    for (int lane = 0; lane < kLanes; ++lane)
    {
        auto row = gridArea.removeFromTop(rowH);
        auto label = row.removeFromLeft(70);
        // pas de label component, juste texte dans paint via overlay rapide
        // (tu peux ajouter des Label si tu veux)

        for (int step = 0; step < kSteps; ++step)
        {
            grid[lane][step].setBounds(row.removeFromLeft(cellW).reduced(6, 10));
        }

        gridArea.removeFromTop(6);
    }
}
