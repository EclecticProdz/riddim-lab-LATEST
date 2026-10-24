# v3.1.1 - Control room: the Room choice is now audible

- The three rooms (Studio, Control Room, Live Room) used to differ by a very quiet reverb tail, 17 to 31 dB below the signal, which is easy to miss under music. The ambience is now about four times stronger and the decay longer, so the three are clearly different: Studio is close and dry, Control Room is medium, Live Room is big with a long tail. Measured with a noise burst: the tail 0.35 to 0.6 seconds after it stops is now -38.6 dB (Studio), -30.5 dB (Control Room) and -23.1 dB (Live Room), against about -19 dB for the burst itself.
- Room only does anything while listening is on (a Monitor is lit). Choosing a room while listening is off now switches listening on (Midfield) so you hear the change straight away.
- Still monitor only: it is bypassed when you export or bounce.

# v3.1 - Eight tabs, Dynamics card, living Control room

- **The Master tab is gone from the tab bar** (8 tabs now: Home, Drums, Guitar, Bass, Piano Bang, Horns, Control, Effects).
- **Its controls moved into Effects as a new "Dynamics" card** (fifth card on the Effects hub, also the DYNAMICS label on the dbx compressor in the picture, and the Effects menu): Filter, Compressor, Tone EQ and Limiter, with the control-room picture and the engineer sitting in his chair. A "< EFFECTS" button takes you back.
- A new plugin now opens on Home (it used to open on Master).
- **Control room: no person.** Equipment moves instead, driven by the output level:
  - the two reels of the tape machine turn while there is signal (the reel part of the picture itself is rotated, so it looks like the picture);
  - the two big speaker cones pump a little;
  - the console meter windows brighten and bounce.
  Switch it all off with Animation: Off in the cog menu.
- Not changed yet: clicking Dynamics still selects the Master settings profile, the same as the old Master tab did, so the sound can still change when you move between tabs. That is the audio restructure, which is next.

# v3.0.2 - Engineer seated properly

- **Master:** the engineer is drawn with his chair again, but now scaled and placed so his chair lies exactly over the chair in the room picture (the two are the same kind of chair), so there is one chair and he is sitting in it, with his back, shoulders and leg.
- **Control:** he is bigger and in the foreground, seen from behind, with the bottom of his chair cropped by the edge of the picture, so he is grounded instead of floating. He sits below the stage so the drum kit stays visible.
- Picture only: positions are in RoomFx.h (buildFigures); the engineer has no floor shadow. Resources/fig_engineer.png is the original again. No sound or parameter changes.

# v3.0 - Speed and page fade

- **Smoother dragging and animation.** The interface used to redraw the whole picture underneath (frame, room photograph, panel shadows, textures) every time anything moved: a knob, a lamp glow or a musician. The fixed part of each page is now drawn once and copied, and only the parts that move are redrawn. In my timing tests the screen work behind one animation frame or knob move dropped by about 20 to 100 times, and a whole-page redraw from about 300 ms to about 40 ms (software timings on a cloud machine at retina scale; your Mac will differ).
- Knob, meter, tape-deck and musician pictures are resized once to the exact pixel size they are drawn at, instead of being resized on every repaint.
- Panel drop shadows no longer use a blur pass (that was the slowest part of drawing a page). They look the same at normal viewing size.
- **60 frames a second.** Animation and meters now run on the display refresh instead of 30 Hz. Needle and LED motion keeps the same feel at any frame rate.
- While a knob is being dragged the room animation redraws less often, so the drag gets the processor time.
- **Page fade.** Changing tab dissolves from the old page to the new one over 0.4 seconds. It never blocks clicks and it does not touch the sound.
- **Settings (cog) menu, new Animation section:** Full, Reduced (lighter on slower computers), Off (still pictures, no fade), and a frame timer in the bottom bar that shows frames per second and the longest gap between frames, so you can tell me real numbers.
- No sound or parameter changes in this version.

# v2.9 - Step 8: Moving figures

- Each room now has its musician, cut out from the figure sheet: drummer (Drums), bassist (Bass), guitarist (Guitar), pianist (Piano Bang), sax player (Horns), engineer at the desk (Master and Control).
- They sway gently and bob with the output level, with a soft shadow on the floor. They are still pictures moved as a whole, not separately animated limbs.
- Home: the man sitting on the right now lights his lighter when you click him; smoke drifts up from the flame. Click again to put it out. This replaces the click-the-header-head trigger from v2.7.
- Figure pictures are Resources/fig_*.png (cut from the supplied sheet). Positions are set in RoomFx.h, function buildFigures().

# v2.8 - Step 7: Sidechain kick/bass lock

