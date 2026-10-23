# Riddim Lab individual sprite pack

This folder contains individually cropped PNG assets extracted from the latest approved-style component board.
CMake embeds every PNG in `RiddimLabIndividualSprites` as JUCE BinaryData.

## Groups
- Rotary controls: large, medium, small, micro knobs
- Switches: vertical toggles, rocker, ON/OFF buttons
- Indicator lamps: amber, green, red, orange, dark
- Meter layers: VU face, empty window, needle
- Tape details: left/right reels, head block, button/meter/switch panels
- Navigation sprites: Master, Drums, Guitar active, Bass, Keys, Horns, Vocal; Chop/Pick/Rhythm
- Panel hardware: green/brass panels, screw, crown, Rasta stripe

## Transparency / production note
Sprites are cropped individually and background flood-fill transparency is applied. Because the source was a rendered component board rather than layered source art, some edge pixels may retain background halos and a few sprites may need manual cleanup. Meter face and needle are separate layers, but the needle needs transform/pivot alignment in the UI code.

## Build
CMake compiles the sprite PNGs into a generated BinaryData target. This is source integration only; a successful JUCE build and visual QA are still required.
