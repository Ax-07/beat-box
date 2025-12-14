//  Drumbox/core/include/drumbox_core/Engine.h

#pragma once
#include "drumbox_core/Types.h"
#include "drumbox_core/seq/Pattern.h"
#include "drumbox_core/seq/Transport.h"
#include "drumbox_core/drums/Kick.h"
#include "drumbox_core/drums/Snare.h"
#include "drumbox_core/drums/HiHat.h"
#include "drumbox_core/Params.h"

namespace drumbox_core {

class Engine {
public:
    Params& params() { return params_; }
    const Params& params() const { return params_; }

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setBpm(float bpm);
    void setPlaying(bool play);

    void setStep(int lane, int step, bool on, float velocity);

    // rendu interleaved: out[frame*ch + c]
    void process(float* outInterleaved, int numFrames, int numChannels);

    // lecture (pour UI plus tard)
    int getStepIndex() const { return transport_.stepIndex; }
    float getBpm() const { return transport_.bpm; }
    bool isPlaying() const { return transport_.playing; }

private:
    void triggerStep(int stepIndex);

    double sampleRate_ = 48000.0;
    int maxBlock_ = 0;

    Pattern pattern_{};
    Transport transport_{};
    
    Params params_{};

    Kick  kick_{};
    Snare snare_{};
    HiHat hat_{};
};

} // namespace drumbox_core
