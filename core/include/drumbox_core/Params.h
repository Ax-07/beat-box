// Drumbox/core/include/drumbox_core/Params.h

#pragma once
#include <atomic>

namespace drumbox_core
{

    struct Params
    {
        // Global
        std::atomic<float> masterGain{0.6f};

        // Master (EQ + clipper)
        std::atomic<float> masterEqLowDb{0.0f};  // -24..24
        std::atomic<float> masterEqMidDb{0.0f};  // -24..24
        std::atomic<float> masterEqHighDb{0.0f}; // -24..24
        std::atomic<float> masterClipOn{1.0f};   // 0/1
        std::atomic<float> masterClipMode{0.0f}; // 0=soft, 1=hard

        // Kick
        std::atomic<float> kickDecay{0.9995f};
        std::atomic<float> kickPitchDecay{0.9930f};
        std::atomic<float> kickDriveDecay{0.9900f};
        std::atomic<float> kickAttackFreq{120.0f};
        std::atomic<float> kickBaseFreq{55.0f};
        std::atomic<float> kickDriveAmount{14.0f};
        std::atomic<float> kickClickGain{0.70f};
        std::atomic<float> kickPreHpHz{30.0f};
        std::atomic<float> kickPostGain{0.85f};

        // Post shaping (gabber/hardstyle)
        std::atomic<float> kickPostLpHz{8000.0f};
        std::atomic<float> kickPostHpHz{25.0f};
        // 0=tanh, 1=hard clip, 2=foldback
        std::atomic<float> kickClipMode{0.0f};

        // Kickbass extensions
        std::atomic<float> kickTailDecay{0.9992f};
        std::atomic<float> kickTailMix{0.45f};    // 0..1
        std::atomic<float> kickTailFreqMul{1.0f}; // 1..4
        std::atomic<float> kickSubMix{0.35f};     // 0..1 (sub propre en parallèle)
        std::atomic<float> kickSubLpHz{180.0f};
        std::atomic<float> kickFeedback{0.08f}; // 0..0.5 typique

        // Kick transient character
        std::atomic<float> kickTokAmount{0.20f}; // 0..1
        std::atomic<float> kickTokHpHz{180.0f};
        std::atomic<float> kickCrunchAmount{0.15f}; // 0..1

        // 2 dist chains + TOK/CRUNCH
        std::atomic<float> kickChain1Mix{0.70f};      // 0..1
        std::atomic<float> kickChain1DriveMul{1.00f}; // multiplicateur de drive
        std::atomic<float> kickChain1LpHz{9000.0f};
        std::atomic<float> kickChain1Asym{0.00f}; // -1..1
        // -1 = suit kickClipMode global, sinon 0=tanh, 1=hard, 2=fold
        std::atomic<float> kickChain1ClipMode{-1.0f};

        std::atomic<float> kickChain2Mix{0.30f}; // 0..1
        std::atomic<float> kickChain2DriveMul{1.60f};
        std::atomic<float> kickChain2LpHz{5200.0f};
        std::atomic<float> kickChain2Asym{0.20f};
        // -1 = suit kickClipMode global, sinon 0=tanh, 1=hard, 2=fold
        std::atomic<float> kickChain2ClipMode{-1.0f};

        // Kick layers (2 mini-synths) - coefficients DSP pour A/D
        // layerType: 0=sine, 1=triangle, 2=square, 3=noise
        std::atomic<float> kickLayer1Enabled{0.0f};
        std::atomic<float> kickLayer1Type{0.0f};
        std::atomic<float> kickLayer1FreqHz{110.0f};
        std::atomic<float> kickLayer1Phase01{0.0f};      // 0..1
        std::atomic<float> kickLayer1Drive{0.0f};        // 0..1 (drive interne)
        std::atomic<float> kickLayer1AttackCoeff{0.05f}; // 0..1 (0=instant)
        std::atomic<float> kickLayer1DecayCoeff{0.9992f};
        std::atomic<float> kickLayer1Vol{0.0f}; // gain lin

        std::atomic<float> kickLayer2Enabled{0.0f};
        std::atomic<float> kickLayer2Type{1.0f};
        std::atomic<float> kickLayer2FreqHz{220.0f};
        std::atomic<float> kickLayer2Phase01{0.0f};
        std::atomic<float> kickLayer2Drive{0.0f};
        std::atomic<float> kickLayer2AttackCoeff{0.05f};
        std::atomic<float> kickLayer2DecayCoeff{0.9992f};
        std::atomic<float> kickLayer2Vol{0.0f};

        // Kick LFO (modulation) - 0..1 etc.
        // shape: 0=sine, 1=triangle, 2=square
        // target: 0=pitch, 1=drive, 2=cutoff, 3=phase
        std::atomic<float> kickLfoAmount{0.0f}; // 0..1
        std::atomic<float> kickLfoRateHz{2.0f}; // Hz
        std::atomic<float> kickLfoShape{0.0f};  // 0..2
        std::atomic<float> kickLfoTarget{0.0f}; // 0..3
        std::atomic<float> kickLfoPulse{0.5f};  // 0..1 (square duty)

        // Kick Reverb (kick-tail)
        std::atomic<float> kickReverbAmount{0.0f}; // 0..1 (wet)
        std::atomic<float> kickReverbSize{0.35f};  // 0..1
        std::atomic<float> kickReverbTone{0.55f};  // 0..1 (bright)

        // Kick FX
        std::atomic<float> kickFxShiftHz{0.0f};    // -2000..2000
        std::atomic<float> kickFxStereo{0.0f};     // 0..1 (width)
        std::atomic<float> kickFxDiffusion{0.0f};  // 0..1 (all-pass feedback)
        std::atomic<float> kickFxCleanDirty{1.0f}; // 0..1 (0=clean, 1=dirty)
        std::atomic<float> kickFxTone{0.5f};       // 0..1 (0=dark, 1=bright)
        // FX envelope (transient emphasis on FX path)
        std::atomic<float> kickFxEnvAttackCoeff{0.05f}; // 0..1 (0=instant)
        std::atomic<float> kickFxEnvDecayCoeff{0.995f}; // 0..1 (close to 1 = long)
        std::atomic<float> kickFxEnvVol{0.0f};          // 0..1
        std::atomic<float> kickFxDisperse{0.0f};        // 0..1
        std::atomic<float> kickFxInflator{0.0f};        // 0..1
        std::atomic<float> kickFxInflatorMix{0.5f};     // 0..1
        std::atomic<float> kickFxOttAmount{0.0f};       // 0..1

        // Oversampling (qualité disto)
        std::atomic<float> kickOversample2x{0.0f}; // 0/1

        // Snare
        std::atomic<float> snareDecay{0.9975f};
        std::atomic<float> snareToneFreq{180.0f};
        std::atomic<float> snareNoiseMix{0.75f};

        // Hat
        std::atomic<float> hatDecay{0.96f};
        std::atomic<float> hatCutoff{7000.0f};
    };

} // namespace drumbox_core
