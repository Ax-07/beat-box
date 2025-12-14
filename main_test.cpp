#include "drumbox_core/Engine.h"
#include <vector>

int main() {
    drumbox_core::Engine engine;
    engine.prepare(48000.0, 512);
    engine.setBpm(120.0f);
    engine.setPlaying(true);

    std::vector<float> buffer(512 * 2);
    engine.process(buffer.data(), 512, 2);

    return 0;
}
