#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "drumbox_core/Engine.h"
#include <cstdio>
#include <vector>
#include <atomic>

static drumbox_core::Engine gEngine;
static std::atomic<bool> gReady{false};

static void data_callback(ma_device* device, void* output, const void*, ma_uint32 frameCount)
{
    float* out = reinterpret_cast<float*>(output);
    if (!gReady.load()) {
        for (ma_uint32 i = 0; i < frameCount * device->playback.channels; ++i) out[i] = 0.0f;
        return;
    }

    gEngine.process(out, (int)frameCount, (int)device->playback.channels);
}

int main()
{
    ma_device_config cfg = ma_device_config_init(ma_device_type_playback);
    cfg.playback.format   = ma_format_f32;
    cfg.playback.channels = 2;
    cfg.sampleRate        = 48000;
    cfg.dataCallback      = data_callback;

    ma_device device;
    if (ma_device_init(nullptr, &cfg, &device) != MA_SUCCESS) {
        std::printf("Failed to open audio device.\n");
        return 1;
    }

    gEngine.prepare((double)cfg.sampleRate, 1024);
    gEngine.setBpm(120.0f);
    gEngine.setPlaying(true);
    gReady.store(true);

    if (ma_device_start(&device) != MA_SUCCESS) {
        std::printf("Failed to start audio.\n");
        ma_device_uninit(&device);
        return 1;
    }

    std::printf("Runner started. Press Enter to quit...\n");
    (void)getchar();

    ma_device_uninit(&device);
    return 0;
}
