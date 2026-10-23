# Riddim Lab live control integration — current handoff

## Image-based UI
- Large rotary control sprite is embedded as JUCE BinaryData and used by the rotary LookAndFeel; its pointer tracks the real slider value.
- Switch component sheet is embedded and used by the ToggleButton LookAndFeel, with a state-dependent indicator.
- Tape-machine component sheet is embedded and drawn in the editor.
- Meter component sheet is embedded and drawn; meter needles are refreshed at 30 Hz using post-processing block peak values exposed through atomics.
- These are grouped/cropped sprites from the generated component board, not a full set of individually authored transparent images for every unique control. Some drawing coordinates are prototype coordinates and require visual QA at plugin size.

## Instrument processing
- Seven section-specific parameter groups: Drive, Tone, Compression.
- Each section has separate filter state and a distinct processing profile.
- Guitar has a `guitarMode` parameter (Chop / Pick / Rhythm) plus dedicated controls for Chop Tightness, Pick Brightness, and Rhythm Body.
- Existing tape/delay DSP remains in the chain.
- DSP has not been professionally tuned or listening-tested. It is an initial engineering implementation requiring compilation, level-matched listening tests, and refinement.

## Build verification
Not compiled in this environment. GitHub Actions build must be run before treating this as a working AU/VST3.
