# Riddim Lab - interface rebuild

## What changed
- Source/PluginEditor.h / PluginEditor.cpp: fully rewritten. One layout function (resized) computes every
  section from the 1280 x 900 window, so nothing is placed with loose coordinates.
- CMakeLists.txt: only Assets/Artwork/header_art.png is embedded now (logo + title). Panels, knobs, VU meters,
  tape reels and lamps are drawn in code. COPY_PLUGIN_AFTER_BUILD is FALSE (CI runner has no install folder).
- Assets/Artwork/header_art.png: new, cropped from the supplied mockup.
- .github/workflows/build.yml: universal build (Apple Silicon + Intel), ad-hoc codesign, packaging.

## Not changed
- Source/PluginProcessor.cpp and PluginProcessor.h are byte-identical to the uploaded version: DSP, delay engine,
  parameter IDs/ranges, state save/restore.

## Honest status
- Compile-checked against JUCE 8.0.6 headers with clang (no errors, no warnings). NOT built on macOS, NOT run
  in Ableton Live, and the rendered look has NOT been seen. Layout was checked by arithmetic only.
- Presets 3-8 in the library are new starting values written for this version; they have not been tuned by ear.
- The current preset highlight is not saved with the project (parameter values are).
- The "Roots Pressure" bass feature does not exist in the code and has not been added.
- Per-section Drive/Tone/Comp knobs control the existing section parameters; the DSP behind them is the
  earlier "initial engineering implementation" and has not been listening-tested.
