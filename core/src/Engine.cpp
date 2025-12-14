// Drumbox/core/src/Engine.cpp

#include "drumbox_core/Engine.h"
#include "drumbox_core/dsp/Saturation.h"

#include <algorithm>
#include <atomic>

namespace drumbox_core
{

    void Engine::prepare(double sampleRate, int maxBlockSize)
    {
        sampleRate_ = sampleRate;
        maxBlock_ = maxBlockSize;

        transport_.prepare(sampleRate_);

        kick_.prepare(sampleRate_);
        snare_.prepare(sampleRate_);
        hat_.prepare(sampleRate_);

        pattern_.clear();

        // Pattern demo par défaut (pour entendre tout de suite)
        // Kick: 1 et 9
        pattern_.setStep(0, 0, true, 1.0f);
        pattern_.setStep(0, 8, true, 0.95f);
        // Snare: 5 et 13
        pattern_.setStep(1, 4, true, 1.0f);
        pattern_.setStep(1, 12, true, 1.0f);
        // Hat: tous les 2 steps
        for (int i = 0; i < kSteps; i += 2)
            pattern_.setStep(2, i, true, 0.5f);
    }

    void Engine::reset()
    {
        transport_.reset();
    }

    void Engine::setBpm(float bpm)
    {
        if (bpm < 40.0f)
            bpm = 40.0f;
        if (bpm > 240.0f)
            bpm = 240.0f;
        transport_.bpm = bpm;
    }

    void Engine::setPlaying(bool play)
    {
        transport_.playing = play;
    }

    void Engine::setStep(int lane, int step, bool on, float velocity)
    {
        if (velocity < 0.0f)
            velocity = 0.0f;
        if (velocity > 1.0f)
            velocity = 1.0f;
        pattern_.setStep(lane, step, on, velocity);
    }

    void Engine::triggerStep(int stepIndex)
    {
        // lane 0 kick, 1 snare, 2 hat
        const Step k = pattern_.getStep(0, stepIndex);
        const Step s = pattern_.getStep(1, stepIndex);
        const Step h = pattern_.getStep(2, stepIndex);

        if (k.on)
            kick_.trigger(k.vel);
        if (s.on)
            snare_.trigger(s.vel);
        if (h.on)
            hat_.trigger(h.vel);
    }

    void Engine::process(float *out, int numFrames, int numChannels)
    {

        const float master = params_.masterGain.load(std::memory_order_relaxed);

        // Kick
        kick_.ampEnv.setDecay(params_.kickDecay.load(std::memory_order_relaxed));
        kick_.attackFreq = params_.kickAttackFreq.load(std::memory_order_relaxed);
        kick_.baseFreq = params_.kickBaseFreq.load(std::memory_order_relaxed);

        // Snare
        snare_.ampEnv.setDecay(params_.snareDecay.load(std::memory_order_relaxed));
        snare_.toneFreq = params_.snareToneFreq.load(std::memory_order_relaxed);
        snare_.noiseMix = params_.snareNoiseMix.load(std::memory_order_relaxed);

        // Hat
        hat_.ampEnv.setDecay(params_.hatDecay.load(std::memory_order_relaxed));
        hat_.cutoff = params_.hatCutoff.load(std::memory_order_relaxed);
        hat_.updateFilterIfNeeded((float)sampleRate_);



        // clear
        std::fill(out, out + (u64)numFrames * (u64)numChannels, 0.0f);

        if (!transport_.playing)
            return;

        const double fps = transport_.framesPerStep();

        for (int f = 0; f < numFrames; ++f)
        {
            // Step trigger timing
            if ((double)transport_.currentFrame >= transport_.nextStepFrame)
            {
                triggerStep(transport_.stepIndex);
                transport_.stepIndex = (transport_.stepIndex + 1) % kSteps;
                transport_.nextStepFrame += fps;
            }
            transport_.currentFrame++;

            // synth mix (mono)
            float x = 0.0f;
            x += kick_.process((float)sampleRate_);
            x += snare_.process((float)sampleRate_);
            x += hat_.process((float)sampleRate_);
            x *= master;

            x = softClip(x);

            // safety clamp (devrait rarement servir)
            if (x > 2.0f) x = 2.0f;
            if (x < -2.0f) x = -2.0f;


            // write to output
            if (numChannels == 1)
            {
                out[f] = x;
            }
            else
            {
                out[f * numChannels + 0] = x;
                out[f * numChannels + 1] = x;
                // si >2 canaux: duplique
                for (int c = 2; c < numChannels; ++c)
                    out[f * numChannels + c] = x;
            }
        }
    }

} // namespace drumbox_core
