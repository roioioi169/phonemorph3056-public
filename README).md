# PHONEMORPH 3056 (Master Edition)

A lightweight master-bus VST3 effect that combines transient-preserving spectral-style morphing, a headphone "vacuum" stereo collapse, a formant-driven oral resonator, a tiny-speaker psychoacoustic translator, and a breath texture layer.

> **Status:** This is a condensed build. It compiles against JUCE 8 in principle but has not been built or tested yet. See [Known limitations](#known-limitations).

## Signal chain

1. **Lookahead transient splitter**: 12 ms lookahead, reported as plugin latency. Fast/slow envelope followers detect hits before they reach the audio path.
2. **Oral resonator**: Two TPT state-variable bandpass filters (300–900 Hz and 900–2600 Hz) centred on a zero-crossing frequency estimate. Transients bypass the resonator through the Transient Guard.
3. **Breath**: xorshift32 noise through a formant-tracking bandpass, gated by RMS dips between hits.
4. **Phantom low**: Sub-bass (below 110 Hz) is rectified and band-passed to 140–650 Hz, so small speakers suggest bass they can't reproduce.
5. **Tiny speaker knock**: A 1.2 kHz midrange bandpass adds punch, boosted on transients.
6. **Vacuum**: Mid/side processing. The side channel ducks on transients, with an inverse-phase 0.4 ms micro-delay on the snap.
7. **Output**: Soft clip with a -0.3 dBFS ceiling, plus output gain.

Filter coefficients update every 32 samples. The audio path itself runs sample by sample with no FFT, no oversampling, and no heap allocation in `processBlock`.

## Parameters

| ID | Name | Range | Default | Description |
|---|---|---|---|---|
| `morph` | Morph | 0–100 % | 45 % | Oral resonator blend on the sustain layer |
| `transientGuard` | Transient Guard | 0–100 % | 75 % | How much transients are protected from resonator smearing |
| `vacuum` | Vacuum | 0–100 % | 50 % | Side ducking and inward pull on transients |
| `throat` | Throat | 0–100 % | 55 % | Formant filter depth |
| `breath` | Breath | 0–100 % | 25 % | Air texture in micro-gaps |
| `phantomLow` | Phantom Low | 0–100 % | 60 % | Missing-fundamental bass for small speakers |
| `tinySpeaker` | Tiny Speaker | 0–100 % | 50 % | 1.2 kHz knock on transients |
| `outputGain` | Output Gain | −18 to +12 dB | 0 dB | Output trim |

Double-click any knob to reset it to its default.

## Building

### Requirements

- CMake 3.22+
- A C++17 compiler
- Git (CMake fetches JUCE 8.0.4 automatically)
- Windows: Visual Studio 2022 with the C++ desktop workload

### Windows (Visual Studio)

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

The VST3 bundle is written to:

```
build\Phonemorph3056_artifacts\Release\VST3\PHONEMORPH 3056.vst3
```

Copy it to `C:\Program Files\Common Files\VST3\` to load it in your DAW.

### macOS / Linux

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

The build should work, but the GitHub workflow only targets Windows.

### Continuous integration

`.github/workflows/build.yml` builds the Release VST3 on `windows-latest` and uploads it as the `phonemorph-3056-vst3` artifact. It runs on every push, on pull requests, and on manual dispatch.

## Project layout

```
phonemorph3056/
├── CMakeLists.txt
├── README.md
├── .github/workflows/build.yml
└── Source/
    ├── PluginProcessor.h / .cpp   APVTS parameters, processBlock, state
    ├── PluginEditor.h / .cpp      Editor layout, knobs, visualizer
    └── DSP/MorphEngines.h         SVF, envelopes, MorphChain DSP
```

## Known limitations

- **Formant tracking** is a zero-crossing heuristic, not a dual-band tracker.
- **Comb filter network** and pitch-tracked ring-modulation morphing are not implemented.
- **High-shelf air-pressure dip** on the vacuum stage is not implemented.
- **GUI caching**: the glass and bezel layers are redrawn every frame instead of being cached as offscreen images.
- **Fine drag** (Shift-drag) uses JUCE's default slider behaviour without custom tuning.
- **Untested**: the project has not yet been compiled or listened to. Expect possible JUCE API adjustments on the first build.

## Licence

Add your licence here. Note that JUCE has its own licence terms that apply to your distribution.
