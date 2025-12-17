// Drumbox/core/include/drumbox_core/drums/Kick.h

#pragma once
#include "drumbox_core/Types.h"
#include "drumbox_core/dsp/EnvelopeExp.h"
#include "drumbox_core/dsp/EnvelopeADExp.h"
#include "drumbox_core/dsp/OnePole.h"
#include "drumbox_core/dsp/Lfo.h"
#include "drumbox_core/dsp/Noise.h"
#include "drumbox_core/dsp/Saturation.h"
#include "drumbox_core/dsp/Oversampling2x.h"
#include <cmath>

namespace drumbox_core {

struct Kick {
    bool active = false;
    float phase = 0.0f;
    float hitVel = 1.0f;

    EnvelopeExp ampEnv;
    EnvelopeExp pitchEnv;
    EnvelopeExp driveEnv;
    EnvelopeExp tailEnv;

    EnvelopeADExp layer1Env;
    EnvelopeADExp layer2Env;

    Noise noise;
    Noise layerNoise;
    OnePoleHP preHP;
    OnePoleLP postLP;
    OnePoleHP postHP;
    OnePoleLP subLP;

    // Two parallel distortion chains (character)
    OnePoleLP chain1LP;
    OnePoleLP chain2LP;
    OnePoleHP tokHP;

    // params
    float preHpHz     = 30.0f;
    float postLpHz    = 8000.0f;
    float postHpHz    = 25.0f;

    // 0=tanh, 1=hard clip, 2=foldback
    int clipMode      = 0;

    // -1 = suit clipMode global, sinon 0=tanh, 1=hard, 2=foldback
    int chain1ClipMode = -1;
    int chain2ClipMode = -1;

    float baseFreq    = 52.0f;   // fin
    float attackFreq  = 360.0f;  // début

    float driveAmount = 14.0f;
    float postGain    = 0.85f;

    float clickGain   = 0.7f;

    // kickbass extras
    float tailMix      = 0.45f;  // 0..1
    float tailFreqMul  = 1.0f;   // 1..4
    float subMix       = 0.35f;  // 0..1
    float subLpHz      = 180.0f;
    float feedback     = 0.08f;

    // distortion chains
    float chain1Mix      = 0.70f;
    float chain1DriveMul = 1.00f;
    float chain1LpHz     = 9000.0f;
    float chain1Asym     = 0.00f;

    float chain2Mix      = 0.30f;
    float chain2DriveMul = 1.60f;
    float chain2LpHz     = 5200.0f;
    float chain2Asym     = 0.20f;

    float tokAmount      = 0.20f;
    float tokHpHz        = 180.0f;
    float crunchAmount   = 0.15f;

    // Layers (2 mini synths)
    // layerType: 0=sine, 1=triangle, 2=square, 3=noise
    float layer1Enabled     = 0.0f;
    float layer1Type        = 0.0f;
    float layer1FreqHz      = 110.0f;
    float layer1Phase01     = 0.0f;
    float layer1Drive       = 0.0f;
    float layer1AttackCoeff = 0.05f;
    float layer1DecayCoeff  = 0.9992f;
    float layer1Vol         = 0.0f;

    float layer2Enabled     = 0.0f;
    float layer2Type        = 1.0f;
    float layer2FreqHz      = 220.0f;
    float layer2Phase01     = 0.0f;
    float layer2Drive       = 0.0f;
    float layer2AttackCoeff = 0.05f;
    float layer2DecayCoeff  = 0.9992f;
    float layer2Vol         = 0.0f;

    float phaseTail = 0.0f;
    float phaseLayer1 = 0.0f;
    float phaseLayer2 = 0.0f;
    float fbZ = 0.0f;

    // seed qui évolue (click moins “figé”)
    u32 seed = 0x12345678u;

    // LFO
    float lfoAmount = 0.0f; // 0..1
    float lfoRateHz = 2.0f;
    float lfoShape  = 0.0f; // 0=sine 1=tri 2=square
    float lfoTarget = 0.0f; // 0=pitch 1=drive 2=cutoff 3=phase
    float lfoPulse  = 0.5f; // square duty
    Lfo   lfo;

    bool oversample2x = false;
    Oversampling2x os2x;

    float sr_ = 48000.0f;