- Kick + Bass Lock now has three modes: Off, EQ Lock, Sidechain.
- **Sidechain:** the drum lows (under about 100 Hz) duck while the bass plays, scaled by Amount (up to 14 dB). It listens to a host sidechain input if one is routed in (the plugin now has an optional second input), otherwise to a Bass tab instance running in the same host. The panel shows which one it found.
- The Bass tab instance shares a bass level with the Drums instance automatically. The fallback can lag the bass by up to one audio block.
- Existing sessions: LockOn 0/1 mean the same as before (Off / EQ Lock).

# v2.7 - Step 6: Animation

- **Room lighting:** the lamps and bright spots already in each room picture are found automatically and glow and flicker. They pump gently with the output level.
- **Lighter and smoke easter egg:** click the Eclectic Prodz head in the header to light a lighter at the bottom left of the room. Smoke drifts up and sways with the music. Click again to put it out.
- VU needles were already live (since v2.2).
- Not built: moving figures. There is no artwork for them yet.

# v2.6 - Step 5: Control tab (listening)

- **Monitors:** Nearfield, Midfield, Main (voicing of small desk monitors, medium monitors, big mains). Click the lit one again to switch listening off.
- **Room:** Studio, Control Room, Live Room (ambience around the speakers).
- **Speaker position:** Width (speaker angle, 60 deg neutral), Distance (more room, softer top), Toe-in (brighter when turned in).
- **Colour:** Analog amount. **Listening:** Level, Mono, Headphones (crossfeed).
- All of this is monitor only: it is bypassed on offline bounce/export and the settings are shared by every tab. It approximates listening setups, it is not a model of one particular speaker or room.

# v2.5 - Step 4: Drums tab

- **Drum selector:** Kick, Snare, Rim, Hat, Toms, Loop. Each gives the drums its own voicing (Loop leaves the sound as it was).
- **Kick + Bass Lock (EQ lock):** dips the drums under 100 Hz and keeps the kick knock just above it, with an Amount knob. Sidechain lock is not in yet.
- **Drum presets:** eight drum-specific presets (Deep Dub Kick, Tight Kick, Crack Snare, Fat Snare, Rim Click, Hat Tamer, Roots Toms, Loop Glue). Choosing one also sets the drum type.
- Drums page rebuilt: Drum Source (selector + knobs), Compressor (mode now under the knobs), Tone EQ, Kick + Bass Lock, Tape Warmth.

# v2.4 - Step 3: Bass tab

- **Sub 30-50 Hz:** Sub Level (-12 to +12 dB, only that band), Harmonics (adds upper harmonics so the note reads on small speakers), live sub-band meter.
- **Speaker Check:** Phone, Laptop, 14" MacBook, Studio Monitor, plus Level. Monitor only: bypassed during offline bounce/export, never saved in presets. Click the active button again to switch off.
- Bass page rebuilt to five modules: SteadyBass, Tone EQ, Sub, Speaker Check, Tape Warmth.
- Note: Harmonics defaults to 25%, so existing Bass sessions gain a little upper content.

# v2.3 - Step 2: Effects

- **Tape:** Type A Clean / B Weight / C Vintage; Speed 7.5/15/30 IPS (moves head bump and top end); Stability (wow + slow drift); Mix (parallel tape).
- **Spring:** new spring engine, 7 characters (Drip, Roots, Twang, Heavy, Surf, Dark Tank, Shimmer), Short/Med/Long, Drive, Feedback, Treble, Bass, Stereo/Mono. Replaces the old generic reverb, so old sessions will sound different.
- **Plate:** new plate engine with Damping, Low cut, Width, Bright/Natural/Dark.
- **Echo:** playback heads H1/H2/H3 and a Reverb-in-loop knob.
- New factory presets for Tape, Spring, Plate and Echo.
- Tested numerically only. No listening tests yet.

# v2.2 - new interface (build step 1 of the redesign)

* New layout from the approved mock-ups: full-brightness logo header (the dimming vignette is gone), wide meter panel with a Settings cog, walnut tab bar with polished-brass active tab.
* Tabs: HOME, MASTER, DRUMS, GUITAR (Chop / Pick / Rhythm), BASS, PIANO BANG, HORNS, CONTROL, EFFECTS (hub + Echo, Spring, Plate, Tape pages). The Vocal tab is gone from the interface.
* Each tab has its own room picture and its own preset strip (PRESET < name > SAVE). Instrument tabs save the whole tab; each effect page has its own presets and saves only its effect. Whole-plugin presets are in the Settings cog (Save / Load everything).
* Colour-coded modules (green / steel / red / brass / wood) with 0-10 printed knob scales, ribbed faders, recessed buttons, LED columns.
* Master tab now has a compressor (it is off by default, so existing sounds do not change). The Filter module moved from the Guitar and Horns tabs to the Echo page ("Echo Filter").
* The old LIBRARY and PRESET panels are removed; the factory presets are on each tab's preset strip.
* Not built yet (later steps): real Tape A/B/C, real Spring and upgraded Plate/Echo engines, Bass sub meter and speaker check, Drum selector and Kick/Bass lock, Control-room listening, animation and figures.

