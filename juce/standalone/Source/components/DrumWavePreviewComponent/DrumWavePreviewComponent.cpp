#include "DrumWavePreviewComponent.h"
#include <cmath>
#include <algorithm>

// ================= OneShotBufferSource =================

void OneShotBufferSource::setBuffer(const juce::AudioBuffer<float>& mono, double sr)
{
    buffer = mono; // copie (buffer petit: 400ms, OK)
    sampleRate = sr;
    pos = 0;
}

void OneShotBufferSource::prepareToPlay(int, double sr)
{
    sampleRate = sr;
    pos = 0;
}

void OneShotBufferSource::releaseResources() {}

void OneShotBufferSource::getNextAudioBlock(const juce::AudioSourceChannelInfo& info)
{
    info.clearActiveBufferRegion();

    if (buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0)
        return;

    const int outCh = info.buffer->getNumChannels();
    const int outN  = info.numSamples;

    const int total = buffer.getNumSamples();
    const int remaining = (int) std::max<juce::int64>(0, (juce::int64)total - pos);
    const int toCopy = std::min(outN, remaining);

    if (toCopy > 0)
    {
        const float* src = buffer.getReadPointer(0, (int)pos);
        for (int ch = 0; ch < outCh; ++ch)
            info.buffer->addFrom(ch, info.startSample, src, toCopy);

        pos += toCopy;
    }
}

void OneShotBufferSource::setNextReadPosition(juce::int64 newPosition)
{
    pos = juce::jlimit<juce::int64>(0, (juce::int64)buffer.getNumSamples(), newPosition);
}

juce::int64 OneShotBufferSource::getNextReadPosition() const { return pos; }
juce::int64 OneShotBufferSource::getTotalLength() const { return buffer.getNumSamples(); }

// ================= DrumWavePreviewComponent =================

DrumWavePreviewComponent::DrumWavePreviewComponent(juce::AudioDeviceManager& dm)
    : deviceManager(dm)
{
    // UI
    addAndMakeVisible(titleLabel);
    titleLabel.setText("Drum Preview", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setColour(juce::Label::textColourId, theme.text);
    titleLabel.setFont(juce::Font(12.0f, juce::Font::bold));

    addAndMakeVisible(playButton);
    playButton.onClick = [this] { onPlayClicked(); };
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a1a1a));
    playButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff60a5fa));
    playButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    playButton.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3a3a3a));

    // audio chain: OneShot -> Transport -> Device
    transport.setSource(&oneShot, 0, nullptr, (double)sampleRate);

    player.setSource(&transport);
    deviceManager.addAudioCallback(&player);

    // buffer par défaut
    mono.setSize(1, (int)std::round(sampleRate * (durationMs / 1000.0)));
    mono.clear();
    computeStats();
    
    // Timer pour mettre à jour la position de lecture
    startTimerHz(30);
}

DrumWavePreviewComponent::~DrumWavePreviewComponent()
{
    stopTimer();
    stopPlayback();
    deviceManager.removeAudioCallback(&player);
    player.setSource(nullptr);
    transport.setSource(nullptr);
}

void DrumWavePreviewComponent::stopPlayback()
{
    transport.stop();
    isPlayingBack = false;
}

void DrumWavePreviewComponent::setTitle(const juce::String& t)
{
    titleLabel.setText(t, juce::dontSendNotification);
}

void DrumWavePreviewComponent::setTheme(const WaveTheme& t)
{
    theme = t;
    titleLabel.setColour(juce::Label::textColourId, theme.text);
    repaint();
}

void DrumWavePreviewComponent::setMarkers(std::vector<WaveMarker> m)
{
    markers = std::move(m);
    repaint();
}

void DrumWavePreviewComponent::setEnvelope(EnvelopeFn fn, bool dashed)
{
    envelopeFn = std::move(fn);
    envelopeDashed = dashed;
    repaint();
}

void DrumWavePreviewComponent::setRenderFn(RenderFn fn)
{
    renderFn = std::move(fn);
}

void DrumWavePreviewComponent::setSampleRate(int sr)
{
    sampleRate = std::max(8000, sr);
    rerender();
}

void DrumWavePreviewComponent::setDurationMs(int ms)
{
    durationMs = std::max(10, ms);
    rerender();
}

void DrumWavePreviewComponent::rerender()
{
    const int N = (int)std::round(sampleRate * (durationMs / 1000.0));
    mono.setSize(1, std::max(1, N));
    mono.clear();

    if (renderFn)
        renderFn(mono, sampleRate, durationMs);

    computeStats();
    extractEnvelope();
    analyzeSignal();
    repaint();
}