    void prepare(double sr) {
        sr_ = (float)sr;
        // AMP ~200-250ms
        ampEnv.setDecay(0.9994f);

        // Pitch ~30ms
        pitchEnv.setDecay(0.9930f);

        // Drive ~20ms
        driveEnv.setDecay(0.9900f);

        // Tail plus long (kickbass)
        tailEnv.setDecay(0.9992f);

        preHP.setCutoff(preHpHz, (float)sr);
        postLP.setCutoff(postLpHz, (float)sr);
        postHP.setCutoff(postHpHz, (float)sr);
        subLP.setCutoff(subLpHz, (float)sr);

        chain1LP.setCutoff(chain1LpHz, (float)sr);
        chain2LP.setCutoff(chain2LpHz, (float)sr);
        tokHP.setCutoff(tokHpHz, (float)sr);

        layer1Env.stage = EnvelopeADExp::Off;
        layer1Env.value = 0.0f;
        layer2Env.stage = EnvelopeADExp::Off;
        layer2Env.value = 0.0f;

        lfo.reset(0.0f);
        os2x.reset();
    }

    void trigger(float vel) {
        active = true;
        phase = 0.0f;
        hitVel = vel;

        ampEnv.trigger(vel);
        pitchEnv.trigger(1.0f);
        driveEnv.trigger(1.0f);
        tailEnv.trigger(vel);

        // varie légèrement la seed à chaque hit
        seed = seed * 1664525u + 1013904223u;
        noise.seed(seed);
        layerNoise.seed(seed ^ 0x9E3779B9u);

        preHP.reset();
        postLP.reset();
        postHP.reset();
        subLP.reset();

        chain1LP.reset();
        chain2LP.reset();
        tokHP.reset();

        phaseTail = 0.0f;
        phaseLayer1 = clampf(layer1Phase01, 0.0f, 1.0f) * (2.0f * kPi);
        phaseLayer2 = clampf(layer2Phase01, 0.0f, 1.0f) * (2.0f * kPi);
        fbZ = 0.0f;

        if (layer1Enabled > 0.5f && layer1Vol > 0.0f)
            layer1Env.trigger(0.0f);
        else
        {
            layer1Env.stage = EnvelopeADExp::Off;
            layer1Env.value = 0.0f;
        }

        if (layer2Enabled > 0.5f && layer2Vol > 0.0f)
            layer2Env.trigger(0.0f);
        else
        {
            layer2Env.stage = EnvelopeADExp::Off;
            layer2Env.value = 0.0f;
        }

        // LFO resync sur chaque hit (plus musical pour un kick)
        lfo.reset(0.0f);
        os2x.reset();
    }

    inline float processDirtyPath(float xDrive)
    {
        // 2 chaînes de disto en parallèle (caractère)
        float y1 = applyAsym(xDrive * clampf(chain1DriveMul, 0.25f, 4.0f), chain1Asym);
        float y2 = applyAsym(xDrive * clampf(chain2DriveMul, 0.25f, 4.0f), chain2Asym);

        const int m1 = (chain1ClipMode >= 0) ? chain1ClipMode : clipMode;
        const int m2 = (chain2ClipMode >= 0) ? chain2ClipMode : clipMode;
        y1 = applyClipper(m1, y1);
        y2 = applyClipper(m2, y2);

        y1 = chain1LP.process(y1);
        y2 = chain2LP.process(y2);

        const float mix1 = clampf(chain1Mix, 0.0f, 1.0f);
        const float mix2 = clampf(chain2Mix, 0.0f, 1.0f);
        float dirty = y1 * mix1 + y2 * mix2;

        // TOK (punch): ajoute un peu de HP (transient)
        const float tok = tokHP.process(dirty) * clampf(tokAmount, 0.0f, 1.0f);
        dirty += tok;

        // CRUNCH: foldback en crossfade
        const float cr = clampf(crunchAmount, 0.0f, 1.0f);
        if (cr > 0.0001f)
        {
            const float c = foldback(dirty * (1.0f + 2.0f * cr), 1.0f);
            dirty = dirty * (1.0f - cr) + c * cr;
        }

        fbZ = dirty;

        // post shaping global (adoucit / sculpte)
        dirty = postLP.process(dirty);
        dirty = postHP.process(dirty);

        return dirty;
    }

