// Drumbox/core/src/Engine.cpp

#include "drumbox_core/Engine.h"

#include <algorithm>
#include <atomic>

namespace drumbox_core
{
    void Engine::clearPattern() { pattern_.clear(); }

    void Engine::prepare(double sampleRate, int maxBlockSize)
    {
        sampleRate_ = sampleRate;
        maxBlock_ = maxBlockSize;

        transport_.prepare(sampleRate_);

        kick_.prepare(sampleRate_);
        snare_.prepare(sampleRate_);
        hat_.prepare(sampleRate_);

        reverb_.prepare((float)sampleRate_);
        fx_.prepare((float)sampleRate_);
        master_.prepare((float)sampleRate_);

        clearPattern(); // UI part de zéro

    }

    void Engine::reset()
    {
        transport_.reset();
        reverb_.reset();
        fx_.reset();
        master_.reset();
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
        {
            kick_.trigger(k.vel);
            fx_.triggerEnv(k.vel);
        }
        if (s.on)
            snare_.trigger(s.vel);
        if (h.on)
            hat_.trigger(h.vel);
    }

    void Engine::process(float *out, int numFrames, int numChannels)
    {

        const float masterGain = params_.masterGain.load(std::memory_order_relaxed);

        // Master params
        const float eqLowDb  = params_.masterEqLowDb.load(std::memory_order_relaxed);
        const float eqMidDb  = params_.masterEqMidDb.load(std::memory_order_relaxed);
        const float eqHighDb = params_.masterEqHighDb.load(std::memory_order_relaxed);
        const bool clipOn    = params_.masterClipOn.load(std::memory_order_relaxed) > 0.5f;
        const int clipMode   = (int)params_.masterClipMode.load(std::memory_order_relaxed);
        master_.setEqDb(eqLowDb, eqMidDb, eqHighDb);
        master_.setClipper(clipOn, clipMode);

        // Kick
        kick_.ampEnv.setDecay(params_.kickDecay.load(std::memory_order_relaxed));
        kick_.pitchEnv.setDecay(params_.kickPitchDecay.load(std::memory_order_relaxed));
        kick_.driveEnv.setDecay(params_.kickDriveDecay.load(std::memory_order_relaxed));

        kick_.attackFreq  = params_.kickAttackFreq.load(std::memory_order_relaxed);
        kick_.baseFreq    = params_.kickBaseFreq.load(std::memory_order_relaxed);

        kick_.driveAmount = params_.kickDriveAmount.load(std::memory_order_relaxed);
        kick_.clickGain   = params_.kickClickGain.load(std::memory_order_relaxed);
        kick_.postGain    = params_.kickPostGain.load(std::memory_order_relaxed);

        kick_.preHpHz     = params_.kickPreHpHz.load(std::memory_order_relaxed);
        kick_.preHP.setCutoff(kick_.preHpHz, (float)sampleRate_);

        // Oversampling: si actif, la partie "disto/post" tourne en 2x (Kick::process)
        kick_.oversample2x = params_.kickOversample2x.load(std::memory_order_relaxed) > 0.5f;
        const float srDist = kick_.oversample2x ? (float)(2.0 * sampleRate_) : (float)sampleRate_;

        kick_.postLpHz    = params_.kickPostLpHz.load(std::memory_order_relaxed);
        kick_.postLP.setCutoff(kick_.postLpHz, srDist);

        kick_.postHpHz    = params_.kickPostHpHz.load(std::memory_order_relaxed);
        kick_.postHP.setCutoff(kick_.postHpHz, srDist);

        kick_.clipMode    = (int)params_.kickClipMode.load(std::memory_order_relaxed);

        {
            float c1 = params_.kickChain1ClipMode.load(std::memory_order_relaxed);
            float c2 = params_.kickChain2ClipMode.load(std::memory_order_relaxed);
            if (c1 < -0.5f) c1 = (float)kick_.clipMode;
            if (c2 < -0.5f) c2 = (float)kick_.clipMode;
            kick_.chain1ClipMode = (int)c1;
            kick_.chain2ClipMode = (int)c2;
        }

        kick_.tokAmount   = params_.kickTokAmount.load(std::memory_order_relaxed);
        kick_.tokHpHz     = params_.kickTokHpHz.load(std::memory_order_relaxed);
        kick_.tokHP.setCutoff(kick_.tokHpHz, srDist);
        kick_.crunchAmount = params_.kickCrunchAmount.load(std::memory_order_relaxed);

        kick_.tailEnv.setDecay(params_.kickTailDecay.load(std::memory_order_relaxed));
        kick_.tailMix      = params_.kickTailMix.load(std::memory_order_relaxed);
        kick_.tailFreqMul  = params_.kickTailFreqMul.load(std::memory_order_relaxed);

        kick_.subMix       = params_.kickSubMix.load(std::memory_order_relaxed);
        kick_.subLpHz      = params_.kickSubLpHz.load(std::memory_order_relaxed);
        kick_.subLP.setCutoff(kick_.subLpHz, (float)sampleRate_);

        kick_.feedback     = params_.kickFeedback.load(std::memory_order_relaxed);

        kick_.chain1Mix      = params_.kickChain1Mix.load(std::memory_order_relaxed);
        kick_.chain1DriveMul = params_.kickChain1DriveMul.load(std::memory_order_relaxed);
        kick_.chain1LpHz     = params_.kickChain1LpHz.load(std::memory_order_relaxed);
        kick_.chain1Asym     = params_.kickChain1Asym.load(std::memory_order_relaxed);
        kick_.chain1LP.setCutoff(kick_.chain1LpHz, srDist);

        kick_.chain2Mix      = params_.kickChain2Mix.load(std::memory_order_relaxed);
        kick_.chain2DriveMul = params_.kickChain2DriveMul.load(std::memory_order_relaxed);
        kick_.chain2LpHz     = params_.kickChain2LpHz.load(std::memory_order_relaxed);
        kick_.chain2Asym     = params_.kickChain2Asym.load(std::memory_order_relaxed);
        kick_.chain2LP.setCutoff(kick_.chain2LpHz, srDist);

        kick_.tokAmount      = params_.kickTokAmount.load(std::memory_order_relaxed);
        kick_.tokHpHz        = params_.kickTokHpHz.load(std::memory_order_relaxed);
        kick_.tokHP.setCutoff(kick_.tokHpHz, srDist);

        kick_.crunchAmount   = params_.kickCrunchAmount.load(std::memory_order_relaxed);

        // Kick layers (2 mini synths)
        kick_.layer1Enabled     = params_.kickLayer1Enabled.load(std::memory_order_relaxed);
        kick_.layer1Type        = params_.kickLayer1Type.load(std::memory_order_relaxed);
        kick_.layer1FreqHz      = params_.kickLayer1FreqHz.load(std::memory_order_relaxed);
        kick_.layer1Phase01     = params_.kickLayer1Phase01.load(std::memory_order_relaxed);
        kick_.layer1Drive       = params_.kickLayer1Drive.load(std::memory_order_relaxed);
        kick_.layer1AttackCoeff = params_.kickLayer1AttackCoeff.load(std::memory_order_relaxed);
        kick_.layer1DecayCoeff  = params_.kickLayer1DecayCoeff.load(std::memory_order_relaxed);
        kick_.layer1Vol         = params_.kickLayer1Vol.load(std::memory_order_relaxed);

        kick_.layer2Enabled     = params_.kickLayer2Enabled.load(std::memory_order_relaxed);
        kick_.layer2Type        = params_.kickLayer2Type.load(std::memory_order_relaxed);
        kick_.layer2FreqHz      = params_.kickLayer2FreqHz.load(std::memory_order_relaxed);
        kick_.layer2Phase01     = params_.kickLayer2Phase01.load(std::memory_order_relaxed);
        kick_.layer2Drive       = params_.kickLayer2Drive.load(std::memory_order_relaxed);
        kick_.layer2AttackCoeff = params_.kickLayer2AttackCoeff.load(std::memory_order_relaxed);
        kick_.layer2DecayCoeff  = params_.kickLayer2DecayCoeff.load(std::memory_order_relaxed);
        kick_.layer2Vol         = params_.kickLayer2Vol.load(std::memory_order_relaxed);

        // Kick LFO
        kick_.lfoAmount = params_.kickLfoAmount.load(std::memory_order_relaxed);
        kick_.lfoRateHz = params_.kickLfoRateHz.load(std::memory_order_relaxed);
        kick_.lfoShape  = params_.kickLfoShape.load(std::memory_order_relaxed);
        kick_.lfoTarget = params_.kickLfoTarget.load(std::memory_order_relaxed);
        kick_.lfoPulse  = params_.kickLfoPulse.load(std::memory_order_relaxed);

        // Kick Reverb
        const float revAmt  = params_.kickReverbAmount.load(std::memory_order_relaxed);
        const float revSize = params_.kickReverbSize.load(std::memory_order_relaxed);
        const float revTone = params_.kickReverbTone.load(std::memory_order_relaxed);
        reverb_.setParams(revAmt, revSize, revTone);

        // Kick FX
        const float fxShift = params_.kickFxShiftHz.load(std::memory_order_relaxed);
        const float fxStereo = params_.kickFxStereo.load(std::memory_order_relaxed);
        const float fxDiff   = params_.kickFxDiffusion.load(std::memory_order_relaxed);
        const float fxCD     = params_.kickFxCleanDirty.load(std::memory_order_relaxed);
        const float fxTone   = params_.kickFxTone.load(std::memory_order_relaxed);
        const float fxEnvA   = params_.kickFxEnvAttackCoeff.load(std::memory_order_relaxed);
        const float fxEnvD   = params_.kickFxEnvDecayCoeff.load(std::memory_order_relaxed);
        const float fxEnvV   = params_.kickFxEnvVol.load(std::memory_order_relaxed);
        const float fxDisp = params_.kickFxDisperse.load(std::memory_order_relaxed);
        const float fxInf  = params_.kickFxInflator.load(std::memory_order_relaxed);
        const float fxMix  = params_.kickFxInflatorMix.load(std::memory_order_relaxed);
        const float fxOtt  = params_.kickFxOttAmount.load(std::memory_order_relaxed);
        fx_.setShiftHz(fxShift);
        fx_.setStereo(fxStereo);
        fx_.setDiffusion(fxDiff);
        fx_.setCleanDirty(fxCD);
        fx_.setTone(fxTone);
        fx_.setEnv(fxEnvA, fxEnvD, fxEnvV);
        fx_.setDisperse(fxDisp);
        fx_.setInflator(fxInf, fxMix);
        fx_.setOtt(fxOtt);


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
        
        playheadStep_.store(transport_.stepIndex, std::memory_order_relaxed);

        const double fps = transport_.framesPerStep();

        for (int f = 0; f < numFrames; ++f)
        {
            // Step trigger timing
            if ((double)transport_.currentFrame >= transport_.nextStepFrame)
            {
                triggerStep(transport_.stepIndex);
                transport_.stepIndex = (transport_.stepIndex + 1) % kSteps;
                playheadStep_.store(transport_.stepIndex, std::memory_order_relaxed);
                transport_.nextStepFrame += fps;

            }
            transport_.currentFrame++;

            // synth mix (dry mono)
            const float k = kick_.process((float)sampleRate_);
            const float s = snare_.process((float)sampleRate_);
            const float h = hat_.process((float)sampleRate_);
            const float dry = k + s + h;

            // Reverb sur le kick (wet stéréo)
            float wetL = 0.0f;
            float wetR = 0.0f;
            reverb_.processMono(k, wetL, wetR);

            float yL = dry + wetL;
            float yR = dry + wetR;

            // FX (disperse/inflator)
            float fxL = 0.0f;
            float fxR = 0.0f;
            fx_.process(yL, yR, fxL, fxR);

            // Master (EQ + gain + clip)
            float outL = 0.0f;
            float outR = 0.0f;
            master_.process(fxL, fxR, masterGain, outL, outR);

            // write to output
            if (numChannels == 1)
            {
                out[f] = 0.5f * (outL + outR);
            }
            else
            {
                out[f * numChannels + 0] = outL;
                out[f * numChannels + 1] = outR;
                for (int c = 2; c < numChannels; ++c)
                    out[f * numChannels + c] = 0.5f * (outL + outR);
            }
        }
    }

} // namespace drumbox_core
