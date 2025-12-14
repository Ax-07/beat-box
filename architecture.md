# Architecture Drum Box

```txt
DrumBox/
├─ CMakeLists.txt                # superbuild (tout)
├─ core/                         # PORTABLE (zéro JUCE)
│  ├─ include/drumbox_core/
│  │  ├─ Engine.h                # API stable: prepare/process/params/events
│  │  ├─ Types.h                 # types communs (Buffers, Events)
│  │  ├─ seq/                    # Pattern/Transport/Sequencer
│  │  │  ├─ Pattern.h
│  │  │  └─ Transport.h
│  │  ├─ drums/                  # Kick/Snare/Hat (synth)
│  │  │  ├─ Kick.h
│  │  │  ├─ Snare.h
│  │  │  └─ Hat.h
│  │  └─ dsp/                    # Envelope/Noise/Filters/Saturation
│  │     ├─ EnvelopeExp.h
│  │     ├─ OnePole.h
│  │     ├─ Saturation.h
│  │     └─ Noise.h
│  └─ src/
│     └─ Engine.cpp
├─ juce/                         # wrappers JUCE (VST3 + Standalone)
│  ├─ third_party/JUCE/          # submodule
│  ├─ plugin/                    # VST3/AU (plus tard)
│  │  ├─ CMakeLists.txt
│  │  └─ Source/
│  │     ├─ PluginProcessor.*    # appelle drumbox_core::Engine
│  │     └─ PluginEditor.*       # UI JUCE stable
│  └─ standalone/                # App standalone
│     ├─ CMakeLists.txt
│     └─ Source/
│        ├─ Main.cpp
│        └─ MainComponent.*      # UI JUCE + audio device
├─ hardware/                     # plus tard (pas Arduino Uno)
│  ├─ common/                    # wrappers I/O communs, mapping potards
│  └─ teensy4_or_daisy/          # projet spécifique cible
└─ tools/
   └─ runner_miniaudio/      ← TEST UNIQUEMENT
      ├─ include/
      │  └─ App.h
      ├─ src/
      │  ├─ App.cpp
      │  └─ main.cpp
      └─ third_party/miniaudio/
         └─ miniaudio.h

```

## Description

Le projet Drum Box est structuré en plusieurs modules pour séparer les responsabilités et faciliter la maintenance.