    static inline float hardClip(float x) {
        if (x > 1.0f) return 1.0f;
        if (x < -1.0f) return -1.0f;
        return x;
    }

    static inline float foldback(float x, float threshold = 1.0f) {
        if (threshold <= 0.0f) return 0.0f;
        const float t2 = threshold * 2.0f;
        const float t4 = threshold * 4.0f;

        if (x > threshold || x < -threshold) {
            float y = std::fmod(x - threshold, t4);
            if (y < 0.0f) y += t4;
            if (y > t2) y = t4 - y;
            y -= threshold;
            return y;
        }
        return x;
    }

    static inline float clampf(float v, float lo, float hi) {
        return (v < lo) ? lo : ((v > hi) ? hi : v);
    }

    static inline float maxf(float a, float b) { return (a > b) ? a : b; }

    static inline float absf(float x) { return x < 0.0f ? -x : x; }

    static inline float applyClipper(int mode, float x)
    {
        switch (mode)
        {
            case 1:  return hardClip(x);
            case 2:  return foldback(x, 1.0f);
            default: return std::tanh(x);
        }
    }

    // asym: -1..1, agit comme une légère asymétrie de gain pos/neg
    static inline float applyAsym(float x, float asym) {
        const float a = clampf(asym, -1.0f, 1.0f);
        const float gPos = 1.0f + 0.35f * a;
        const float gNeg = 1.0f - 0.35f * a;
        return (x >= 0.0f) ? (x * gPos) : (x * gNeg);
    }

    static inline float triangleFromPhase(float phase) {
        // phase en radians [0..2pi)
        const float invTwoPi = 1.0f / (2.0f * kPi);
        float t = phase * invTwoPi; // 0..1
        t -= std::floor(t);
        // 0..1 -> -1..1
        return 4.0f * std::abs(t - 0.5f) - 1.0f;
    }

    static inline float squareFromPhase(float phase) {
        // phase en radians [0..2pi)
        return (phase < kPi) ? 1.0f : -1.0f;
    }

    static inline float wrapPhase(float phase) {
        if (phase >= 2.0f * kPi)
            phase -= 2.0f * kPi;
        if (phase < 0.0f)
            phase += 2.0f * kPi;
        return phase;
    }

    static inline float semitoneRatio(float semis) {
        // ratio = 2^(semis/12)
        return std::pow(2.0f, semis / 12.0f);
    }

    float processLayer(EnvelopeADExp& env,
                       float& phaseRad,
                       float enabled,
                       float typeF,
                       float freqHz,
                       float volLin,
                       float drive01,
                       float attackCoeff,
                       float decayCoeff,
                       float sr,
                       float phaseOffsetRad)
    {
        if (enabled < 0.5f || volLin <= 0.0f)
            return 0.0f;

        env.setAttack(clampf(attackCoeff, 0.0f, 1.0f));
        env.setDecay(clampf(decayCoeff, 0.0f, 0.999999f));

        const float e = env.process();
        if (e <= 0.0f)
            return 0.0f;

        const int type = (int)clampf(typeF, 0.0f, 3.0f);
        const float hz = clampf(freqHz, 1.0f, 20000.0f);
        phaseRad += (2.0f * kPi) * hz / maxf(1.0f, sr);
        phaseRad = wrapPhase(phaseRad);

        float osc = 0.0f;
        const float phaseForOsc = wrapPhase(phaseRad + phaseOffsetRad);
        switch (type)
        {
            default:
            case 0: osc = std::sinf(phaseForOsc); break;
            case 1: osc = triangleFromPhase(phaseForOsc); break;
            case 2: osc = squareFromPhase(phaseForOsc); break;
            case 3: osc = layerNoise.white(); break;
        }

        const float d = clampf(drive01, 0.0f, 1.0f);
        float x = osc * e * volLin * hitVel;
        x = softClip(x * (1.0f + 16.0f * d));
        return x;
    }