void DrumWavePreviewComponent::computeStats()
{
    peak = 0.0f;
    double sumSq = 0.0;

    const int n = mono.getNumSamples();
    const float* x = mono.getReadPointer(0);

    for (int i = 0; i < n; ++i)
    {
        peak = std::max(peak, std::abs(x[i]));
        sumSq += (double)x[i] * (double)x[i];
    }

    rms = (n > 0) ? (float)std::sqrt(sumSq / (double)n) : 0.0f;
}

void DrumWavePreviewComponent::onPlayClicked()
{
    if (mono.getNumSamples() <= 0)
        return;

    stopPlayback();

    // Recharge le oneshot (et reset position)
    oneShot.setBuffer(mono, (double)sampleRate);

    transport.setPosition(0.0);
    transport.start();
    isPlayingBack = true;
    playbackPositionMs.store(0.0f, std::memory_order_relaxed);
}

void DrumWavePreviewComponent::updatePlaybackPosition()
{
    if (isPlayingBack && transport.isPlaying())
    {
        const double pos = transport.getCurrentPosition();
        playbackPositionMs.store((float)(pos * 1000.0), std::memory_order_relaxed);
        
        // Arrête si on a dépassé la durée
        if (pos * 1000.0 >= durationMs)
        {
            isPlayingBack = false;
            playbackPositionMs.store(0.0f, std::memory_order_relaxed);
        }
        
        repaint();
    }
}

void DrumWavePreviewComponent::extractEnvelope()
{
    const int n = mono.getNumSamples();
    if (n == 0) return;
    
    const float* x = mono.getReadPointer(0);
    
    // Extraction d'enveloppe par fenêtre glissante
    const int windowSize = std::max(1, (int)(sampleRate * 0.005)); // 5ms
    envelope.clear();
    envelope.reserve(n / windowSize + 1);
    
    for (int i = 0; i < n; i += windowSize)
    {
        float maxAbs = 0.0f;
        const int end = std::min(n, i + windowSize);
        for (int j = i; j < end; ++j)
            maxAbs = std::max(maxAbs, std::abs(x[j]));
        envelope.push_back(maxAbs);
    }
}

void DrumWavePreviewComponent::analyzeSignal()
{
    const int n = mono.getNumSamples();
    if (n == 0) return;
    
    const float* x = mono.getReadPointer(0);
    const float sampleDurationMs = 1000.0f / (float)sampleRate;
    
    // 1. Attack Time (temps pour atteindre le pic)
    attackTimeMs = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        if (std::abs(x[i]) >= peak * 0.9f)
        {
            attackTimeMs = (float)i * sampleDurationMs;
            break;
        }
    }
    
    // 2. Decay -6dB (peak/2 en linéaire)
    decay6dBMs = (float)durationMs;
    const float threshold6dB = peak * 0.5f;
    for (int i = 0; i < n; ++i)
    {
        if (std::abs(x[i]) >= peak * 0.9f)
        {
            // Cherche après le pic
            for (int j = i; j < n; ++j)
            {
                if (std::abs(x[j]) <= threshold6dB)
                {
                    decay6dBMs = (float)j * sampleDurationMs;
                    break;
                }
            }
            break;
        }
    }
    
    // 3. Decay -20dB (peak/10 en linéaire)
    decay20dBMs = (float)durationMs;
    const float threshold20dB = peak * 0.1f;
    for (int i = 0; i < n; ++i)
    {
        if (std::abs(x[i]) >= peak * 0.9f)
        {
            for (int j = i; j < n; ++j)
            {
                if (std::abs(x[j]) <= threshold20dB)
                {
                    decay20dBMs = (float)j * sampleDurationMs;
                    break;
                }
            }
            break;
        }
    }
    
    // 4. Durée effective (signal > -40dB)
    effectiveDurationMs = 0.0f;
    const float thresholdEffective = peak * 0.01f;
    for (int i = n - 1; i >= 0; --i)
    {
        if (std::abs(x[i]) > thresholdEffective)
        {
            effectiveDurationMs = (float)i * sampleDurationMs;
            break;
        }
    }
    
    // 5. Fréquence dominante (zero-crossing simpliste)
    int zeroCrossings = 0;
    for (int i = 1; i < std::min(n, (int)(sampleRate * 0.1)); ++i)
    {
        if ((x[i-1] >= 0.0f && x[i] < 0.0f) || (x[i-1] < 0.0f && x[i] >= 0.0f))
            zeroCrossings++;
    }
    dominantFreqHz = (float)zeroCrossings * 5.0f; // sur 100ms
}

void DrumWavePreviewComponent::timerCallback()
{
    updatePlaybackPosition();
}

