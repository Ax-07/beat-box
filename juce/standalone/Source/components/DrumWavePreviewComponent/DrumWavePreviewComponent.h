#pragma once
#include <JuceHeader.h>
#include <functional>

// Marker vertical (en ms)
struct WaveMarker
{
    float atMs = 0.0f;
    juce::String label;
    juce::Colour colour { juce::Colour(0xfff59e0b) };
};

struct WaveTheme
{
    juce::Colour bg      { juce::Colour(0xff0a0a0a) };
    juce::Colour wave    { juce::Colour(0xff60a5fa) };
    juce::Colour env     { juce::Colour(0xff22c55e) };
    juce::Colour grid    { juce::Colour(0xff2a2a2a) };
    juce::Colour text    { juce::Colour(0xffa3a3a3) };
    juce::Colour markers { juce::Colour(0xfff59e0b) };
};

// Un petit AudioSource qui lit un buffer mono en one-shot
class OneShotBufferSource : public juce::PositionableAudioSource
{
public:
    void setBuffer(const juce::AudioBuffer<float>& mono, double sr);

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override;

    void setNextReadPosition(juce::int64 newPosition) override;
    juce::int64 getNextReadPosition() const override;
    juce::int64 getTotalLength() const override;
    bool isLooping() const override { return false; }

private:
    juce::AudioBuffer<float> buffer;
    juce::int64 pos = 0;
    double sampleRate = 48000.0;
};

// ------------------------------------------------------------------

class DrumWavePreviewComponent : public juce::Component,
                                  private juce::Timer
{
public:
    // renderFn doit écrire un buffer mono (1 canal).
    // Tu peux capturer tes params dedans.
    using RenderFn = std::function<void(juce::AudioBuffer<float>& outMono,
                                        int sampleRate,
                                        int durationMs)>;

    // envelope overlay: ampAtTime(tSec) => 0..1
    using EnvelopeFn = std::function<float(double tSec)>;

    DrumWavePreviewComponent(juce::AudioDeviceManager& dm);
    ~DrumWavePreviewComponent() override;

    void setTitle(const juce::String& t);
    void setTheme(const WaveTheme& t);
    void setMarkers(std::vector<WaveMarker> m);
    void setEnvelope(EnvelopeFn fn, bool dashed = false);

    void setRenderFn(RenderFn fn);

    void setSampleRate(int sr);
    void setDurationMs(int ms);

    // Force un rerender offline (ex: quand params changent)
    void rerender();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void onPlayClicked();
    void computeStats();
    void stopPlayback();
    void extractEnvelope();
    void analyzeSignal();
    void updatePlaybackPosition();
    void timerCallback() override;

    juce::AudioDeviceManager& deviceManager;

    // UI
    juce::TextButton playButton { "Play" };
    juce::Label titleLabel;

    // preview state
    RenderFn renderFn;
    EnvelopeFn envelopeFn;
    bool envelopeDashed = false;
    std::vector<WaveMarker> markers;

    WaveTheme theme;

    int sampleRate = 48000;
    int durationMs = 400;

    juce::AudioBuffer<float> mono; // 1ch buffer offline
    float peak = 0.0f;
    float rms  = 0.0f;
    
    // Analyse automatique
    std::vector<float> envelope; // Enveloppe d'amplitude extraite
    float attackTimeMs = 0.0f;
    float decay6dBMs = 0.0f;
    float decay20dBMs = 0.0f;
    float effectiveDurationMs = 0.0f;
    float dominantFreqHz = 0.0f;
    bool autoNormalize = true;
    
    // Curseur de lecture
    std::atomic<float> playbackPositionMs{0.0f};
    bool isPlayingBack = false;

    // audio playback chain
    OneShotBufferSource oneShot;
    juce::AudioTransportSource transport;
    juce::AudioSourcePlayer player;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumWavePreviewComponent)
};