    float process(float sr) {
        if (!active) return 0.0f;

        const float srDist = oversample2x ? (2.0f * sr) : sr;

        // LFO (par sample)
        const float amount = clampf(lfoAmount, 0.0f, 1.0f);
        const int shape = (int)clampf(lfoShape, 0.0f, 2.0f);
        const int target = (int)clampf(lfoTarget, 0.0f, 3.0f);
        const float lfoV = (amount > 0.0001f) ? lfo.process(lfoRateHz, sr, shape, lfoPulse) : 0.0f;

        const float amp   = ampEnv.process();
        const float pitch = pitchEnv.process();
        const float drive = driveEnv.process();
        const float tailA  = tailEnv.process();

        // Modulation Pitch: profondeur +/-12 demi-tons à amount=1
        float baseHz = baseFreq;
        float attackHz = attackFreq;
        if (target == 0 && amount > 0.0001f)
        {
            const float depthSemis = 12.0f * amount;
            const float ratio = semitoneRatio(lfoV * depthSemis);
            baseHz = clampf(baseHz * ratio, 1.0f, 20000.0f);
            attackHz = clampf(attackHz * ratio, 1.0f, 20000.0f);
        }

        const float freq = baseHz + (attackHz - baseHz) * pitch;

        phase += (2.0f * kPi) * freq / sr;
        if (phase >= 2.0f * kPi) phase -= 2.0f * kPi;

        const float body = std::sinf(phase);

        // tail: triangle (plus riche en harmoniques que sinus)
        const float tailFreq = maxf(1.0f, baseFreq * clampf(tailFreqMul, 1.0f, 4.0f));
        phaseTail += (2.0f * kPi) * tailFreq / sr;
        if (phaseTail >= 2.0f * kPi) phaseTail -= 2.0f * kPi;
        const float tail = triangleFromPhase(phaseTail);

        // click bruité (suit driveEnv pour taper au début)
        const float click = noise.white() * clickGain * drive;

        // Chemin sub propre (évite de faire clipper la sub)
        const float sub = subLP.process(body * amp);

        // Modulation Drive
        float driveAmt = driveAmount;
        if (target == 1 && amount > 0.0001f)
        {
            const float m = 1.0f + 0.75f * amount * lfoV;
            driveAmt = clampf(driveAmt * m, 0.0f, 40.0f);
        }

        // Modulation Cutoff (postLP)
        float postLp = postLpHz;
        if (target == 2 && amount > 0.0001f)
        {
            const float depthSemis = 24.0f * amount; // +/- 2 octaves à amount=1
            const float ratio = semitoneRatio(lfoV * depthSemis);
            postLp = clampf(postLp * ratio, 40.0f, 20000.0f);
            postLP.setCutoff(postLp, srDist);
        }

        // Chemin "dirty": body + tail + click
        float dirtyIn = (body * amp) + (tail * tailA * clampf(tailMix, 0.0f, 1.0f)) + click;

        // Layers ajoutés avant disto
        const float phaseMod = (target == 3) ? (lfoV * amount * kPi) : 0.0f;

        dirtyIn += processLayer(layer1Env, phaseLayer1,
                    layer1Enabled, layer1Type, layer1FreqHz,
                    layer1Vol, layer1Drive,
                    layer1AttackCoeff, layer1DecayCoeff,
                sr, phaseMod);
        dirtyIn += processLayer(layer2Env, phaseLayer2,
                    layer2Enabled, layer2Type, layer2FreqHz,
                    layer2Vol, layer2Drive,
                    layer2AttackCoeff, layer2DecayCoeff,
                sr, phaseMod);
        dirtyIn = preHP.process(dirtyIn);

        // drive commun (modulé par env) + feedback
        const float fb = clampf(feedback, 0.0f, 0.5f);
        const float driveK = 1.0f + drive * driveAmt;
        const float xDrive = (dirtyIn + fbZ * fb) * driveK;

        float dirty = 0.0f;
        if (oversample2x)
        {
            dirty = os2x.process(xDrive, [this](float xs) {
                return processDirtyPath(xs);
            });
        }
        else
        {
            dirty = processDirtyPath(xDrive);
        }

        // mix final
        const float sm = clampf(subMix, 0.0f, 1.0f);
        float x = sub * sm + dirty * (1.0f - sm);

        // sortie
        x = softClip(x) * postGain;

        if (!ampEnv.isActive() && !layer1Env.isActive() && !layer2Env.isActive())
            active = false;

        return x;
    }
};

} // namespace drumbox_core