# Riddim Lab v2.1 - simpler interface, SteadyBass, Piano Bang

## Interface trimmed ("keep the tabs, trim each")
Every tab now shows only what it uses. Layout: INPUT / METERS / OUTPUT on top, then one row of tab-specific panels,
then TAPE MACHINE / DUB ECHO / REVERB, then a SIGNAL FLOW strip listing only the stages that tab really has.
- Master: Filter, Tone EQ, Limiter.  Drums and Vocal: Source, Compressor, Tone EQ.  Guitar and Horns: Source, Filter, Tone EQ.
- Bass: SteadyBass and Tone EQ.  Piano Bang (replaces Keys): Piano Bang knobs with OLD SCHOOL / MODERN voicing buttons, and Tone EQ.
- Removed from the interface and the audio path: modulation (chorus/phaser/flanger), the second delay, and the Hall/Room/Dub reverbs.
  Reverb is now SPRING or PLATE. Dub echo is the existing Echo, renamed DUB ECHO and unchanged.
- Factory presets cleaned up to match.

## SteadyBass (Bass tab)
Weight-preserving bass leveller. Evens out note-to-note level while keeping the fundamental, the attack and the body.
Controls: INTENSITY (how much), PUNCH (how much of each note's attack passes before levelling settles), WEIGHT (how gently the
low fundamental region is treated compared with the upper harmonics). Processes only the incoming bass, generates nothing.
No latency, no look-ahead, no phase change (a gain-only design with a zero-latency 220 Hz crossover that sums back flat).
It self-matches long-term loudness so "on" is not just "louder". New parameter IDs: bass_sbOn, bass_sbAmount, bass_sbPunch, bass_sbWeight.

## Piano Bang
OLD SCHOOL: upright piano on tape (warm low shelf, forward mids, rolled-off top, vintage tape, a little wobble).
MODERN: clean Motif-ES-style stage piano (scooped mids, bright hammer and air, clean tape). Both write the EQ, tape and
BANG / RING / HAMMER knobs in one click; the button lights while the settings still match.

## Measured with synthetic bass lines (Tests/steadybass_tests.cpp, offline)
Note-to-note body level spread 4.4 dB -> about 2.7 dB at default settings (consistent across five random bass lines);
long-term loudness within +/-0.1 dB of bypass at inputs from -25 to -1 dBFS peak; low band (<110 Hz) kept within 0.1 dB;
note attack preserved (onset-to-body ratio not reduced); no added clipping; 3rd harmonic of a steady 50 Hz tone below -90 dBc;
amount 0 is transparent; silence stays silent; zero latency. An ordinary 4:1 compressor measured on the same material
levelled slightly harder (about 2.0 dB spread) but SteadyBass keeps more onset and does it with loudness auto-matched.
Not tested: macOS build, Ableton, listening on real basses and speakers.

# Riddim Lab v2.0 - what changed (earlier release)

## Audio engine (rewritten, every section now does real work)
Fixed the faults found by measuring the old engine: wow/flutter did nothing, the first echo repeat skipped the filter,
resonance never resonated, level dropped at default settings, the reverb was never run, endless low-level echo tails.
New modules (Source/RiddimDSP.h): input gain/saturation, filter (LP/HP/BP, echo-loop or input route), compressor
(Gentle/Punch/Limit), tone EQ, tape machine (Clean/Vintage/Worn, real wow & flutter), dub echo (Tape/Digital/Ping-pong),
rhythm delay, 5 reverbs, chorus/phaser/flanger, analog tone, master limiter. Each instrument tab has its own
settings plus a source voicing (drums, guitar chop/pick/rhythm, bass, keys, horns, vocal).
Old parameter IDs on the Master tab are unchanged, so old sessions still load.

## Interface (rebuilt)
Textured panels, knurled brass knobs, VU + gain-reduction meters, tape deck with turning reels, signal-flow lamps,
library with thumbnails, working preset Load / Save / Delete / Browse. All art is generated by Tools/gen_assets.py.
Logo and title art unchanged. Window can be resized (keeps 1280x900 proportions).

## Tests run (Linux, offline)
~50 measured checks in Tests/ (echo timing, filters, resonance, compressor, EQ, wow/flutter, limiter ceiling,
reverb decay, mod effects, every source knob, random-parameter stress, memory checker). Not tested: macOS build, Ableton, listening.