void DrumWavePreviewComponent::resized()
{
    auto r = getLocalBounds().reduced(8);

    auto top = r.removeFromTop(22);
    titleLabel.setBounds(top.removeFromLeft(r.getWidth() - 60));
    playButton.setBounds(top.removeFromRight(54).reduced(2));
}

static inline float clamp01(float x) { return std::max(0.0f, std::min(1.0f, x)); }

void DrumWavePreviewComponent::paint(juce::Graphics& g)
{
    g.fillAll(theme.bg);

    auto r = getLocalBounds().reduced(8);
    r.removeFromTop(22 + 6);

    // waveform area
    const auto area = r;
    const int W = area.getWidth();
    const int H = area.getHeight();
    if (W <= 2 || H <= 2)
        return;

    const int midY = area.getY() + H / 2;
    const float ampPix = H * 0.45f;
    
    // Zoom vertical automatique (normalisation si signal faible)
    float displayScale = 1.0f;
    if (autoNormalize && peak > 0.0f && peak < 0.3f)
        displayScale = 0.3f / peak;

    // grille: midline
    g.setColour(theme.grid);
    g.drawLine((float)area.getX(), (float)midY, (float)area.getRight(), (float)midY, 1.0f);

    // verticales temps (chaque 50ms par défaut)
    const int stepMs = 50;
    for (int ms = stepMs; ms < durationMs; ms += stepMs)
    {
        const float x = (float)area.getX() + (float)ms / (float)durationMs * (float)W;
        g.drawLine(x, (float)area.getY(), x, (float)area.getBottom(), 1.0f);
    }

    // waveform: min/max par pixel pour meilleur rendu
    if (mono.getNumSamples() > 0)
    {
        const float* x = mono.getReadPointer(0);
        const int n = mono.getNumSamples();

        g.setColour(theme.wave);

        const double step = (double)n / (double)W;

        for (int px = 0; px < W; ++px)
        {
            const int iStart = (int)std::floor((double)px * step);
            const int iEnd = std::min(n - 1, (int)std::floor((double)(px + 1) * step));
            
            // Trouve min/max dans cette plage pour un meilleur rendu
            float minVal = x[iStart];
            float maxVal = x[iStart];
            for (int i = iStart; i <= iEnd; ++i)
            {
                minVal = std::min(minVal, x[i]);
                maxVal = std::max(maxVal, x[i]);
            }
            
            const float yMin = (float)midY - maxVal * ampPix * displayScale;
            const float yMax = (float)midY - minVal * ampPix * displayScale;
            const float X = (float)area.getX() + (float)px;

            // Dessine une ligne verticale min->max pour ce pixel
            g.drawLine(X, yMin, X, yMax, 1.5f);
        }
    }
    
    // Overlay: enveloppe d'amplitude extraite
    if (!envelope.empty())
    {
        g.setColour(theme.env.withAlpha(0.6f));
        juce::Path envPath;
        
        const int envSize = (int)envelope.size();
        for (int i = 0; i < envSize; ++i)
        {
            const float r = (float)i / (float)(envSize - 1);
            const float X = (float)area.getX() + r * (float)W;
            const float e = envelope[i] * displayScale;
            const float Y = (float)midY - e * ampPix;
            
            if (i == 0) envPath.startNewSubPath(X, Y);
            else envPath.lineTo(X, Y);
        }
        
        g.strokePath(envPath, juce::PathStrokeType(2.0f));
    }

    // envelope overlay (custom)
    if (envelopeFn)
    {
        g.setColour(theme.env);

        juce::Path env;
        const int N = std::max(32, W / 3);

        for (int k = 0; k <= N; ++k)
        {
            const float r01 = (float)k / (float)N;
            const double tSec = ((double)r01 * (double)durationMs) / 1000.0;
            const float e = clamp01(envelopeFn(tSec));

            const float X = (float)area.getX() + r01 * (float)W;
            const float Y = (float)midY - e * ampPix;

            if (k == 0) env.startNewSubPath(X, Y);
            else env.lineTo(X, Y);
        }

        if (envelopeDashed)
        {
            // Dessin manuel des pointillés
            const float dashLength = 4.0f;
            const float gapLength = 3.0f;
            const float totalDash = dashLength + gapLength;
            
            juce::PathFlatteningIterator it(env, juce::AffineTransform(), 1.0f);
            float distanceAlong = 0.0f;
            juce::Point<float> lastPt;
            bool first = true;
            
            while (it.next())
            {
                juce::Point<float> pt(it.x1, it.y1);
                
                if (!first)
                {
                    const float segLen = lastPt.getDistanceFrom(pt);
                    float startDist = distanceAlong;
                    float endDist = distanceAlong + segLen;
                    
                    // Dessine les segments visibles sur ce morceau de path
                    while (startDist < endDist)
                    {
                        const float dashStart = std::fmod(startDist, totalDash);
                        
                        if (dashStart < dashLength)
                        {
                            const float visibleEnd = std::min(startDist + (dashLength - dashStart), endDist);
                            const float t0 = (startDist - distanceAlong) / segLen;
                            const float t1 = (visibleEnd - distanceAlong) / segLen;
                            
                            const auto p0 = lastPt + (pt - lastPt) * t0;
                            const auto p1 = lastPt + (pt - lastPt) * t1;
                            
                            g.drawLine(p0.x, p0.y, p1.x, p1.y, 1.2f);
                            
                            startDist = visibleEnd;
                        }
                        else
                        {
                            startDist += totalDash - dashStart;
                        }
                    }
                    
                    distanceAlong = endDist;
                }
                
                lastPt = pt;
                first = false;
            }
        }
        else
        {
            g.strokePath(env, juce::PathStrokeType(1.2f));
        }
    }

    // markers
    if (!markers.empty())
    {
        g.setFont(juce::Font(10.0f, juce::Font::plain));
        for (const auto& m : markers)
        {
            const float X = (float)area.getX() + (m.atMs / (float)durationMs) * (float)W;
            g.setColour(m.colour.isTransparent() ? theme.markers : m.colour);
            g.drawLine(X, (float)area.getY(), X, (float)area.getBottom(), 1.0f);

            if (m.label.isNotEmpty())
            {
                g.setColour(theme.text);
                g.drawText(m.label, (int)X + 4, area.getY() + 2, 120, 12, juce::Justification::centredLeft);
            }
        }
    }
    
    // Marqueurs automatiques des points clés
    g.setFont(juce::Font(9.0f, juce::Font::plain));
    
    // Attack
    if (attackTimeMs > 0.0f && attackTimeMs < durationMs)
    {
        const float X = (float)area.getX() + (attackTimeMs / (float)durationMs) * (float)W;
        g.setColour(juce::Colour(0xff22c55e));
        g.drawLine(X, (float)area.getY(), X, (float)area.getBottom(), 1.0f);
        g.setColour(juce::Colour(0xff22c55e).brighter(0.3f));
        g.drawText("A", (int)X - 6, area.getY() + 2, 12, 12, juce::Justification::centred);
    }
    
    // Decay -6dB
    if (decay6dBMs > attackTimeMs && decay6dBMs < durationMs)
    {
        const float X = (float)area.getX() + (decay6dBMs / (float)durationMs) * (float)W;
        g.setColour(juce::Colour(0xfffbbf24).withAlpha(0.7f));
        g.drawLine(X, (float)area.getY(), X, (float)area.getBottom(), 1.0f);
        g.drawText("-6", (int)X - 8, area.getY() + 16, 16, 12, juce::Justification::centred);
    }
    
    // Decay -20dB
    if (decay20dBMs > decay6dBMs && decay20dBMs < durationMs)
    {
        const float X = (float)area.getX() + (decay20dBMs / (float)durationMs) * (float)W;
        g.setColour(juce::Colour(0xfff97316).withAlpha(0.7f));
        g.drawLine(X, (float)area.getY(), X, (float)area.getBottom(), 1.0f);
        g.drawText("-20", (int)X - 10, area.getY() + 30, 20, 12, juce::Justification::centred);
    }
    
    // Curseur de lecture animé
    if (isPlayingBack)
    {
        const float playMs = playbackPositionMs.load(std::memory_order_relaxed);
        if (playMs >= 0.0f && playMs <= durationMs)
        {
            const float X = (float)area.getX() + (playMs / (float)durationMs) * (float)W;
            g.setColour(juce::Colour(0xffef4444));
            g.drawLine(X, (float)area.getY(), X, (float)area.getBottom(), 2.0f);
        }
    }

    // stats (bas)
    g.setColour(theme.text);
    g.setFont(juce::Font(11.0f));

    auto db = [](float v) {
        const float x = std::max(1.0e-9f, v);
        return 20.0f * std::log10(x);
    };

    juce::String s;
    s << "SR: " << sampleRate << " Hz   "
      << "Durée: " << durationMs << " ms   "
      << "Samples: " << mono.getNumSamples() << "   "
      << "Peak: " << juce::String(peak, 3) << " (" << juce::String(db(peak), 1) << " dBFS)   "
      << "RMS: "  << juce::String(rms,  3) << " (" << juce::String(db(rms),  1) << " dBFS)   "
      << "Freq: " << juce::String(dominantFreqHz, 0) << " Hz   "
      << "Attack: " << juce::String(attackTimeMs, 1) << " ms";

    g.drawText(s, area.getX(), area.getBottom() + 6, area.getWidth(), 14, juce::Justification::centredLeft);
}
