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

    setSize(1100, 600);
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
    engine.setPlaying(false);
    playing.store(false);
    playButton.setButtonText("Play");

    lastPlayheadStep = -1;
    updatePlayheadOutline();

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

void MainComponent::updatePlayheadOutline()
{
    const int step = engine.getStepIndex();
    if (step == lastPlayheadStep) return;
    lastPlayheadStep = step;

    for (int lane = 0; lane < kLanes; ++lane)
    {
        for (int s = 0; s < kSteps; ++s)
            grid[lane][s].setPlayhead(s == step);
    }
}

void MainComponent::timerCallback()
{
    updatePlayheadOutline();
    repaint();
}

void MainComponent::paint (juce::Graphics& g)
{
    // Background dégradé
    auto bounds = getLocalBounds();
    juce::ColourGradient gradient(juce::Colour(0xff1a1a1a), 0, 0,
                                  juce::Colour(0xff0f0f0f), 0, (float)bounds.getHeight(), false);
    g.setGradientFill(gradient);
    g.fillAll();

    // Titre principal
    g.setColour(juce::Colour(0xffff6b35));
    g.setFont(juce::Font(24.0f, juce::Font::bold));
    g.drawText("DrumBox", 20, 12, 200, 40, juce::Justification::left);

    // Step indicator
    const int step = engine.getStepIndex();
    g.setColour(juce::Colours::lightgrey);
    g.setFont(14.0f);
    g.drawText("Step: " + juce::String(step + 1) + "/16", 20, 50, 150, 20, juce::Justification::left);

    // Labels des lanes (à gauche de la grille)
    auto area = getLocalBounds().reduced(16);
    area.removeFromTop(85);
    area.removeFromBottom(150);
    
    const int rowH = (area.getHeight() - 20) / kLanes;
    auto gridY = area.getY() + 10;
    
    g.setFont(juce::Font(16.0f, juce::Font::bold));
    const char* laneNames[] = {"KICK", "SNARE", "HAT"};
    const juce::Colour laneColors[] = {
        juce::Colour(0xffff4444),  // Rouge pour Kick
        juce::Colour(0xff44ff44),  // Vert pour Snare
        juce::Colour(0xff4444ff)   // Bleu pour Hat
    };
    
    for (int lane = 0; lane < kLanes; ++lane)
    {
        g.setColour(laneColors[lane]);
        int y = gridY + lane * rowH;
        g.drawText(laneNames[lane], 24, y, 80, rowH - 10, juce::Justification::centredLeft);
        
        // Ligne de séparation subtile
        if (lane > 0)
        {
            g.setColour(juce::Colour(0xff2a2a2a));
            g.drawLine(110.0f, (float)y, (float)(getWidth() - 20), (float)y, 1.0f);
        }
    }
    
    // Marqueurs de mesure (1, 5, 9, 13)
    g.setColour(juce::Colour(0xff666666));
    g.setFont(11.0f);
    auto gridArea = getLocalBounds().reduced(16);
    gridArea.removeFromTop(85);
    gridArea.removeFromBottom(150);
    gridArea.removeFromLeft(110);
    const int cellW = gridArea.getWidth() / kSteps;
    
    for (int i = 0; i < kSteps; i += 4)
    {
        int x = gridArea.getX() + i * cellW + cellW / 2;
        g.drawText(juce::String(i + 1), x - 15, gridArea.getY() - 20, 30, 15, juce::Justification::centred);
    }
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(16);

    // === TOP BAR : Transport + Master ===
    auto topBar = area.removeFromTop(75);
    topBar.removeFromLeft(200); // Espace pour le titre
    
    // Play button
    playButton.setBounds(topBar.removeFromLeft(100).reduced(4, 12));
    topBar.removeFromLeft(8);
    
    // BPM
    bpmLabel.setBounds(topBar.removeFromLeft(50).reduced(4, 12));
    bpmSlider.setBounds(topBar.removeFromLeft(220).reduced(4, 12));
    topBar.removeFromLeft(20);
    
    // Master (à droite)
    auto masterArea = topBar.removeFromRight(260);
    masterLabel.setBounds(masterArea.removeFromLeft(60).reduced(4, 12));
    masterSlider.setBounds(masterArea.reduced(4, 12));

    // === BOTTOM : Contrôles des instruments (horizontal) ===
    auto bottomControls = area.removeFromBottom(140);
    
    // Largeur égale pour chaque instrument
    const int instrumentWidth = bottomControls.getWidth() / 3;
    
    // Kick (gauche)
    auto kickArea = bottomControls.removeFromLeft(instrumentWidth).reduced(8, 4);
    kickGroup.setBounds(kickArea);
    auto kickInner = kickArea.reduced(12, 26);
    kickDecay.setBounds(kickInner.removeFromTop(30));
    kickInner.removeFromTop(6);
    kickAttack.setBounds(kickInner.removeFromTop(30));
    kickInner.removeFromTop(6);
    kickBase.setBounds(kickInner.removeFromTop(30));
    
    // Snare (centre)
    auto snareArea = bottomControls.removeFromLeft(instrumentWidth).reduced(8, 4);
    snareGroup.setBounds(snareArea);
    auto snareInner = snareArea.reduced(12, 26);
    snareDecay.setBounds(snareInner.removeFromTop(30));
    snareInner.removeFromTop(6);
    snareTone.setBounds(snareInner.removeFromTop(30));
    snareInner.removeFromTop(6);
    snareNoiseMix.setBounds(snareInner.removeFromTop(30));
    
    // Hat (droite)
    auto hatArea = bottomControls.reduced(8, 4);
    hatGroup.setBounds(hatArea);
    auto hatInner = hatArea.reduced(12, 26);
    hatDecay.setBounds(hatInner.removeFromTop(30));
    hatInner.removeFromTop(6);
    hatCutoff.setBounds(hatInner.removeFromTop(30));

    // === CENTRE : GRILLE SÉQUENCEUR (plus grande) ===
    auto gridArea = area.reduced(0, 10);
    gridArea.removeFromLeft(110); // Espace pour les labels de lanes
    
    const int rowH = (gridArea.getHeight() - 20) / kLanes;
    const int cellW = gridArea.getWidth() / kSteps;

    for (int lane = 0; lane < kLanes; ++lane)
    {
        auto row = gridArea.removeFromTop(rowH);
        
        for (int step = 0; step < kSteps; ++step)
        {
            // Plus d'espace entre les cellules
            auto cellArea = row.removeFromLeft(cellW);
            
            // Espacement supplémentaire tous les 4 steps (visuellement)
            int marginLeft = (step % 4 == 0) ? 8 : 4;
            int marginRight = (step % 4 == 3) ? 8 : 4;
            
            grid[lane][step].setBounds(cellArea.reduced(marginLeft, 8)
                                              .withTrimmedRight(marginRight - marginLeft));
        }
        
        gridArea.removeFromTop(10);
    }
}
