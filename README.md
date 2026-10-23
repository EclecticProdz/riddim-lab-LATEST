# Riddim Lab v0.3 — custom dub desk UI

A JUCE/CMake source project for a roots-reggae delay, targeting macOS VST3 and AU.

## What's new in v0.3
- Custom rotary-knob drawing and hardware-inspired panel
- Dedicated delay and tone sections
- Roots Tape, Filtered Skank and Runaway Dub starting-setting buttons
- Hold-to-throw button intended to boost feedback while pressed and return to its saved value on release
- Panic/Clear button
- Smoothed core parameters

## Build status and important caveats
This is source code, not a verified compiled release. It has not been built or tested in Ableton Live in this environment. The momentary button wiring and JUCE parameter API should be compiled and tested against the included JUCE version; if the compiler reports API differences, resolve those before use. The current Panic/Clear action clears the delay buffer and is not a substitute for a monitoring limiter. Feedback is capped below unity, but any high-feedback effect should be tested at low monitoring volume.

The three preset buttons load starting values; they are not yet a formal preset browser or factory preset files. The UI includes custom-drawn knobs, section separators, a metal faceplate look, accent rails and fastener details.

## Build on macOS
Requirements: Xcode Command Line Tools, CMake, and internet access on first configure.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Locate the generated `Riddim Lab.vst3` or `Riddim Lab.component`, install it if needed, restart Ableton Live, and rescan plug-ins.

## Suggested initial use
Put it on a return track or an audio track with a rimshot/skank:
- Tempo sync on; start at dotted eighth
- Mix 20–30%
- Feedback 35–50%
- Filter cutoff 1.5–4 kHz
- Tape heat 15–35%
- Hold-to-throw only when you want an echo swell

Keep monitoring levels conservative during first tests.


## Approved navigation order
Master → Drums → Guitar (Chop / Pick / Rhythm) → Bass → Piano Bang → Horns → Vocal.

See `ARTWORK_INTEGRATION_STATUS.md` and `Assets/` for the current artwork handoff. The mockup and grouped artwork sheets are visual assets; they are not themselves a compiled plugin UI.
