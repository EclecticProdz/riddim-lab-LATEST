#include "PluginEditor.h"
#include "PluginProcessor.h"

using namespace juce;
using rp::P;

namespace
{
    struct FactoryPreset { const char* name; const char* sub; std::vector<std::pair<P, float>> v; };

    const std::vector<FactoryPreset>& factoryPresets()
    {
        static const std::vector<FactoryPreset> f
        {
            { "KINGSTON SESSION", "Tape warmth, medium echo",
              { { P::Feedback, 0.42f }, { P::Mix, 0.26f }, { P::Cutoff, 2800 }, { P::Resonance, 0.35f }, { P::Heat, 0.28f }, { P::Division, 2 },
                { P::TapeSat, 0.18f }, { P::TapeTone, 0.72f }, { P::Wow, 0.05f }, { P::Flutter, 0.04f }, { P::Wear, 0.10f }, { P::TapeType, 1 },
                { P::RevType, 0 }, { P::RevMix, 0.10f }, { P::RevDecay, 0.40f } } },
            { "FOUNDATION RIDDIMS", "Filtered chop echo, vintage tape",
              { { P::Feedback, 0.52f }, { P::Mix, 0.31f }, { P::Cutoff, 2100 }, { P::Resonance, 0.48f }, { P::Heat, 0.36f }, { P::Division, 1 },
                { P::TapeSat, 0.32f }, { P::TapeTone, 0.60f }, { P::Wow, 0.08f }, { P::Flutter, 0.06f }, { P::Wear, 0.20f }, { P::TapeType, 1 },
                { P::RevType, 0 }, { P::RevMix, 0.14f }, { P::RevDecay, 0.45f }, { P::CompOn, 1 }, { P::CompThresh, -16 }, { P::CompRatio, 3 }, { P::CompMakeup, 2 } } },
            { "ROOTS & CULTURE", "Dark worn tape, long repeats",
              { { P::Feedback, 0.38f }, { P::Mix, 0.22f }, { P::Cutoff, 3200 }, { P::Resonance, 0.30f }, { P::Heat, 0.22f }, { P::Division, 0 },
                { P::TapeSat, 0.30f }, { P::TapeTone, 0.55f }, { P::Wow, 0.10f }, { P::Flutter, 0.06f }, { P::Wear, 0.25f }, { P::TapeType, 2 },
                { P::Width, 0.18f }, { P::Damping, 0.50f }, { P::RevType, 1 }, { P::RevMix, 0.12f }, { P::RevDecay, 0.55f } } },
            { "DUB SESSIONS", "Runaway feedback, spring verb",
              { { P::Feedback, 0.74f }, { P::Mix, 0.36f }, { P::Cutoff, 3600 }, { P::Resonance, 0.62f }, { P::Heat, 0.45f }, { P::Division, 5 },
                { P::TapeSat, 0.42f }, { P::TapeTone, 0.68f }, { P::Wow, 0.12f }, { P::Flutter, 0.09f }, { P::Wear, 0.30f }, { P::TapeType, 1 },
                { P::RevType, 0 }, { P::RevMix, 0.22f }, { P::RevDecay, 0.70f } } },
            { "LOVERS ROCK", "Soft, bright repeats, plate verb",
              { { P::Feedback, 0.34f }, { P::Mix, 0.24f }, { P::Cutoff, 4500 }, { P::Resonance, 0.25f }, { P::Heat, 0.18f }, { P::Division, 2 },
                { P::TapeSat, 0.15f }, { P::TapeTone, 0.80f }, { P::Wow, 0.06f }, { P::Flutter, 0.03f }, { P::Wear, 0.05f }, { P::TapeType, 0 },
                { P::Width, 0.22f }, { P::Damping, 0.35f }, { P::RevType, 1 }, { P::RevMix, 0.20f }, { P::RevDecay, 0.50f } } },
            { "ROCKSTEADY", "Short, clean slap echo",
              { { P::Feedback, 0.26f }, { P::Mix, 0.20f }, { P::Cutoff, 5200 }, { P::Resonance, 0.20f }, { P::Heat, 0.20f }, { P::Division, 4 }, { P::EchoType, 1 },
                { P::TapeSat, 0.22f }, { P::TapeTone, 0.70f }, { P::Wow, 0.04f }, { P::Flutter, 0.03f }, { P::Wear, 0.08f }, { P::TapeType, 0 },
                { P::RevType, 0 }, { P::RevMix, 0.12f }, { P::RevDecay, 0.30f } } },
            { "DANCEHALL VIBES", "Bright ping-pong echoes",
              { { P::Feedback, 0.48f }, { P::Mix, 0.30f }, { P::Cutoff, 6500 }, { P::Resonance, 0.40f }, { P::Heat, 0.30f }, { P::Division, 3 }, { P::EchoType, 2 },
                { P::TapeSat, 0.20f }, { P::TapeTone, 0.85f }, { P::Wow, 0.03f }, { P::Flutter, 0.02f }, { P::Wear, 0.0f }, { P::TapeType, 0 },
                { P::Width, 0.25f }, { P::Damping, 0.30f }, { P::RevType, 1 }, { P::RevMix, 0.10f } } },
            { "MIX TEMPLATES", "Glue, tape and a touch of air",
              { { P::Feedback, 0.20f }, { P::Mix, 0.08f }, { P::Cutoff, 6000 }, { P::Resonance, 0.2f }, { P::Heat, 0.1f }, { P::Division, 1 },
                { P::CompOn, 1 }, { P::CompThresh, -14 }, { P::CompRatio, 2 }, { P::CompMakeup, 2 }, { P::CompMode, 0 },
                { P::EqLow, 1 }, { P::EqHigh, 1.5f }, { P::TapeSat, 0.25f }, { P::TapeType, 0 }, { P::TapeTone, 0.85f },
                { P::Wow, 0.0f }, { P::Flutter, 0.0f }, { P::Wear, 0.0f }, { P::RevType, 1 }, { P::RevMix, 0.05f }, { P::RevDecay, 0.4f } } }
        };
        return f;
    }

    String srcHelp (rp::Sec s, int mode, int i)
    {
        static const char* drums[3]  { "Boost or soften the front of every hit", "Lengthen or tighten the ring after the hit", "Add stick crack in the upper mids" };
        static const char* chop[3]   { "How quickly each chord is cut off", "How far the signal drops between chops", "Pick click on the chop" };
        static const char* pick[3]   { "Lift the pick sound", "Sharpen or soften the pluck", "High-pass the body to thin the guitar" };
        static const char* rhythm[3] { "Low-mid weight", "Round, saturated top end", "Gentle compression to bind the strum" };
                static const char* keys[3]   { "The bang: boost or soften the front of every note", "How long the note rings after the hit", "Hammer click in the upper mids" };
        static const char* horns[3]  { "Presence around 2.4 kHz", "Weight around 350 Hz", "Round off the top end" };
        static const char* vocal[3]  { "Tame harsh sibilance", "Add sparkle above 11 kHz", "Chest warmth and a little saturation" };
        switch (s)
        {
            case rp::Drums:  return drums[i];
            case rp::Guitar: return mode == 0 ? chop[i] : (mode == 1 ? pick[i] : rhythm[i]);
            case rp::Keys:   return keys[i];
            case rp::Horns:  return horns[i];
            case rp::Vocal:  return vocal[i];
            default:         return {};
        }
    }

    const char* kHelp (P p)
    {
        switch (p)
        {
            case P::InGain: return "Level into the processing chain";
            case P::OutGain: return "Final level after all processing";
            case P::Cutoff: return "Filter frequency. Sweep it for classic dub filtering";
            case P::Resonance: return "Filter emphasis at the cutoff";
            case P::CompThresh: return "Level above which compression starts";
            case P::CompRatio: return "How hard peaks above the threshold are squashed";
            case P::CompMakeup: return "Gain added back after compression";
            case P::EqLow: return "Low shelf at 100 Hz";
            case P::EqMid: return "Mid bell at 800 Hz";
            case P::EqHigh: return "High shelf at 7 kHz";
            case P::EqPres: return "Presence bell at 3.5 kHz";
            case P::TapeSat: return "Tape saturation and compression";
            case P::TapeTone: return "Tape head tone: low is dark, high is open";
            case P::Wow: return "Slow pitch drift, like a tired tape transport";
            case P::Flutter: return "Fast pitch shimmer";
            case P::Wear: return "Worn tape: loses highs and adds hiss";
            case P::Feedback: return "How many repeats the echo makes";
            case P::Mix: return "Echo level against the dry signal";
            case P::Heat: return "Saturation inside the echo loop; repeats get dirtier";
            case P::Width: return "Offsets the right echo time from the left";
            case P::Damping: return "How fast repeats lose their highs";
            case P::RevPre: return "Delay before the reverb starts";
            case P::RevDecay: return "Reverb length";
            case P::RevMix: return "Reverb level";
            case P::SbAmount: return "How strongly the notes are evened out. Only the incoming bass is processed";
            case P::SbPunch: return "Higher lets more of each note's attack through before the levelling settles";
            case P::SbWeight: return "Higher keeps the low fundamental fuller; the upper harmonics are levelled more";
            case P::CtlWidth: return "Angle between the two speakers. 60 degrees is the classic equilateral listening triangle";
            case P::CtlDist: return "How far you sit from the speakers: more distance means more room and a softer top end";
            case P::CtlToe: return "How far the speakers are turned in towards you: more toe-in is brighter and more direct";
            case P::CtlColour: return "A little analog console and tape colour on the monitor path";
            case P::CtlLevel: return "Listening level only. It does not change your mix";
            case P::LockAmt: return "How much room the drums leave for the bass. EQ lock: a fixed cut under 100 Hz. Sidechain: the drum lows duck only while the bass plays";
            case P::SubLevel: return "Raises or lowers only the 30 to 50 Hz band";
            case P::SubHarm: return "Adds upper harmonics of the sub note so it still reads on small speakers";
            case P::SpkLevel: return "Level of the speaker check, to match how loud the real speaker would be";
            case P::TapeStab: return "Lower values let the tape transport drift and wobble more";
            case P::TapeMix: return "Blend of tape against the clean signal (parallel tape)";
            case P::SpDrive: return "Pushes the signal into the spring driver for more crunch and drip";
            case P::SpFb: return "Extra regeneration: more of the spring signal travels around again";
            case P::SpTreble: return "Brightness of the spring return";
            case P::SpBass: return "Low end of the spring return";
            case P::PlDamp: return "How quickly the plate loses its highs as it decays";
            case P::PlLowCut: return "Removes low frequencies from the reverb so it stays out of the bass";
            case P::PlWidth: return "Stereo spread of the plate: 0 is mono";
            case P::EchoRev: return "Smears every repeat into a small reverb, like a space echo with reverb in the loop";
            case P::LimThresh: return "Level where the limiter starts working";
            case P::LimRelease: return "How fast the limiter lets go";
            case P::LimCeiling: return "The output can never go above this level";
            default: return "";
        }
    }
}

//==============================================================================
// Preset banks. Instrument tabs use the full factory list and save the whole tab;
// each effect page has its own small bank and only touches its own effect.
namespace
{
    const std::vector<FactoryPreset>& drumPresets()
    {
        static const std::vector<FactoryPreset> f
        {
            { "DEEP DUB KICK",  "Kick: weight low, boxiness out, bass lock on", { { P::DrumType, 0 }, { P::Src1, 0.50f }, { P::Src2, 0.55f }, { P::Src3, 0.20f }, { P::CompOn, 1 }, { P::CompThresh, -16 }, { P::CompRatio, 4 }, { P::CompMakeup, 3 }, { P::CompMode, 1 }, { P::EqLow, 2 }, { P::EqMid, -1 }, { P::TapeSat, 0.30f }, { P::LockOn, 1 }, { P::LockAmt, 0.5f } } },
            { "TIGHT KICK",     "Kick: short, clicky, dry",                     { { P::DrumType, 0 }, { P::Src1, 0.70f }, { P::Src2, 0.20f }, { P::Src3, 0.35f }, { P::CompOn, 1 }, { P::CompThresh, -14 }, { P::CompRatio, 5 }, { P::CompMakeup, 2 }, { P::CompMode, 1 }, { P::EqLow, 1 }, { P::EqMid, -2.5f }, { P::LockOn, 1 }, { P::LockAmt, 0.6f } } },
            { "CRACK SNARE",    "Snare: bright crack, quick tail",              { { P::DrumType, 1 }, { P::Src1, 0.70f }, { P::Src2, 0.35f }, { P::Src3, 0.75f }, { P::CompOn, 1 }, { P::CompThresh, -16 }, { P::CompRatio, 4 }, { P::CompMakeup, 3 }, { P::CompMode, 1 }, { P::EqHigh, 2 } } },
            { "FAT SNARE",      "Snare: body and tape weight",                  { { P::DrumType, 1 }, { P::Src1, 0.50f }, { P::Src2, 0.65f }, { P::Src3, 0.40f }, { P::CompOn, 1 }, { P::CompThresh, -18 }, { P::CompRatio, 3 }, { P::CompMakeup, 3 }, { P::EqLow, 1.5f }, { P::TapeSat, 0.40f }, { P::TapeType, 1 }, { P::RevMix, 0.14f }, { P::RevDecay, 0.35f } } },
            { "RIM CLICK",      "Rim and sidestick: woody click",               { { P::DrumType, 2 }, { P::Src1, 0.65f }, { P::Src2, 0.15f }, { P::Src3, 0.50f }, { P::RevMix, 0.12f }, { P::RevDecay, 0.30f } } },
            { "HAT TAMER",      "Hats: smooth the harsh top",                   { { P::DrumType, 3 }, { P::Src1, 0.35f }, { P::Src2, 0.30f }, { P::Src3, 0.20f }, { P::EqHigh, -2 }, { P::TapeSat, 0.20f }, { P::TapeTone, 0.70f } } },
            { "ROOTS TOMS",     "Toms: round and boomy, spring tail",           { { P::DrumType, 4 }, { P::Src1, 0.55f }, { P::Src2, 0.60f }, { P::Src3, 0.30f }, { P::CompOn, 1 }, { P::CompThresh, -16 }, { P::CompRatio, 3 }, { P::CompMakeup, 2 }, { P::RevMix, 0.20f }, { P::RevDecay, 0.45f } } },
            { "LOOP GLUE",      "Full loop: gentle glue and tape",              { { P::DrumType, 5 }, { P::CompOn, 1 }, { P::CompThresh, -14 }, { P::CompRatio, 2.5f }, { P::CompMakeup, 2 }, { P::CompMode, 0 }, { P::TapeSat, 0.25f }, { P::TapeType, 0 }, { P::LockOn, 1 }, { P::LockAmt, 0.35f } } }
        };
        return f;
    }
    const std::vector<FactoryPreset>& tapePresets()
    {
        static const std::vector<FactoryPreset> f
        {
            { "CLEAN GLUE",    "A  Clean, 30 ips, closed top end", { { P::TapeType, 0 }, { P::TapeSpeed, 2 }, { P::TapeSat, 0.15f }, { P::TapeTone, 0.85f }, { P::Wow, 0.0f }, { P::Flutter, 0.0f }, { P::Wear, 0.0f }, { P::TapeStab, 1.0f }, { P::TapeMix, 1.0f } } },
            { "WEIGHT",        "B  Weight, 15 ips, fat low end",   { { P::TapeType, 1 }, { P::TapeSpeed, 1 }, { P::TapeSat, 0.40f }, { P::TapeTone, 0.70f }, { P::Wow, 0.02f }, { P::Flutter, 0.02f }, { P::Wear, 0.0f }, { P::TapeStab, 1.0f }, { P::TapeMix, 1.0f } } },
            { "VINTAGE WARM",  "C  Vintage, 15 ips, soft and moving", { { P::TapeType, 2 }, { P::TapeSpeed, 1 }, { P::TapeSat, 0.38f }, { P::TapeTone, 0.60f }, { P::Wow, 0.05f }, { P::Flutter, 0.04f }, { P::Wear, 0.10f }, { P::TapeStab, 0.9f }, { P::TapeMix, 1.0f } } },
            { "DUB TAPE",      "B  Weight, 7.5 ips, pushed",       { { P::TapeType, 1 }, { P::TapeSpeed, 0 }, { P::TapeSat, 0.55f }, { P::TapeTone, 0.55f }, { P::Wow, 0.08f }, { P::Flutter, 0.06f }, { P::Wear, 0.20f }, { P::TapeStab, 0.8f }, { P::TapeMix, 1.0f } } },
            { "WORN CASSETTE", "C  Vintage, 7.5 ips, dark and noisy", { { P::TapeType, 2 }, { P::TapeSpeed, 0 }, { P::TapeSat, 0.45f }, { P::TapeTone, 0.45f }, { P::Wow, 0.14f }, { P::Flutter, 0.10f }, { P::Wear, 0.35f }, { P::TapeStab, 0.5f }, { P::TapeMix, 1.0f } } },
            { "PARALLEL GLUE", "A  Clean, half wet, keeps the attack", { { P::TapeType, 0 }, { P::TapeSpeed, 1 }, { P::TapeSat, 0.60f }, { P::TapeTone, 0.80f }, { P::Wow, 0.0f }, { P::Flutter, 0.0f }, { P::Wear, 0.0f }, { P::TapeStab, 1.0f }, { P::TapeMix, 0.5f } } }
        };
        return f;
    }
    const std::vector<FactoryPreset>& springPresets()
    {
        static const std::vector<FactoryPreset> f
        {
            { "DRIP",       "Short, splashy",          { { P::RevType, 0 }, { P::SpChar, 0 }, { P::SpLen, 0 }, { P::RevPre, 10 }, { P::RevDecay, 0.40f }, { P::RevMix, 0.18f }, { P::SpDrive, 0.30f } } },
            { "ROOTS",      "Classic medium spring",   { { P::RevType, 0 }, { P::SpChar, 1 }, { P::SpLen, 1 }, { P::RevPre, 20 }, { P::RevDecay, 0.45f }, { P::RevMix, 0.20f } } },
            { "TWANG",      "Tight, bright, thin",     { { P::RevType, 0 }, { P::SpChar, 2 }, { P::SpLen, 0 }, { P::RevPre, 5 },  { P::RevDecay, 0.40f }, { P::RevMix, 0.18f }, { P::SpTreble, 0.65f }, { P::SpBass, 0.35f } } },
            { "HEAVY",      "Deep, long, thick",       { { P::RevType, 0 }, { P::SpChar, 3 }, { P::SpLen, 2 }, { P::RevPre, 25 }, { P::RevDecay, 0.55f }, { P::RevMix, 0.22f }, { P::SpBass, 0.60f } } },
            { "SURF",       "Wobbly boing",            { { P::RevType, 0 }, { P::SpChar, 4 }, { P::SpLen, 1 }, { P::RevPre, 10 }, { P::RevDecay, 0.50f }, { P::RevMix, 0.26f }, { P::SpDrive, 0.45f } } },
            { "DARK TANK",  "Dull and deep",           { { P::RevType, 0 }, { P::SpChar, 5 }, { P::SpLen, 1 }, { P::RevPre, 30 }, { P::RevDecay, 0.55f }, { P::RevMix, 0.16f }, { P::SpTreble, 0.35f } } },
            { "SHIMMER",    "Airy, moving top end",    { { P::RevType, 0 }, { P::SpChar, 6 }, { P::SpLen, 2 }, { P::RevPre, 15 }, { P::RevDecay, 0.60f }, { P::RevMix, 0.22f }, { P::SpTreble, 0.65f } } }
        };
        return f;
    }
    const std::vector<FactoryPreset>& platePresets()
    {
        static const std::vector<FactoryPreset> f
        {
            { "BRIGHT PLATE",  "Open and shiny",  { { P::RevType, 1 }, { P::RevPre, 15 }, { P::RevDecay, 0.45f }, { P::RevMix, 0.20f }, { P::PlTone, 0 }, { P::PlDamp, 0.25f }, { P::PlLowCut, 0.20f } } },
            { "NATURAL PLATE", "Even and smooth", { { P::RevType, 1 }, { P::RevPre, 20 }, { P::RevDecay, 0.50f }, { P::RevMix, 0.16f }, { P::PlTone, 1 }, { P::PlDamp, 0.40f }, { P::PlLowCut, 0.15f } } },
            { "DARK PLATE",    "Soft top end",    { { P::RevType, 1 }, { P::RevPre, 30 }, { P::RevDecay, 0.65f }, { P::RevMix, 0.18f }, { P::PlTone, 2 }, { P::PlDamp, 0.60f }, { P::PlLowCut, 0.15f } } },
            { "BIG PLATE",     "Long and wide",   { { P::RevType, 1 }, { P::RevPre, 40 }, { P::RevDecay, 0.85f }, { P::RevMix, 0.22f }, { P::PlTone, 1 }, { P::PlDamp, 0.35f }, { P::PlWidth, 1.0f } } },
            { "TIGHT PLATE",   "Short, drum-friendly", { { P::RevType, 1 }, { P::RevPre, 5 }, { P::RevDecay, 0.20f }, { P::RevMix, 0.16f }, { P::PlTone, 0 }, { P::PlDamp, 0.50f }, { P::PlLowCut, 0.40f } } }
        };
        return f;
    }
    const std::vector<FactoryPreset>& echoPresets()
    {
        static const std::vector<FactoryPreset> f
        {
            { "SLAP",        "Short, clean slap",        { { P::EchoType, 1 }, { P::Feedback, 0.26f }, { P::Mix, 0.20f }, { P::Heat, 0.20f }, { P::Division, 4 }, { P::Width, 0.0f }, { P::Damping, 0.35f }, { P::Head1, 1 } } },
            { "DUB WARM",    "Tape echo, dark repeats",  { { P::EchoType, 0 }, { P::Feedback, 0.45f }, { P::Mix, 0.28f }, { P::Heat, 0.30f }, { P::Division, 2 }, { P::Width, 0.10f }, { P::Damping, 0.50f }, { P::Head1, 1 } } },
            { "RUNAWAY",     "Feedback pushed hard",     { { P::EchoType, 0 }, { P::Feedback, 0.74f }, { P::Mix, 0.36f }, { P::Heat, 0.45f }, { P::Division, 5 }, { P::Width, 0.18f }, { P::Damping, 0.55f }, { P::Head1, 1 } } },
            { "PING PONG",   "Bright bouncing repeats",  { { P::EchoType, 2 }, { P::Feedback, 0.48f }, { P::Mix, 0.30f }, { P::Heat, 0.25f }, { P::Division, 3 }, { P::Width, 0.25f }, { P::Damping, 0.30f }, { P::Head1, 1 } } },
            { "THREE HEADS", "Heads 1+2+3, dense repeats", { { P::EchoType, 0 }, { P::Feedback, 0.40f }, { P::Mix, 0.30f }, { P::Heat, 0.30f }, { P::Division, 1 }, { P::Width, 0.08f }, { P::Damping, 0.45f }, { P::Head1, 1 }, { P::Head2, 1 }, { P::Head3, 1 } } },
            { "SPACE ECHO",  "Heads 2+3 into the reverb", { { P::EchoType, 0 }, { P::Feedback, 0.50f }, { P::Mix, 0.32f }, { P::Heat, 0.35f }, { P::Division, 2 }, { P::Width, 0.10f }, { P::Damping, 0.45f }, { P::Head1, 0 }, { P::Head2, 1 }, { P::Head3, 1 }, { P::EchoRev, 0.55f } } }
        };
        return f;
    }
    const char* kPageNames[] { "HOME", "MASTER", "DRUMS", "GUITAR", "BASS", "PIANO BANG", "HORNS", "CONTROL", "EFFECTS", "TAPE", "SPRING", "PLATE", "ECHO" };
}

struct MainUI::Bank { String scope; const std::vector<FactoryPreset>* list; std::vector<P> savable; };

const MainUI::Bank& MainUI::bankFor (Page pg) const
{
    static const Bank none { "", nullptr, {} };
    static const Bank tabBank { "tab", &factoryPresets(), {} };
    static const Bank drumBank { "drums", &drumPresets(), {} };
    static const Bank tapeBank { "tape", &tapePresets(), { P::TapeType, P::TapeSat, P::TapeTone, P::Wow, P::Flutter, P::Wear, P::TapeSpeed, P::TapeStab, P::TapeMix } };
    static const Bank springBank { "spring", &springPresets(), { P::RevType, P::RevPre, P::RevDecay, P::RevMix, P::SpChar, P::SpLen, P::SpDrive, P::SpFb, P::SpTreble, P::SpBass, P::SpStereo } };
    static const Bank plateBank { "plate", &platePresets(), { P::RevType, P::RevPre, P::RevDecay, P::RevMix, P::PlDamp, P::PlLowCut, P::PlWidth, P::PlTone } };
    static const Bank echoBank { "echo", &echoPresets(), { P::EchoType, P::Feedback, P::Mix, P::Heat, P::Width, P::Damping, P::Division, P::EchoTime, P::Head1, P::Head2, P::Head3, P::EchoRev } };
    if (pg == PDrums) return drumBank;
    if (isInstrumentPage (pg)) return tabBank;
    switch (pg) { case PTape: return tapeBank; case PSpring: return springBank; case PPlate: return plateBank; case PEcho: return echoBank; default: return none; }
}

//==============================================================================
MainUI::MainUI (RiddimLabAudioProcessor& p) : proc (p)
{
    setLookAndFeel (&look);
    ui::Assets::get();
    setBufferedToImage (true);
    setOpaque (true);

    auto info = [this] (const String& s) { setInfo (s); };

    // ---- tab bar
    {
        const char* names[9] { "HOME", "MASTER", "DRUMS", "GUITAR", "BASS", "PIANO BANG", "HORNS", "CONTROL", "EFFECTS" };
        const Page pages[9] { PHome, PMaster, PDrums, PGuitar, PBass, PKeys, PHorns, PControl, PFx };
        for (int i = 0; i < 9; ++i)
        {
            tab[i] = std::make_unique<ui::NavTab> (names[i], i == 3 || i == 8);
            auto* t = tab[i].get();
            t->onClick = [this, i, pg = pages[i], t]
            {
                if (t->arrowHit && i == 3) { showGuitarMenu(); return; }
                if (t->arrowHit && i == 8) { showEffectsMenu(); return; }
                selectPage (pg, gMode, true);
            };
            addAndMakeVisible (*t);
        }
    }
    cog.onClick = [this] { showSettingsMenu(); };
    addAndMakeVisible (cog);

    // ---- knobs (captions as in the mock-ups; the layout sets diameters and colours)
    using K = P;
    addKnob (K::InGain, "GAIN", 58, 3);   addKnob (K::OutGain, "LEVEL", 58, 3);
    addKnob (K::Cutoff, "CUTOFF", 58, 1);  addKnob (K::Resonance, "RESONANCE", 58, 0);
    addKnob (K::CompThresh, "THRESHOLD", 58, 3); addKnob (K::CompRatio, "RATIO", 58, 4); addKnob (K::CompMakeup, "MAKEUP", 58, 3);
    addKnob (K::EqLow, "LOW", 58, 3); addKnob (K::EqMid, "MID", 58, 3); addKnob (K::EqHigh, "HIGH", 58, 3); addKnob (K::EqPres, "PRESENCE", 58, 3);
    addKnob (K::TapeSat, "DRIVE", 58, 0); addKnob (K::TapeTone, "TONE", 58, 0); addKnob (K::Wow, "WOW", 58, 0);
    addKnob (K::Flutter, "FLUTTER", 58, 0); addKnob (K::Wear, "WEAR", 58, 0);
    echoTimeKnob = addKnob (K::EchoTime, "TIME", 58, 2, true);
    addKnob (K::Feedback, "FEEDBACK", 58, 0); addKnob (K::Mix, "MIX", 58, 0);
    addKnob (K::Heat, "TAPE HEAT", 58, 0); addKnob (K::Width, "OFFSET", 58, 0); addKnob (K::Damping, "DAMPING", 58, 0);
    addKnob (K::RevPre, "PRE-DELAY", 58, 2); addKnob (K::RevDecay, "DECAY", 58, 0); addKnob (K::RevMix, "MIX", 58, 0);
    addKnob (K::SbAmount, "INTENSITY", 58, 0); addKnob (K::SbPunch, "PUNCH", 58, 0); addKnob (K::SbWeight, "WEIGHT", 58, 0);
    for (auto k : { K::LimThresh, K::LimRelease, K::LimCeiling })
    {
        auto* kn = addKnob (k, k == K::LimThresh ? "THRESHOLD" : (k == K::LimRelease ? "RELEASE" : "CEILING"), 58, k == K::LimRelease ? 2 : 3);
        limKnobs.push_back (kn);
    }
    for (int i = 0; i < 3; ++i) srcKnobs.push_back (addKnob (K::Src1, "", 58, 0));
    {
        int idx = 0;
        for (auto& kd : knobs) if (kd->p == K::Src1) { kd->p = (P) ((int) K::Src1 + idx); ++idx; }   // distinguished by index; rebound in rebindAll
    }
    for (auto& kd : knobs) { kd->k->onInfo = info; if (kd->p < K::Src1 || kd->p > K::Src9) kd->k->help = kHelp (kd->p); }
    addKnob (K::TapeStab, "STABILITY", 58, 0); addKnob (K::TapeMix, "MIX", 58, 0);
    addKnob (K::SpDrive, "DRIVE", 58, 0); addKnob (K::SpFb, "FEEDBACK", 58, 0); addKnob (K::SpTreble, "TREBLE", 58, 0); addKnob (K::SpBass, "BASS", 58, 0);
    addKnob (K::CtlWidth, "WIDTH", 58, 9); addKnob (K::CtlDist, "DISTANCE", 58, 8); addKnob (K::CtlToe, "TOE-IN", 58, 0);
    addKnob (K::CtlColour, "ANALOG", 58, 0); addKnob (K::CtlLevel, "LEVEL", 58, 3);
    addKnob (K::LockAmt, "AMOUNT", 58, 0);
    addKnob (K::SubLevel, "SUB LEVEL", 58, 3); addKnob (K::SubHarm, "HARMONICS", 58, 0); addKnob (K::SpkLevel, "LEVEL", 58, 3);
    addKnob (K::PlDamp, "DAMPING", 58, 0); addKnob (K::PlLowCut, "LOW CUT", 58, 0); addKnob (K::PlWidth, "WIDTH", 58, 0); addKnob (K::EchoRev, "REVERB", 58, 0);
    for (auto& kd : knobs) { kd->k->onInfo = info; if (kd->p < K::Src1 || kd->p > K::Src9) { if (kd->k->help.isEmpty()) kd->k->help = kHelp (kd->p); } }
    for (int i = 0; i < 4; ++i) { cardKnob[i] = std::make_unique<ui::Knob> ("MIX", 58); addChildComponent (*cardKnob[i]); cardKnob[i]->onInfo = info; }

    // ---- lamps and choices
    lSat = addLamp (std::make_unique<ui::Lamp> (ui::Lamp::LampLabelRight, "SATURATION", 5.5f));
    lAnalog = addLamp (std::make_unique<ui::Lamp> (ui::Lamp::LampLabelRight, "ANALOG TONE", 5.5f));
    lSync = addLamp (std::make_unique<ui::Lamp> (ui::Lamp::Plate, "TEMPO SYNC"));
    lMono = addLamp (std::make_unique<ui::Lamp> (ui::Lamp::Plate, "MONO")); lPhones = addLamp (std::make_unique<ui::Lamp> (ui::Lamp::Plate, "HEADPHONES"));
    lMono->onInfo = lPhones->onInfo = info;
    cLock = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "OFF", "EQ LOCK", "SIDECHAIN" }, true, 12.5f));
    lSoft = addLamp (std::make_unique<ui::Lamp> (ui::Lamp::Plate, "SOFT CLIP"));
    lBypass = addLamp (std::make_unique<ui::Lamp> (ui::Lamp::Plate, "BYPASS"));
    lThrow = addLamp (std::make_unique<ui::Lamp> (ui::Lamp::Momentary, "THROW"));
    lPanic = addLamp (std::make_unique<ui::Lamp> (ui::Lamp::Momentary, "PANIC"));
    auto pw = [&] { return addLamp (std::make_unique<ui::Lamp> (ui::Lamp::LampOnly, "", 5.0f)); };
    pFilt = pw(); pComp = pw(); pEq = pw(); pTape = pw(); pEcho = pw(); pRev = pw(); pLim = pw(); pSrc = pw(); pSteady = pw();
    for (int i = 0; i < 9; ++i) flow[i] = addLamp (std::make_unique<ui::Lamp> (ui::Lamp::LampOnly, "", 7.0f));
    for (int i = 0; i < 4; ++i) cardLamp[i] = addLamp (std::make_unique<ui::Lamp> (ui::Lamp::LampOnly, "", 5.0f));
    lSat->onInfo = lAnalog->onInfo = lSync->onInfo = lSoft->onInfo = lBypass->onInfo = lThrow->onInfo = lPanic->onInfo = info;

    cFilterMode  = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "LOW-PASS", "HIGH-PASS", "BAND-PASS" }, true, 12.5f));
    cFilterRoute = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "ECHO", "INPUT" }, true, 12.5f));
    cCompMode    = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "GENTLE", "PUNCH", "LIMIT" }, true, 12.5f));
    cTapeType    = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "A  CLEAN", "B  WEIGHT", "C  VINTAGE" }, true, 12.5f));
    cEchoType    = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "TAPE", "DIGITAL", "PING-PONG" }, true, 12.5f));
    cTapeSpeed   = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "7.5 IPS", "15 IPS", "30 IPS" }, true, 12.5f));
    cSpChar      = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "DRIP", "ROOTS", "TWANG", "HEAVY", "SURF", "DARK TANK", "SHIMMER" }, true, 12.5f));
    cSpLen       = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "SHORT", "MED", "LONG" }, true, 12.5f));
    cSpStereo    = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "STEREO", "MONO" }, true, 12.5f));
    cPlTone      = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "BRIGHT", "NATURAL", "DARK" }, true, 12.5f));
    for (int i = 0; i < 3; ++i) { head[i] = addLamp (std::make_unique<ui::Lamp> (ui::Lamp::Plate, "H" + String (i + 1))); head[i]->onInfo = info; }
    cRevType     = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "SPRING", "PLATE" }, true, 13.0f));

    // recessed buttons
    for (auto* b : { &oldSchoolB, &modernB, &styleB[0], &styleB[1], &styleB[2], &spkB[0], &spkB[1], &spkB[2], &spkB[3], &drumB[0], &drumB[1], &drumB[2], &drumB[3], &drumB[4], &drumB[5], &monB[0], &monB[1], &monB[2], &roomB[0], &roomB[1], &roomB[2] })
    {
        b->getProperties().set ("role", "recess"); b->setClickingTogglesState (false); addChildComponent (*b);
    }
    oldSchoolB.setButtonText ("OLD SCHOOL"); oldSchoolB.onClick = [this] { applyPianoVoicing (0); };
    modernB.setButtonText ("MODERN");        modernB.onClick = [this] { applyPianoVoicing (1); };
    oldSchoolB.setTooltip ("Upright piano on tape: warm, dark, a little wobble");
    modernB.setTooltip ("Clean, bright, hammer-forward digital stage piano");
    const char* styleNames[3] { "CHOP", "PICK", "RHYTHM" };
    for (int i = 0; i < 3; ++i) { styleB[i].setButtonText (styleNames[i]); styleB[i].onClick = [this, i] { selectPage (PGuitar, i, true); }; }
    const char* spkNames[4] { "PHONE", "LAPTOP", "14\" MACBOOK", "STUDIO MONITOR" };
    const char* spkHelp[4] { "Phone speaker: one tiny driver, nothing below about 400 Hz", "Laptop speakers: thin, nothing below about 200 Hz", "14-inch MacBook: small but fuller, rolls off below about 90 Hz", "Studio monitors: full range down to the sub" };
    for (int i = 0; i < 4; ++i)
    {
        spkB[i].setButtonText (spkNames[i]); spkB[i].setTooltip (spkHelp[i]);
        spkB[i].onClick = [this, i] { const int cur = juce::roundToInt (proc.parameters.getRawParameterValue (rp::id (rp::Bass, P::SpkMode))->load()); setParam (rp::Bass, P::SpkMode, cur == i + 1 ? 0.0f : (float) (i + 1)); };
    }
    const char* drumNames[6] { "KICK", "SNARE", "RIM", "HAT", "TOMS", "LOOP" };
    const char* drumHelp[6] { "Kick: adds weight, takes out the box, a little click", "Snare: body and crack", "Rim and sidestick: woody click, no lows", "Hats: removes the lows and tames the harsh middle", "Toms: round body around 120 Hz", "Full drum loop: no change in voicing" };
    for (int i = 0; i < 6; ++i) { drumB[i].setButtonText (drumNames[i]); drumB[i].setTooltip (drumHelp[i]); drumB[i].onClick = [this, i] { setParam (rp::Drums, P::DrumType, (float) i); }; }
    const char* monNames[3] { "NEARFIELD", "MIDFIELD", "MAIN" }, *roomNames[3] { "STUDIO", "CONTROL ROOM", "LIVE ROOM" };
    const char* monHelp[3] { "Small desk monitors close to you: little bass, forward mids", "Medium monitors a few metres away: balanced", "Big soffit-mounted mains: full bass, relaxed top end" };
    for (int i = 0; i < 3; ++i)
    {
        monB[i].setButtonText (monNames[i]); monB[i].setTooltip (String (monHelp[i]) + ". Click the lit one again to switch listening off.");
        monB[i].onClick = [this, i] { const int cur = juce::roundToInt (proc.parameters.getRawParameterValue (rp::id (rp::Master, P::CtlMon))->load()); setParam (rp::Master, P::CtlMon, cur == i + 1 ? 0.0f : (float) (i + 1)); };
        roomB[i].setButtonText (roomNames[i]); roomB[i].setTooltip ("Size of the room around the speakers");
        roomB[i].onClick = [this, i] { setParam (rp::Master, P::CtlRoom, (float) i); };
    }
    addChildComponent (ledSub);
    backB.setButtonText ("<  EFFECTS"); backB.getProperties().set ("role", "brass"); backB.onClick = [this] { selectPage (PFx, gMode, false); };
    addChildComponent (backB);
    const char* openNames[4] { "OPEN", "OPEN", "OPEN", "OPEN" };
    for (int i = 0; i < 4; ++i)
    {
        openB[i].setButtonText (openNames[i]); openB[i].getProperties().set ("role", "recess");
        openB[i].onClick = [this, i]
        {
            if (i == 1) setParam ((rp::Sec) sec, P::RevType, 0);
            if (i == 2) setParam ((rp::Sec) sec, P::RevType, 1);
            selectPage (i == 0 ? PEcho : (i == 1 ? PSpring : (i == 2 ? PPlate : PTape)), gMode, false);
        };
        addChildComponent (openB[i]);
    }

    // faders and meters
    for (auto* f : { &fIn, &fOut, &fCeil }) addChildComponent (*f);
    for (auto* l : { &ledIn, &ledOut, &ledGr }) addChildComponent (*l);
    for (auto* m : { &meterL, &meterR }) addAndMakeVisible (m);
    for (auto* m : { &meterIn, &meterGr }) addChildComponent (m);
    addAndMakeVisible (ladder);

    // preset strip
    addChildComponent (strip);
    strip.prev.onClick = [this] { stepPreset (-1); };
    strip.next.onClick = [this] { stepPreset (1); };
    strip.nameB.onClick = [this] { showLoadMenu(); };
    strip.saveB.onClick = [this] { saveUserPreset(); };

    // ---- state: section + sync
    secAtt = std::make_unique<ParameterAttachment> (*proc.parameters.getParameter ("activeSection"), [this] (float v)
    {
        const int s = jlimit (0, 5, roundToInt (v));
        if (s == sec) return;
        if (isInstrumentPage (page)) selectPage (pageOfSection (s), gMode, false);
        else { sec = s; rebindAll(); }
    });
    modeAtt = std::make_unique<ParameterAttachment> (*proc.parameters.getParameter ("guitarMode"), [this] (float v)
    {
        const int m = jlimit (0, 2, roundToInt (v));
        if (m != gMode) { gMode = m; rebindAll(); }
    });
    sec = jlimit (0, 5, roundToInt (proc.parameters.getRawParameterValue ("activeSection")->load()));
    gMode = jlimit (0, 2, roundToInt (proc.parameters.getRawParameterValue ("guitarMode")->load()));
    selectPage (pageOfSection (sec), gMode, false);

    addAndMakeVisible (roomFx); roomFx.toBack();
    addChildComponent (fade);
    animMode = loadAnimMode();
    roomFx.setActive (animMode != 2);
    updateRoomFx();
    setSize (kW, kH);
    // animation runs on the display's own refresh (60 fps); the slower timer is only a safety net in case a host never delivers refresh callbacks
    vblank = std::make_unique<VBlankAttachment> (this, [this] { onFrame (false); });
    startTimerHz (30);
    ready = true;
}

MainUI::~MainUI()
{
    stopTimer();
    secAtt.reset(); modeAtt.reset(); syncAtt.reset();
    hotspots.clear();
    knobs.clear(); lamps.clear(); choices.clear();
    for (auto& c : cardKnob) c.reset();
    setLookAndFeel (nullptr);
}

ui::Knob* MainUI::addKnob (P p, const String& caption, int diameter, int fmt, bool echoTime)
{
    auto kd = std::make_unique<KnobDef>();
    kd->k = std::make_unique<ui::Knob> (caption, diameter); kd->p = p; kd->fmt = fmt; kd->echoTime = echoTime;
    addChildComponent (*kd->k);
    auto* raw = kd->k.get();
    knobs.push_back (std::move (kd));
    return raw;
}
ui::Lamp* MainUI::addLamp (std::unique_ptr<ui::Lamp> l) { addChildComponent (*l); lamps.push_back (std::move (l)); return lamps.back().get(); }
ui::ChoiceLamps* MainUI::addChoice (std::unique_ptr<ui::ChoiceLamps> c) { addChildComponent (*c); choices.push_back (std::move (c)); return choices.back().get(); }
ui::Knob* MainUI::knobFor (P p)
{
    for (auto& kd : knobs) if (kd->p == p && ! kd->echoTime) return kd->k.get();
    return nullptr;
}

void MainUI::setInfo (const String& s) { if (s != infoText) { infoText = s; repaint (rInfo); } }

//------------------------------------------------------------------ pages
int MainUI::sectionOfPage (Page p)
{
    switch (p) { case PMaster: return rp::Master; case PDrums: return rp::Drums; case PGuitar: return rp::Guitar; case PBass: return rp::Bass; case PKeys: return rp::Keys; case PHorns: return rp::Horns; default: return -1; }
}
MainUI::Page MainUI::pageOfSection (int s)
{
    switch (s) { case rp::Drums: return PDrums; case rp::Guitar: return PGuitar; case rp::Bass: return PBass; case rp::Keys: return PKeys; case rp::Horns: return PHorns; default: return PMaster; }
}
String MainUI::pageTitle (Page p) { return kPageNames[(int) p]; }
String MainUI::pageSubtitle (Page p)
{
    switch (p)
    {
        case PHome: return String::fromUTF8 ("LIVE ROOM  \xc2\xb7  CLICK AN INSTRUMENT TO JUMP TO ITS TAB");
        case PMaster: return String::fromUTF8 ("CONTROL ROOM  \xc2\xb7  FINISH THE MIX");
        case PDrums: return "DRUM ROOM";
        case PGuitar: return String::fromUTF8 ("GUITAR ROOM  \xc2\xb7  CHOP / PICK / RHYTHM");
        case PBass: return "BASS ROOM";
        case PKeys: return String::fromUTF8 ("PIANO  \xc2\xb7  ORGAN  \xc2\xb7  KEYBOARD ROOM");
        case PHorns: return "BRASS & WOODWIND ROOM";
        case PControl: return "STUDIO REFERENCE LISTENING";
        case PFx: return "CLICK A MACHINE TO OPEN IT";
        case PTape: return "TAPE MACHINE";
        case PSpring: return "SPRING TANK";
        case PPlate: return "COPPER PLATE";
        case PEcho: return "DUB TAPE ECHO";
        default: return {};
    }
}

void MainUI::selectPage (Page pg, int guitarMode, bool writeParams)
{
    // picture of the page we are leaving, for the dissolve
    const juce::Rectangle<int> fadeArea (0, 196, kW, kH - 196);
    const bool doFade = ready && pg != page && animMode != 2 && (isShowing() || fadeForTest);
    juce::Image snap;
    if (doFade) snap = createComponentSnapshot (fadeArea, true, lastPs);

    const int s = sectionOfPage (pg);
    if (s >= 0) sec = s;
    page = pg; gMode = jlimit (0, 2, guitarMode);
    if (writeParams)
    {
        std::vector<std::pair<const char*, int>> w { { "guitarMode", gMode } };
        if (s >= 0) w.push_back ({ "activeSection", sec });
        for (auto [id, v] : w)
            if (auto* prm = proc.parameters.getParameter (id)) { prm->beginChangeGesture(); prm->setValueNotifyingHost (prm->convertTo0to1 ((float) v)); prm->endChangeGesture(); }
    }
    const int active = pg == PHome ? 0 : (pg == PControl ? 7 : (pg == PFx || isEffectPage (pg) ? 8 : (pg == PMaster ? 1 : (pg == PDrums ? 2 : (pg == PGuitar ? 3 : (pg == PBass ? 4 : (pg == PKeys ? 5 : 6)))))));
    for (int i = 0; i < 9; ++i) tab[i]->setActive (i == active);
    currentFactory = -1; currentUserFile = File();
    rebindAll();
    updateRoomFx();
    refreshUserPresets();
    currentName = proc.parameters.state.getProperty ("presetName_" + String ((int) page), "").toString();
    strip.setName (currentName);
    bgDirty = true; slowCount = 5;
    repaint();
    if (doFade)
    {
        ensureBg (lastPs);          // draw the new page's background now, so the dissolve starts smoothly afterwards
        fade.start (snap, fadeArea, 0.4f);
    }
}

void MainUI::updateRoomFx()
{
    auto& A = ui::Assets::get();
    const Image* im = nullptr;
    switch (page)
    {
        case PHome: im = &A.room[0]; break;
        case PDrums: im = &A.room[1]; break;
        case PGuitar: im = &A.room[2]; break;
        case PBass: im = &A.room[3]; break;
        case PKeys: im = &A.room[4]; break;
        case PHorns: im = &A.room[5]; break;
        case PMaster: im = &A.room[6]; break;
        case PControl: im = &A.room[7]; break;
        case PFx: im = &A.fxRoom; break;
        case PTape: im = &A.hero[0]; break;
        case PSpring: im = &A.hero[1]; break;
        case PPlate: im = &A.hero[2]; break;
        default: im = &A.hero[3]; break;
    }
    roomFx.setBounds (14, 200, 1252, page == PFx ? 440 : 358);
    roomFx.setFigures (A.fig);
    roomFx.setRoom (*im, (int) page);
}

void MainUI::showGuitarMenu()
{
    PopupMenu m;
    const char* n[3] { "Chop", "Pick", "Rhythm" };
    for (int i = 0; i < 3; ++i) m.addItem (i + 1, n[i], true, page == PGuitar && gMode == i);
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (tab[3].get()), [this, safe = SafePointer<MainUI> (this)] (int r) { if (safe != nullptr && r > 0) selectPage (PGuitar, r - 1, true); });
}

void MainUI::showEffectsMenu()
{
    PopupMenu m;
    m.addItem (1, "All effects", true, page == PFx);
    m.addSeparator();
    m.addItem (2, "Echo", true, page == PEcho); m.addItem (3, "Spring", true, page == PSpring);
    m.addItem (4, "Plate", true, page == PPlate); m.addItem (5, "Tape", true, page == PTape);
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (tab[8].get()), [this, safe = SafePointer<MainUI> (this)] (int r)
    {
        if (safe == nullptr || r <= 0) return;
        if (r == 3) setParam ((rp::Sec) sec, P::RevType, 0);
        if (r == 4) setParam ((rp::Sec) sec, P::RevType, 1);
        selectPage (r == 1 ? PFx : (r == 2 ? PEcho : (r == 3 ? PSpring : (r == 4 ? PPlate : PTape))), gMode, false);
    });
}

void MainUI::buildHotspots()
{
    hotspots.clear();
    struct H { const char* label; float x, y; std::function<void()> go; };
    std::vector<H> hs;
    const float roomX = 14.0f, roomY = 200.0f;
    if (page == PHome)
    {
        const float sx = 1252.0f / 1536.0f, sy = 358.0f / 438.0f;
        auto at = [&] (float px, float py) { return std::pair<float, float> { roomX + px * sx, roomY + (py - 190.0f) * sy }; };
        auto jump = [this] (Page pg) { return [this, pg] { selectPage (pg, gMode, true); }; };
        auto a = at (150, 470);  hs.push_back ({ "PIANO BANG", a.first, a.second, jump (PKeys) });
        a = at (700, 400);       hs.push_back ({ "DRUMS", a.first, a.second, jump (PDrums) });
        a = at (1035, 395);      hs.push_back ({ "BASS", a.first, a.second, jump (PBass) });
        a = at (1112, 535);      hs.push_back ({ "GUITAR", a.first, a.second, jump (PGuitar) });
        a = at (1420, 330);      hs.push_back ({ "CONTROL ROOM", a.first, a.second, jump (PControl) });
    }
    else if (page == PFx)
    {
        const float sx = 1252.0f / 1652.0f;
        auto at = [&] (float px, float py) { return std::pair<float, float> { roomX + px * sx, roomY + (py - 70.0f) * sx }; };
        auto open = [this] (Page pg) { return [this, pg] { if (pg == PSpring) setParam ((rp::Sec) sec, P::RevType, 0); if (pg == PPlate) setParam ((rp::Sec) sec, P::RevType, 1); selectPage (pg, gMode, false); }; };
        auto a = at (715, 170);  hs.push_back ({ "TAPE", a.first, a.second, open (PTape) });
        a = at (440, 400);       hs.push_back ({ "ECHO", a.first, a.second, open (PEcho) });
        a = at (670, 505);       hs.push_back ({ "PLATE", a.first, a.second, open (PPlate) });
        a = at (235, 560);       hs.push_back ({ "SPRING", a.first, a.second, open (PSpring) });
    }
    for (auto& h : hs)
    {
        auto c = std::make_unique<ui::Hotspot> (h.label);
        c->onClick = h.go;
        c->setBounds (Rectangle<int> (c->widthFor() + 14, 38).withCentre ({ roundToInt (h.x), roundToInt (h.y) }));
        addAndMakeVisible (*c);
        hotspots.push_back (std::move (c));
    }
}

//------------------------------------------------------------------ binding
void MainUI::updateEchoTimeBinding()
{
    const auto s = (rp::Sec) sec;
    for (auto& kd : knobs)
        if (kd->echoTime)
        {
            const bool sync = proc.parameters.getRawParameterValue (rp::id (s, P::Sync))->load() > 0.5f;
            syncOn = sync;
            if (sync) kd->k->bind (proc.parameters, rp::id (s, P::Division), 5, rp::defaultValue (s, P::Division), rp::choiceList (P::Division));
            else      kd->k->bind (proc.parameters, rp::id (s, P::EchoTime), 2, rp::defaultValue (s, P::EchoTime));
            kd->k->help = sync ? "Echo time as a note length, locked to the host tempo" : "Echo time in milliseconds (tempo sync is off)";
        }
}

void MainUI::rebindAll()
{
    const auto s = (rp::Sec) sec;
    auto idOf = [&] (P p) { return rp::isControl (p) ? rp::id (rp::Master, p) : (rp::exists (s, p) ? rp::id (s, p) : String()); };
    const auto src = rp::sourceInfo (s, gMode);
    const bool hasSrc = rp::avail (s, rp::MSrc), hasSteady = rp::avail (s, rp::MSteady), hasFilt = rp::avail (s, rp::MFilt),
               hasComp = rp::avail (s, rp::MComp);
    const auto shown = [&] (P p)
    {
        switch (p)
        {
            case P::Cutoff: case P::Resonance: return hasFilt;
            case P::CompThresh: case P::CompRatio: case P::CompMakeup: return hasComp;
            case P::SbAmount: case P::SbPunch: case P::SbWeight: return hasSteady;
            default: return true;
        }
    };
    int srcIdx = 0;
    for (auto& kd : knobs)
    {
        if (kd->echoTime) continue;
        P p = kd->p;
        if (p >= P::Src1 && p <= P::Src9)
        {
            const int i = srcIdx++;
            p = rp::sourceParam (s, gMode, i);
            if (! hasSrc) { kd->k->bind (proc.parameters, {}, 0, 0.0); continue; }
            kd->k->setCaption (String (src.names[i]));
            kd->k->help = srcHelp (s, gMode, i);
            kd->k->bind (proc.parameters, idOf (p), 0, rp::defaultValue (s, p));
            continue;
        }
        if (p == P::LimThresh || p == P::LimRelease || p == P::LimCeiling)
        {
            kd->k->bind (proc.parameters, rp::id (rp::Master, p), kd->fmt, rp::defaultValue (rp::Master, p));
            continue;
        }
        kd->k->bind (proc.parameters, shown (p) ? idOf (p) : String(), kd->fmt, rp::defaultValue (s, p));
    }
    updateEchoTimeBinding();

    auto bindL = [&] (ui::ParamWidget* w, P p) { w->bindParam (proc.parameters, idOf (p)); };
    bindL (lSat, P::InSat); bindL (lAnalog, P::Analog); bindL (lSync, P::Sync);
    lSoft->bindParam (proc.parameters, rp::id (rp::Master, P::LimSoft));
    lBypass->bindParam (proc.parameters, "bypass"); lThrow->bindParam (proc.parameters, "throw"); lPanic->bindParam (proc.parameters, "panic");
    bindL (pFilt, P::FiltOn); bindL (pComp, P::CompOn); bindL (pEq, P::EqOn); bindL (pTape, P::TapeOn); bindL (pEcho, P::EchoOn);
    bindL (pRev, P::RevOn); bindL (pSrc, P::SrcOn); bindL (pSteady, P::SbOn);
    pLim->bindParam (proc.parameters, rp::id (rp::Master, P::LimOn));
    bindL (cFilterMode, P::FilterMode); bindL (cFilterRoute, P::FilterRoute); bindL (cCompMode, P::CompMode); bindL (cTapeType, P::TapeType);
    bindL (cEchoType, P::EchoType); bindL (cRevType, P::RevType);
    bindL (cLock, P::LockOn); bindL (lMono, P::CtlMono); bindL (lPhones, P::CtlPhones);
    bindL (cTapeSpeed, P::TapeSpeed); bindL (cSpChar, P::SpChar); bindL (cSpLen, P::SpLen); bindL (cSpStereo, P::SpStereo); bindL (cPlTone, P::PlTone);
    bindL (head[0], P::Head1); bindL (head[1], P::Head2); bindL (head[2], P::Head3);

    fIn.bind (proc.parameters, idOf (P::InGain), rp::defaultValue (s, P::InGain), true);
    fOut.bind (proc.parameters, idOf (P::OutGain), rp::defaultValue (s, P::OutGain), true);
    fCeil.bind (proc.parameters, rp::id (rp::Master, P::LimCeiling), rp::defaultValue (rp::Master, P::LimCeiling), true);

    // effects hub cards
    cardKnob[0]->bind (proc.parameters, idOf (P::Mix), 0, rp::defaultValue (s, P::Mix));
    cardKnob[1]->bind (proc.parameters, idOf (P::RevMix), 0, rp::defaultValue (s, P::RevMix));
    cardKnob[2]->bind (proc.parameters, idOf (P::RevMix), 0, rp::defaultValue (s, P::RevMix));
    cardKnob[3]->bind (proc.parameters, idOf (P::TapeSat), 0, rp::defaultValue (s, P::TapeSat));
    cardLamp[0]->bindParam (proc.parameters, idOf (P::EchoOn)); cardLamp[1]->bindParam (proc.parameters, idOf (P::RevOn));
    cardLamp[2]->bindParam (proc.parameters, idOf (P::RevOn));  cardLamp[3]->bindParam (proc.parameters, idOf (P::TapeOn));

    // signal flow: only the modules this tab really has
    flowItems.clear();
    if (hasSrc)    flowItems.push_back ({ "SOURCE", P::SrcOn, false });
    if (hasSteady) flowItems.push_back ({ "STEADY", P::SbOn, false });
    if (hasFilt)   flowItems.push_back ({ "FILTER", P::FiltOn, false });
    if (hasComp)   flowItems.push_back ({ "COMP", P::CompOn, false });
    flowItems.push_back ({ "EQ", P::EqOn, false }); flowItems.push_back ({ "TAPE", P::TapeOn, false });
    flowItems.push_back ({ "ECHO", P::EchoOn, false }); flowItems.push_back ({ "REVERB", P::RevOn, false });
    flowItems.push_back ({ "LIMITER", P::LimOn, true });
    for (int i = 0; i < 9; ++i)
    {
        const bool used = i < (int) flowItems.size();
        flow[i]->bindParam (proc.parameters, used ? (flowItems[(size_t) i].master ? rp::id (rp::Master, flowItems[(size_t) i].param) : idOf (flowItems[(size_t) i].param)) : String());
    }

    syncAtt.reset();
    syncAtt = std::make_unique<ParameterAttachment> (*proc.parameters.getParameter (rp::id (s, P::Sync)), [this] (float v) { if ((v > 0.5f) != syncOn) updateEchoTimeBinding(); });
    layoutControls();
    buildHotspots();
}

//------------------------------------------------------------------ layout
namespace
{
    juce::Rectangle<int> bodyOf (const juce::Rectangle<int>& r) { return r.withTrimmedTop (34).reduced (10, 6); }
    juce::Rectangle<int> rightCol (const juce::Rectangle<int>& r, int w) { return juce::Rectangle<int> (r.getRight() - w - 12, r.getY() + 46, w, r.getHeight() - 58); }
}

int MainUI::naturalWidth (MK k, int dia) const
{
    const int kw = dia + 16;
    switch (k)
    {
        case MK::Src:     return 28 + 3 * kw + (page == PKeys ? 140 : (page == PGuitar ? 120 : 0));
        case MK::Steady:  return 28 + 3 * kw;
        case MK::Sub:     return 28 + 2 * kw + 70;
        case MK::Spk:     return 240;
        case MK::Filter:  return 28 + 2 * kw + 104;
        case MK::Comp:    return 28 + 3 * kw + (page == PDrums ? 0 : 92);
        case MK::DrumSrc: return 28 + 3 * kw;
        case MK::CtlMon: case MK::CtlRoom: return 200;
        case MK::CtlPos: return 290; case MK::CtlColour: return 170; case MK::CtlListen: return 300;
        case MK::Lock:    return 228;
        case MK::Eq:      return 28 + 4 * kw;
        case MK::Limiter: return 28 + 3 * kw + 80;
        case MK::TapeW:   return 28 + 2 * kw + 96;
        case MK::Reverb:  return 28 + 3 * kw + 96;
        case MK::TapeMain: return 380;   case MK::TapeMore: return 330; case MK::TapeTrans: return 250; case MK::TapeOut: return 150;
        case MK::RevMain:  return 520;   case MK::RevOut: return 160;
        case MK::SpChar: return 270; case MK::SpKnobs: return 324; case MK::SpTone: return 250; case MK::SpOut: return 190;
        case MK::PlMain: return 472; case MK::PlTone: return 180; case MK::PlOut: return 160;
        case MK::EchoMain: return 540;   case MK::EchoHeat: return 324;  case MK::EchoFilter: return 300;
        default:          return 200;
    }
}

void MainUI::buildModules()
{
    mods.clear();
    bandTop = page == PFx ? 648 : 566;
    const int x0 = 14, total = 1252, gap = 8, bottom = 846;
    auto add = [&] (MK k, int arg = 0) { mods.push_back ({ k, {}, arg }); };
    const auto s = (rp::Sec) sec;
    switch (page)
    {
        case PHome:
            add (MK::HomeIn); add (MK::HomeMeters); add (MK::HomeOut); add (MK::HomeLim); add (MK::HomeFlow);
            break;
        case PMaster:
            add (MK::Filter); add (MK::Comp); add (MK::Eq); add (MK::Limiter); break;
        case PDrums:  add (MK::DrumSrc); add (MK::Comp); add (MK::Eq); add (MK::Lock); add (MK::TapeW); break;
        case PGuitar: add (MK::Src); add (MK::Eq); add (MK::Reverb); add (MK::TapeW); break;
        case PBass:   add (MK::Steady); add (MK::Eq); add (MK::Sub); add (MK::Spk); add (MK::TapeW); break;
        case PKeys:   add (MK::Src); add (MK::Eq); add (MK::TapeW); break;
        case PHorns:  add (MK::Src); add (MK::Eq); add (MK::Reverb); add (MK::TapeW); break;
        case PControl: add (MK::CtlMon); add (MK::CtlRoom); add (MK::CtlPos); add (MK::CtlColour); add (MK::CtlListen); break;
        case PFx:     for (int i = 0; i < 4; ++i) add (MK::FxCard, i); break;
        case PTape:   add (MK::TapeMain); add (MK::TapeMore); add (MK::TapeTrans); add (MK::TapeOut); break;
        case PSpring: add (MK::SpChar); add (MK::SpKnobs); add (MK::SpTone); add (MK::SpOut); break;
        case PPlate:  add (MK::PlMain); add (MK::PlTone); add (MK::PlOut); break;
        case PEcho:   add (MK::EchoMain); add (MK::EchoHeat); if (rp::avail (s, rp::MFilt)) add (MK::EchoFilter); break;
        default: break;
    }
    const int n = (int) mods.size();
    std::vector<int> w ((size_t) n);
    if (page == PHome)
    {
        const int fixed[5] { 170, 380, 170, 220, 0 };
        int used = 0; for (int i = 0; i < 4; ++i) { w[(size_t) i] = fixed[i]; used += fixed[i]; }
        w[4] = total - gap * 4 - used;
    }
    else if (page == PFx) for (int i = 0; i < n; ++i) w[(size_t) i] = (total - gap * 3) / 4;
    else
    {
        moduleDia = 58;
        for (int dia : { 58, 46 })
        {
            int sum = gap * (n - 1); for (auto& m : mods) sum += naturalWidth (m.k, dia);
            moduleDia = dia; if (sum <= total) break;
        }
        int sum = gap * (n - 1); for (auto& m : mods) sum += naturalWidth (m.k, moduleDia);
        const int extra = juce::jmax (0, (total - sum) / n);
        for (int i = 0; i < n; ++i) w[(size_t) i] = naturalWidth (mods[(size_t) i].k, moduleDia) + extra;
    }
    int x = x0;
    for (int i = 0; i < n; ++i)
    {
        const int ww = (i + 1 == n) ? (x0 + total - x) : w[(size_t) i];
        mods[(size_t) i].r = { x, bandTop, ww, bottom - bandTop };
        x += ww + gap;
    }
}

static ui::Mat materialOf (int k, int arg)
{
    using namespace ui;
    switch (k)
    {
        case 0: return Mat::Brass;   // Src
        case 1: return Mat::Brass;   // Steady
        case 2: return Mat::Green;   // Filter
        case 3: return Mat::Red;     // Comp
        case 4: return Mat::Green;   // Eq
        case 5: return Mat::Steel;   // Limiter
        case 6: return Mat::Wood;    // TapeW
        case 7: return Mat::Steel;   // Reverb
        case 8: return Mat::Green;   // HomeIn
        case 9: return Mat::Dark;    // HomeMeters
        case 10: return Mat::Steel;  // HomeOut
        case 11: return Mat::Red;  // HomeLim
        case 12: return Mat::Green;  // HomeFlow
        case 13: return Mat::Wood;   // TapeMain
        case 14: return Mat::Steel;  // TapeTrans
        case 15: return Mat::Green;  // TapeOut
        case 16: return Mat::Steel;  // RevMain
        case 17: return Mat::Green;  // RevOut
        case 18: return Mat::Green;  // EchoMain
        case 19: return Mat::Brass;  // EchoHeat
        case 20: return Mat::Steel;  // EchoFilter
        case 35: return Mat::Green;  // CtlMon
        case 36: return Mat::Steel;  // CtlRoom
        case 37: return Mat::Steel;  // CtlPos
        case 38: return Mat::Red;    // CtlColour
        case 39: return Mat::Dark;   // CtlListen
        case 33: return Mat::Brass;  // DrumSrc
        case 34: return Mat::Steel;  // Lock
        case 31: return Mat::Green;  // Sub
        case 32: return Mat::Dark;   // Spk
        case 23: return Mat::Steel;  // TapeMore
        case 24: return Mat::Steel;  // SpChar
        case 25: return Mat::Brass;  // SpKnobs
        case 26: return Mat::Wood;   // SpTone
        case 27: return Mat::Green;  // SpOut
        case 28: return Mat::Steel;  // PlMain
        case 29: return Mat::Brass;  // PlTone
        case 30: return Mat::Green;  // PlOut
        case 21: { const Mat c[4] { Mat::Green, Mat::Steel, Mat::Red, Mat::Wood }; return c[arg & 3]; }
        default: return Mat::Dark;
    }
}

void MainUI::styleKnob (ui::Knob* k, ui::Mat m, int dia)
{
    k->setDiameter (dia); k->setInk (ui::inkFor (m), ui::litFor (m));
}
void MainUI::styleWidget (ui::ParamWidget* w, ui::Mat m) { w->ink = ui::inkFor (m); w->lit = ui::litFor (m); w->repaint(); }

void MainUI::power (ui::Lamp* l, const Rectangle<int>& panel)
{
    l->setBounds (panel.getRight() - 32, panel.getY() + 5, 20, 20); l->setVisible (true);
}

void MainUI::knobRow (std::initializer_list<P> ps, Rectangle<int> area, int dia, ui::Mat m, int)
{
    std::vector<ui::Knob*> ks;
    for (auto p : ps) if (auto* k = knobFor (p)) ks.push_back (k);
    const int n = (int) ks.size(); if (n == 0) return;
    const int cw = area.getWidth() / n;
    for (int i = 0; i < n; ++i)
    {
        styleKnob (ks[(size_t) i], m, dia);
        ks[(size_t) i]->setBounds (Rectangle<int> (ks[(size_t) i]->preferredWidth(), ks[(size_t) i]->preferredHeight()).withCentre ({ area.getX() + cw * i + cw / 2, area.getCentreY() }));
        ks[(size_t) i]->setVisible (true);
    }
}

void MainUI::hideAllControls()
{
    for (auto& kd : knobs) kd->k->setVisible (false);
    for (auto& l : lamps) l->setVisible (false);
    for (auto& c : choices) c->setVisible (false);
    for (auto& k : cardKnob) k->setVisible (false);
    for (auto* c : std::initializer_list<Component*> { &oldSchoolB, &modernB, &styleB[0], &styleB[1], &styleB[2], &backB, &openB[0], &openB[1], &openB[2], &openB[3],
                                                         &fIn, &fOut, &fCeil, &ledIn, &ledOut, &ledGr, &ledSub, &monB[0], &monB[1], &monB[2], &roomB[0], &roomB[1], &roomB[2], &drumB[0], &drumB[1], &drumB[2], &drumB[3], &drumB[4], &drumB[5], &spkB[0], &spkB[1], &spkB[2], &spkB[3], &meterIn, &meterGr, &strip })
        c->setVisible (false);
}

void MainUI::layoutControls()
{
    buildModules();
    hideAllControls();
    const auto s = (rp::Sec) sec;
    const int dia = moduleDia;

    for (auto& m : mods)
    {
        const auto& r = m.r;
        const auto mat = materialOf ((int) m.k, m.arg);
        const auto body = bodyOf (r);
        switch (m.k)
        {
            case MK::Src:
            {
                const int rw = page == PKeys ? 140 : (page == PGuitar ? 120 : 0);
                power (pSrc, r); styleWidget (pSrc, mat);
                std::vector<ui::Knob*> ks = srcKnobs;
                const int cw = body.withTrimmedRight (rw).getWidth() / 3;
                for (int i = 0; i < 3; ++i)
                {
                    styleKnob (ks[(size_t) i], mat, dia);
                    ks[(size_t) i]->setBounds (Rectangle<int> (ks[(size_t) i]->preferredWidth(), ks[(size_t) i]->preferredHeight()).withCentre ({ body.getX() + cw * i + cw / 2, body.getCentreY() }));
                    ks[(size_t) i]->setVisible (rp::avail (s, rp::MSrc));
                }
                if (page == PKeys)
                {
                    auto c = rightCol (r, rw - 8);
                    oldSchoolB.setBounds (c.getX(), c.getY() + 20, c.getWidth(), 34); modernB.setBounds (c.getX(), c.getY() + 62, c.getWidth(), 34);
                    oldSchoolB.setVisible (true); modernB.setVisible (true);
                }
                if (page == PGuitar)
                {
                    auto c = rightCol (r, rw - 8);
                    for (int i = 0; i < 3; ++i) { styleB[i].setBounds (c.getX(), c.getY() + 18 + i * 42, c.getWidth(), 34); styleB[i].setVisible (true); styleB[i].setToggleState (i == gMode, dontSendNotification); }
                }
                break;
            }
            case MK::Steady:
            {
                power (pSteady, r); styleWidget (pSteady, mat);
                knobRow ({ P::SbAmount, P::SbPunch, P::SbWeight }, body.withTrimmedBottom (46), dia, mat);
                break;
            }
            case MK::Filter:
            {
                power (pFilt, r); styleWidget (pFilt, mat);
                knobFor (P::Cutoff)->setCaption ("CUTOFF");
                knobRow ({ P::Cutoff, P::Resonance }, body.withTrimmedRight (104), dia, mat);
                auto c = rightCol (r, 96);
                cFilterMode->setBounds (c.getX(), c.getY() + 16, c.getWidth(), 66); cFilterMode->setVisible (true); styleWidget (cFilterMode, mat);
                cFilterRoute->setBounds (c.getX(), c.getY() + 112, c.getWidth(), 44); cFilterRoute->setVisible (true); styleWidget (cFilterRoute, mat);
                break;
            }
            case MK::Comp:
            {
                power (pComp, r); styleWidget (pComp, mat);
                if (page == PDrums)
                {
                    knobRow ({ P::CompThresh, P::CompRatio, P::CompMakeup }, body.withTrimmedBottom (84), dia, mat);
                    cCompMode->setBounds (r.getX() + 36, r.getBottom() - 82, r.getWidth() - 72, 66);
                }
                else
                {
                    knobRow ({ P::CompThresh, P::CompRatio, P::CompMakeup }, body.withTrimmedRight (92), dia, mat);
                    auto c = rightCol (r, 84);
                    cCompMode->setBounds (c.getX(), c.getY() + 16, c.getWidth(), 66);
                }
                cCompMode->setVisible (true); styleWidget (cCompMode, mat);
                break;
            }
            case MK::Eq:
            {
                power (pEq, r); styleWidget (pEq, mat);
                knobRow ({ P::EqLow, P::EqMid, P::EqHigh, P::EqPres }, body, dia, mat);
                break;
            }
            case MK::Limiter:
            {
                power (pLim, r); styleWidget (pLim, mat);
                std::vector<ui::Knob*> ks = limKnobs;
                const int cw = body.withTrimmedRight (80).getWidth() / 3;
                for (int i = 0; i < 3; ++i)
                {
                    styleKnob (ks[(size_t) i], mat, dia);
                    ks[(size_t) i]->setBounds (Rectangle<int> (ks[(size_t) i]->preferredWidth(), ks[(size_t) i]->preferredHeight()).withCentre ({ body.getX() + cw * i + cw / 2, body.getCentreY() }));
                    ks[(size_t) i]->setVisible (true);
                }
                auto c = rightCol (r, 72);
                lSoft->setBounds (c.getX(), c.getY() + 40, c.getWidth(), 32); lSoft->setVisible (true); styleWidget (lSoft, mat);
                break;
            }
            case MK::TapeW:
            {
                power (pTape, r); styleWidget (pTape, mat);
                knobFor (P::TapeSat)->setCaption ("DRIVE");
                knobRow ({ P::TapeSat, P::TapeTone }, body.withTrimmedRight (96), dia, mat);
                auto c = rightCol (r, 90);
                cTapeType->setBounds (c.getX(), c.getY() + 16, c.getWidth(), 66); cTapeType->setVisible (true); styleWidget (cTapeType, mat);
                break;
            }
            case MK::Reverb:
            {
                power (pRev, r); styleWidget (pRev, mat);
                knobRow ({ P::RevPre, P::RevDecay, P::RevMix }, body.withTrimmedRight (96), dia, mat);
                auto c = rightCol (r, 90);
                cRevType->setBounds (c.getX(), c.getY() + 16, c.getWidth(), 44); cRevType->setVisible (true); styleWidget (cRevType, mat);
                break;
            }
            case MK::HomeIn:
            {
                fIn.setBounds (r.getX() + 16, r.getY() + 40, 90, r.getHeight() - 52); fIn.setInk (ui::inkFor (mat), ui::litFor (mat)); fIn.setVisible (true);
                ledIn.setBounds (r.getX() + 118, r.getY() + 40 + 16 + 8, 22, r.getHeight() - 52 - 32 - 16); ledIn.setVisible (true);
                break;
            }
            case MK::HomeOut:
            {
                fOut.setBounds (r.getX() + 16, r.getY() + 40, 90, r.getHeight() - 52); fOut.setInk (ui::inkFor (mat), ui::litFor (mat)); fOut.setVisible (true);
                ledOut.setBounds (r.getX() + 118, r.getY() + 40 + 16 + 8, 22, r.getHeight() - 52 - 32 - 16); ledOut.setVisible (true);
                break;
            }
            case MK::HomeLim:
            {
                fCeil.setBounds (r.getX() + 12, r.getY() + 40, 90, r.getHeight() - 52); fCeil.setInk (ui::inkFor (mat), ui::litFor (mat)); fCeil.setVisible (true);
                ledGr.setBounds (r.getX() + 104, r.getY() + 40 + 16 + 8, 20, r.getHeight() - 52 - 32 - 16); ledGr.setVisible (true);
                auto* t = knobFor (P::LimThresh); auto* rl = knobFor (P::LimRelease);
                for (auto* k : { t, rl }) styleKnob (k, mat, 46);
                t->setBounds (Rectangle<int> (t->preferredWidth(), t->preferredHeight()).withCentre ({ r.getRight() - 52, r.getY() + 36 + 52 })); t->setVisible (true);
                rl->setBounds (Rectangle<int> (rl->preferredWidth(), rl->preferredHeight()).withCentre ({ r.getRight() - 52, r.getY() + 36 + 52 + 98 })); rl->setVisible (true);
                break;
            }
            case MK::HomeMeters:
            {
                meterIn.setBounds (r.getX() + 16, r.getY() + 44, 168, 105); meterGr.setBounds (r.getRight() - 184, r.getY() + 44, 168, 105);
                meterIn.setVisible (true); meterGr.setVisible (true);
                lSat->setBounds (r.getX() + 24, r.getY() + 186, 160, 20); lAnalog->setBounds (r.getX() + 200, r.getY() + 186, 160, 20);
                lSoft->setBounds (r.getX() + 24, r.getY() + 218, 132, 30);
                for (auto* l : { lSat, lAnalog, lSoft }) { l->setVisible (true); styleWidget (l, mat); }
                break;
            }
            case MK::HomeFlow:
            {
                const int n = (int) flowItems.size();
                for (int i = 0; i < 9; ++i)
                {
                    const int col = i % 3, row = i / 3;
                    flow[i]->setBounds (r.getX() + 20 + col * 84, r.getY() + 46 + row * 30, 22, 22);
                    flow[i]->setVisible (i < n); styleWidget (flow[i], mat);
                }
                const int bw = (r.getWidth() - 32 - 12) / 3, by = r.getY() + 176;
                lBypass->setBounds (r.getX() + 16, by, bw, 34); lThrow->setBounds (r.getX() + 16 + bw + 6, by, bw, 34); lPanic->setBounds (r.getX() + 16 + 2 * (bw + 6), by, bw, 34);
                for (auto* l : { lBypass, lThrow, lPanic }) { l->setVisible (true); styleWidget (l, mat); }
                break;
            }
            case MK::FxCard:
            {
                auto* k = cardKnob[m.arg].get();
                k->setDiameter (58); k->setInk (ui::inkFor (mat), ui::litFor (mat));
                const char* caps[4] { "MIX", "MIX", "MIX", "DRIVE" };
                k->setCaption (caps[m.arg]);
                k->setBounds (Rectangle<int> (k->preferredWidth(), k->preferredHeight()).withCentre ({ r.getX() + 64, r.getY() + 36 + 56 }));
                k->setVisible (true);
                cardLamp[m.arg]->setBounds (r.getX() + 132, r.getY() + 48, 20, 20); cardLamp[m.arg]->setVisible (true);
                openB[m.arg].setBounds (r.getX() + 128, r.getY() + 92, r.getWidth() - 148, 34); openB[m.arg].setVisible (true);
                break;
            }
            case MK::TapeMain:
            {
                power (pTape, r); styleWidget (pTape, mat);
                auto* sat = knobFor (P::TapeSat); sat->setCaption ("AMOUNT");
                const int rw = 132;
                auto kb = body.withTrimmedRight (rw);
                styleKnob (sat, mat, 80);
                sat->setBounds (Rectangle<int> (sat->preferredWidth(), sat->preferredHeight()).withCentre ({ kb.getX() + kb.getWidth() / 3, kb.getCentreY() })); sat->setVisible (true);
                auto* tn = knobFor (P::TapeTone); styleKnob (tn, mat, 58);
                tn->setBounds (Rectangle<int> (tn->preferredWidth(), tn->preferredHeight()).withCentre ({ kb.getX() + kb.getWidth() * 2 / 3 + 20, kb.getCentreY() })); tn->setVisible (true);
                (void) 0;
                auto c = rightCol (r, rw - 16);
                cTapeType->setBounds (c.getX(), c.getY() + 16, c.getWidth(), 84); cTapeType->setVisible (true); styleWidget (cTapeType, mat);
                break;
            }
            case MK::DrumSrc:
            {
                power (pSrc, r); styleWidget (pSrc, mat);
                const int bw = (r.getWidth() - 28 - 12) / 3;
                for (int i = 0; i < 6; ++i)
                {
                    drumB[i].setBounds (r.getX() + 14 + (i % 3) * (bw + 6), r.getY() + 44 + (i / 3) * 38, bw, 32);
                    drumB[i].setVisible (true);
                }
                std::vector<ui::Knob*> ks = srcKnobs;
                const auto area = body.withTrimmedTop (82);
                const int cw = area.getWidth() / 3;
                for (int i = 0; i < 3; ++i)
                {
                    styleKnob (ks[(size_t) i], mat, dia);
                    ks[(size_t) i]->setBounds (Rectangle<int> (ks[(size_t) i]->preferredWidth(), ks[(size_t) i]->preferredHeight()).withCentre ({ area.getX() + cw * i + cw / 2, area.getCentreY() }));
                    ks[(size_t) i]->setVisible (true);
                }
                break;
            }
            case MK::Lock:
            {
                cLock->setBounds (r.getX() + 24, r.getY() + 46, r.getWidth() - 48, 70); cLock->setVisible (true); styleWidget (cLock, mat);
                auto* k = knobFor (P::LockAmt); styleKnob (k, mat, 46);
                k->setBounds (Rectangle<int> (k->preferredWidth(), k->preferredHeight()).withCentre ({ r.getX() + 62, r.getY() + 172 })); k->setVisible (true);
                break;
            }
            case MK::Sub:
            {
                knobRow ({ P::SubLevel, P::SubHarm }, body.withTrimmedRight (74).withTrimmedBottom (4), dia, mat);
                ledSub.setBounds (r.getRight() - 52, r.getY() + 64, 34, r.getHeight() - 110); ledSub.setVisible (true);
                break;
            }
            case MK::Spk:
            {
                const int bw = (r.getWidth() - 28 - 8) / 2;
                for (int i = 0; i < 4; ++i)
                {
                    spkB[i].setBounds (r.getX() + 14 + (i % 2) * (bw + 8), r.getY() + 46 + (i / 2) * 48, bw, 40);
                    spkB[i].setVisible (true);
                }
                auto* k = knobFor (P::SpkLevel); styleKnob (k, mat, 46);
                k->setBounds (Rectangle<int> (k->preferredWidth(), k->preferredHeight()).withCentre ({ r.getX() + 62, r.getY() + 190 })); k->setVisible (true);
                break;
            }
            case MK::TapeMore:
            {
                knobRow ({ P::TapeStab, P::TapeMix }, body.withTrimmedRight (122), 58, mat);
                auto c = rightCol (r, 110);
                cTapeSpeed->setBounds (c.getX(), c.getY() + 16, c.getWidth(), 84); cTapeSpeed->setVisible (true); styleWidget (cTapeSpeed, mat);
                break;
            }
            case MK::SpChar:
            {
                power (pRev, r); styleWidget (pRev, mat);
                const int lw = 112;
                auto c = rightCol (r, r.getWidth() - 24);
                cSpChar->setBounds (c.getX(), c.getY() + 16, c.getWidth() - lw - 8, c.getHeight() - 18); cSpChar->setVisible (true); styleWidget (cSpChar, mat);
                cSpLen->setBounds (c.getRight() - lw, c.getY() + 16, lw, 84); cSpLen->setVisible (true); styleWidget (cSpLen, mat);
                break;
            }
            case MK::SpKnobs:
                knobRow ({ P::SpDrive, P::RevPre, P::RevDecay, P::RevMix }, body, 58, mat); break;
            case MK::SpTone:
                knobRow ({ P::SpFb, P::SpTreble, P::SpBass }, body, 58, mat); break;
            case MK::SpOut:
            {
                auto* k = knobFor (P::OutGain); k->setCaption ("OUT"); styleKnob (k, mat, 68);
                k->setBounds (Rectangle<int> (k->preferredWidth(), k->preferredHeight()).withCentre ({ body.getCentreX(), body.getY() + 66 })); k->setVisible (true);
                cSpStereo->setBounds (r.getX() + 24, r.getBottom() - 74, r.getWidth() - 48, 56); cSpStereo->setVisible (true); styleWidget (cSpStereo, mat);
                break;
            }
            case MK::PlMain:
            {
                power (pRev, r); styleWidget (pRev, mat);
                knobRow ({ P::RevPre, P::RevDecay, P::PlDamp, P::PlLowCut, P::PlWidth, P::RevMix }, body, 58, mat);
                break;
            }
            case MK::PlTone:
            {
                auto c = rightCol (r, r.getWidth() - 24);
                cPlTone->setBounds (c.getX(), c.getY() + 16, c.getWidth(), 84); cPlTone->setVisible (true); styleWidget (cPlTone, mat);
                break;
            }
            case MK::PlOut:
            {
                auto* k = knobFor (P::OutGain); k->setCaption ("OUT"); styleKnob (k, mat, 68);
                k->setBounds (Rectangle<int> (k->preferredWidth(), k->preferredHeight()).withCentre (body.getCentre())); k->setVisible (true);
                break;
            }
            case MK::TapeTrans:
                knobRow ({ P::Wow, P::Flutter, P::Wear }, body, 58, mat); break;
            case MK::TapeOut:
            case MK::RevOut:
            {
                auto* k = knobFor (P::OutGain); k->setCaption ("OUT"); styleKnob (k, mat, 68);
                k->setBounds (Rectangle<int> (k->preferredWidth(), k->preferredHeight()).withCentre (body.getCentre())); k->setVisible (true);
                break;
            }
            case MK::RevMain:
            {
                power (pRev, r); styleWidget (pRev, mat);
                knobRow ({ P::RevPre, P::RevDecay, P::RevMix }, body, 68, mat);
                break;
            }
            case MK::EchoMain:
            {
                power (pEcho, r); styleWidget (pEcho, mat);
                echoTimeKnob->setCaption ("TIME"); styleKnob (echoTimeKnob, mat, 68);
                knobFor (P::Feedback)->setCaption ("FEEDBACK");
                const int rw = 130;
                auto kb = body.withTrimmedRight (rw);
                const int cw = kb.getWidth() / 3;
                std::vector<ui::Knob*> ks { echoTimeKnob, knobFor (P::Feedback), knobFor (P::Mix) };
                for (int i = 0; i < 3; ++i)
                {
                    styleKnob (ks[(size_t) i], mat, 68);
                    ks[(size_t) i]->setBounds (Rectangle<int> (ks[(size_t) i]->preferredWidth(), ks[(size_t) i]->preferredHeight()).withCentre ({ kb.getX() + cw * i + cw / 2, kb.getCentreY() }));
                    ks[(size_t) i]->setVisible (true);
                }
                auto c = rightCol (r, rw - 16);
                cEchoType->setBounds (c.getX(), c.getY() + 16, c.getWidth(), 66); cEchoType->setVisible (true); styleWidget (cEchoType, mat);
                lSync->setBounds (c.getX(), c.getY() + 110, c.getWidth(), 32); lSync->setVisible (true); styleWidget (lSync, mat);
                break;
            }
            case MK::EchoHeat:
            {
                knobRow ({ P::Heat, P::Width, P::Damping, P::EchoRev }, body.withTrimmedBottom (46), 58, mat);
                const int hw = (r.getWidth() - 48 - 16) / 3;
                for (int i = 0; i < 3; ++i)
                {
                    head[i]->setBounds (r.getX() + 24 + i * (hw + 8), r.getBottom() - 46, hw, 32);
                    head[i]->setVisible (true); styleWidget (head[i], mat);
                }
                break;
            }
            case MK::EchoFilter:
            {
                power (pFilt, r); styleWidget (pFilt, mat);
                knobFor (P::Cutoff)->setCaption ("TONE");
                knobRow ({ P::Cutoff, P::Resonance }, body.withTrimmedRight (104), 58, mat);
                auto c = rightCol (r, 96);
                cFilterMode->setBounds (c.getX(), c.getY() + 16, c.getWidth(), 66); cFilterMode->setVisible (true); styleWidget (cFilterMode, mat);
                cFilterRoute->setBounds (c.getX(), c.getY() + 112, c.getWidth(), 44); cFilterRoute->setVisible (true); styleWidget (cFilterRoute, mat);
                break;
            }
            case MK::CtlMon:
                for (int i = 0; i < 3; ++i) { monB[i].setBounds (r.getX() + 18, r.getY() + 46 + i * 44, r.getWidth() - 36, 36); monB[i].setVisible (true); }
                break;
            case MK::CtlRoom:
                for (int i = 0; i < 3; ++i) { roomB[i].setBounds (r.getX() + 18, r.getY() + 46 + i * 44, r.getWidth() - 36, 36); roomB[i].setVisible (true); }
                break;
            case MK::CtlPos:
                knobRow ({ P::CtlWidth, P::CtlDist, P::CtlToe }, body, dia, mat); break;
            case MK::CtlColour:
                knobRow ({ P::CtlColour }, body, 68, mat); break;
            case MK::CtlListen:
            {
                auto* k = knobFor (P::CtlLevel); styleKnob (k, mat, 68);
                k->setBounds (Rectangle<int> (k->preferredWidth(), k->preferredHeight()).withCentre ({ r.getX() + 80, r.getY() + 130 })); k->setVisible (true);
                lMono->setBounds (r.getX() + 160, r.getY() + 76, r.getWidth() - 180, 34); lMono->setVisible (true); styleWidget (lMono, mat);
                lPhones->setBounds (r.getX() + 160, r.getY() + 120, r.getWidth() - 180, 34); lPhones->setVisible (true); styleWidget (lPhones, mat);
                break;
            }
            case MK::Soon: break;
        }
    }

    // room-level widgets
    const bool hasBank = bankFor (page).list != nullptr;
    strip.setBounds (14 + 1252 - 12 - 330, 200 + 358 - 12 - 34, 330, 34);
    strip.setVisible (hasBank);
    backB.setBounds (26, 210, 118, 30); backB.setVisible (isEffectPage (page));
    for (auto& h : hotspots) h->toFront (false);
    if (fade.running()) fade.toFront (false);
    bgDirty = true;
    repaint();
}

//------------------------------------------------------------------ painting (cached; the live parts are child components)
void MainUI::resized()
{
    rHeader = { 14, 9, 576, 141 };
    rMeterPanel = { 600, 12, 666, 138 };
    rRail = { 14, 152, 1252, 44 };
    rRoom = { 14, 200, 1252, 358 };
    rInfo = { 14, 856, 1252, 28 };
    for (int i = 0; i < 9; ++i) tab[i]->setBounds (14 + i * 140, 154, 132, 40);
    meterL.setBounds (rMeterPanel.getX() + 22, rMeterPanel.getY() + 10, 190, 117);
    meterR.setBounds (rMeterPanel.getX() + 22 + 190 + 56, rMeterPanel.getY() + 10, 190, 117);
    ladder.setBounds (rMeterPanel.getX() + 22 + 190 + 8, rMeterPanel.getY() + 14, 40, 96);
    cog.setBounds (rMeterPanel.getRight() - 120, rMeterPanel.getY() + 22, 100, 100);
    layoutControls();
}

void MainUI::paintModuleExtras (Graphics& g, const Mod& m)
{
    using ui::printLabel;
    const auto& r = m.r;
    const auto mat = materialOf ((int) m.k, m.arg);
    const auto col = mat == ui::Mat::Brass ? Colour (0xff3b2808) : ui::col::creamDim;
    auto lab = [&] (const String& t, Rectangle<float> rc, Justification j = Justification::centred) { printLabel (g, t, rc, mat, j, 11.0f, true); };
    switch (m.k)
    {
        case MK::Src:
            if (page == PKeys) { auto c = rightCol (r, 132); lab ("VOICING", { (float) c.getX(), (float) c.getY(), (float) c.getWidth(), 16 }); lab ("EQ + TAPE PRESET", { (float) c.getX() - 6, (float) c.getY() + 102, (float) c.getWidth() + 12, 14 }); }
            if (page == PGuitar) { auto c = rightCol (r, 112); lab ("STYLE", { (float) c.getX(), (float) c.getY(), (float) c.getWidth(), 16 }); }
            break;
        case MK::CtlMon:
            lab ("PICK ONE TO SWITCH LISTENING ON", { (float) r.getX() + 8, (float) r.getBottom() - 30, (float) r.getWidth() - 16, 14 }); break;
        case MK::CtlRoom:
            lab ("AMBIENCE AROUND THE SPEAKERS", { (float) r.getX() + 8, (float) r.getBottom() - 30, (float) r.getWidth() - 16, 14 }); break;
        case MK::CtlListen:
            lab ("MONITOR ONLY", { (float) r.getX() + 12, (float) r.getBottom() - 46, (float) r.getWidth() - 24, 14 });
            lab ("OFF WHEN YOU EXPORT", { (float) r.getX() + 12, (float) r.getBottom() - 30, (float) r.getWidth() - 24, 14 }); break;
        case MK::Lock:
        {
            lab ("MAKES ROOM FOR", { (float) r.getX() + 104, (float) r.getY() + 146, (float) r.getWidth() - 112, 14 });
            lab ("THE BASS BELOW", { (float) r.getX() + 104, (float) r.getY() + 162, (float) r.getWidth() - 112, 14 });
            lab ("100 Hz", { (float) r.getX() + 104, (float) r.getY() + 180, (float) r.getWidth() - 112, 16 });
            const int mode = juce::roundToInt (proc.parameters.getRawParameterValue (rp::id (rp::Drums, P::LockOn))->load());
            const int st = proc.getSidechainState();
            String t = mode != 2 ? "SIDECHAIN: DUCKS WHEN THE BASS PLAYS" : (st == 1 ? "LISTENING: SIDECHAIN INPUT" : (st == 2 ? "LISTENING: BASS TAB" : "NO BASS FOUND: ROUTE ONE IN"));
            lab (t, { (float) r.getX() + 6, (float) r.getBottom() - 30, (float) r.getWidth() - 12, 14 });
            break;
        }
        case MK::Sub:
            lab ("30-50", { (float) r.getRight() - 58, (float) r.getY() + 46, 46.0f, 14 });
            lab ("SUB BAND", { (float) r.getRight() - 62, (float) r.getBottom() - 40, 54.0f, 14 }); break;
        case MK::Spk:
            lab ("MONITOR ONLY", { (float) r.getX() + 110, (float) r.getY() + 172, (float) r.getWidth() - 120, 14 });
            lab ("OFF WHEN YOU EXPORT", { (float) r.getX() + 110, (float) r.getY() + 190, (float) r.getWidth() - 120, 14 }); break;
        case MK::Steady:
        {
            if (page == PBass)
            { printLabel (g, "EVENS OUT NOTE LEVELS", { (float) r.getX() + 10, (float) r.getBottom() - 46, (float) r.getWidth() - 20, 16 }, mat, Justification::centred, 11.0f, true);
              printLabel (g, "LOW END STAYS FULL  /  NO LATENCY", { (float) r.getX() + 10, (float) r.getBottom() - 28, (float) r.getWidth() - 20, 16 }, mat, Justification::centred, 11.0f, true); break; }
            const char* lines[4] { "EVENS OUT NOTE LEVELS", "LOW END STAYS FULL", "ATTACK COMES THROUGH", "NO LATENCY" };
            auto c = rightCol (r, 178);
            for (int i = 0; i < 4; ++i) printLabel (g, lines[i], { (float) c.getX(), (float) c.getY() + 20.0f + 24.0f * i, (float) c.getWidth(), 16 }, mat, Justification::centredLeft, 11.5f, i != 0);
            break;
        }
        case MK::Filter: case MK::EchoFilter:
        { auto c = rightCol (r, 96); lab ("TYPE", { (float) c.getX(), (float) c.getY(), (float) c.getWidth(), 14 }); lab ("ROUTE", { (float) c.getX(), (float) c.getY() + 94, (float) c.getWidth(), 14 }); break; }
        case MK::Comp:
        { if (page == PDrums) { lab ("MODE", { (float) r.getX() + 36, (float) r.getBottom() - 98, (float) r.getWidth() - 72, 14 }); break; } auto c = rightCol (r, 84); lab ("MODE", { (float) c.getX(), (float) c.getY(), (float) c.getWidth(), 14 }); break; }
        case MK::TapeW: case MK::TapeMain:
        { auto c = rightCol (r, m.k == MK::TapeMain ? 116 : 90); lab ("TYPE", { (float) c.getX(), (float) c.getY(), (float) c.getWidth(), 14 }); break; }
        case MK::Reverb:
        { auto c = rightCol (r, 90); lab ("TYPE", { (float) c.getX(), (float) c.getY(), (float) c.getWidth(), 14 }); break; }
        case MK::EchoMain:
        { auto c = rightCol (r, 114); lab ("TYPE", { (float) c.getX(), (float) c.getY(), (float) c.getWidth(), 14 }); break; }
        case MK::EchoHeat:
            lab ("PLAYBACK HEADS", { (float) r.getX() + 16, (float) r.getBottom() - 64, (float) r.getWidth() - 32, 14 }); break;
        case MK::TapeMore:
        { auto c = rightCol (r, 110); lab ("SPEED", { (float) c.getX(), (float) c.getY(), (float) c.getWidth(), 14 }); break; }
        case MK::SpChar:
        { auto c = rightCol (r, r.getWidth() - 24); lab ("CHARACTER", { (float) c.getX(), (float) c.getY(), (float) c.getWidth() - 120, 14 }); lab ("LENGTH", { (float) c.getRight() - 112, (float) c.getY(), 112.0f, 14 }); break; }
        case MK::SpOut:
            lab ("SPREAD", { (float) r.getX() + 24, (float) r.getBottom() - 92, (float) r.getWidth() - 48, 14 }); break;
        case MK::PlTone:
        { auto c = rightCol (r, r.getWidth() - 24); lab ("TONE", { (float) c.getX(), (float) c.getY(), (float) c.getWidth(), 14 }); break; }
        case MK::Limiter:
        { auto c = rightCol (r, 72); lab ("OUTPUT", { (float) c.getX(), (float) c.getY() + 14, (float) c.getWidth(), 14 }); break; }
        case MK::HomeMeters:
        {
            lab ("INPUT LEVEL", { (float) r.getX() + 16, (float) r.getY() + 150, 168, 16 });
            lab ("GAIN REDUCTION", { (float) r.getRight() - 184, (float) r.getY() + 150, 168, 16 });
            lab ("THE MASTER LIMITER ALWAYS GUARDS THE OUTPUT", { (float) r.getX() + 16, (float) r.getBottom() - 24, (float) r.getWidth() - 32, 14 });
            break;
        }
        case MK::HomeFlow:
        {
            for (int i = 0; i < (int) flowItems.size(); ++i)
            {
                const int c = i % 3, rw = i / 3;
                printLabel (g, flowItems[(size_t) i].label, { (float) r.getX() + 20 + c * 84 + 26, (float) r.getY() + 46 + rw * 30, 56, 22 }, mat, Justification::centredLeft, 12.0f);
            }
            lab ("LAMPS SWITCH EACH STAGE ON AND OFF", { (float) r.getX() + 10, (float) r.getY() + 148, (float) r.getWidth() - 20, 14 });
            lab ("ONE PLUGIN PER TRACK", { (float) r.getX() + 10, (float) r.getBottom() - 24, (float) r.getWidth() - 20, 14 });
            break;
        }
        case MK::HomeLim:
        { lab ("GR", { (float) r.getX() + 98, (float) r.getY() + 34, 32, 14 }); break; }
        case MK::FxCard:
        {
            printLabel (g, "ON", { (float) r.getX() + 156, (float) r.getY() + 48, 40, 20 }, mat, Justification::centredLeft, 12.0f);
            const char* sub[4] { "DUB TAPE ECHO", "SPRING TANK", "COPPER PLATE", "TAPE MACHINE  A / B / C" };
            lab (sub[m.arg & 3], { (float) r.getX() + 12, (float) r.getBottom() - 36, (float) r.getWidth() - 24, 16 });
            break;
        }
        case MK::Soon:
        {
            auto b = r.toFloat().reduced (30.0f);
            g.setColour (ui::col::amber); g.setFont (ui::font (22.0f, 3, 0.14f));
            g.drawText (String::fromUTF8 ("LISTENING CHECK  \xc2\xb7  ARRIVES IN A LATER UPDATE"), b.removeFromTop (b.getHeight() * 0.55f), Justification::centredBottom, false);
            g.setColour (ui::col::creamDim); g.setFont (ui::font (14.0f, 1, 0.04f));
            g.drawFittedText ("Nearfield / Midfield / Main monitors, room size, width and distance. It will only colour what you hear while mixing and switches itself off when you export.", b.toNearestInt(), Justification::centredTop, 3);
            break;
        }
        default: break;
    }
    (void) col;
}

// The part of the picture that is the same on every page (wood frame, plate, logo, meters panel, tab rail). Drawn once per screen scale.
void MainUI::paintChrome (Graphics& g)
{
    auto& A = ui::Assets::get();
    // wood frame + dark back plate (no vignette: the logo area stays at full brightness)
    g.setTiledImageFill (A.wood, 0, 0, 1.0f); g.fillAll();
    auto plate = Rectangle<float> (7, 7, kW - 14.0f, kH - 14.0f);
    g.setColour (Colours::black); g.fillRoundedRectangle (plate.expanded (2), 6);
    g.setTiledImageFill (A.dark, 0, 0, 1.0f); g.fillRoundedRectangle (plate, 5);
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.05f), 0, plate.getY(), Colours::black.withAlpha (0.25f), 0, plate.getBottom(), false)); g.fillRoundedRectangle (plate, 5);
    g.setColour (ui::col::brass.withAlpha (0.55f)); g.drawRoundedRectangle (plate.reduced (1.0f), 5, 1.4f);
    for (auto c : { Point<float> (kW * 0.5f, 3.5f), Point<float> (kW * 0.5f, kH - 3.5f) }) ui::drawScrew (g, c, 7.0f, 1);

    // header: logo plate (art drawn at full strength) and meter panel
    g.setImageResamplingQuality (Graphics::highResamplingQuality);
    ui::paintPanel (g, rHeader.toFloat(), A.green2, 2, {}, 0, true);
    g.setColour (Colours::white);
    g.drawImage (A.header, Rectangle<float> (16, 12, 568, 138), RectanglePlacement::centred);
    ui::paintPanel (g, rMeterPanel.toFloat(), A.dark, 3, {}, 0, true);
    ui::engraved (g, "OUTPUT  LEFT", { (float) meterL.getX(), (float) meterL.getBottom() - 1.0f, 190, 14 }, ui::font (12.5f, 2, 0.12f), ui::col::cream);
    ui::engraved (g, "OUTPUT  RIGHT", { (float) meterR.getX(), (float) meterR.getBottom() - 1.0f, 190, 14 }, ui::font (12.5f, 2, 0.12f), ui::col::cream);
    ui::engraved (g, "L", { (float) ladder.getX() + 3.0f, (float) ladder.getBottom() + 2.0f, 16, 14 }, ui::font (12.5f, 2, 0.1f), ui::col::brass);
    ui::engraved (g, "R", { (float) ladder.getX() + 21.0f, (float) ladder.getBottom() + 2.0f, 16, 14 }, ui::font (12.5f, 2, 0.1f), ui::col::brass);

    // tab rail
    {
        auto r = rRail.toFloat();
        g.setGradientFill (ColourGradient (Colour (0xff040302), 0, r.getY(), Colour (0xff1b130b), 0, r.getBottom(), false)); g.fillRoundedRectangle (r, 6.0f);
        g.setColour (Colours::black); g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.6f);
        g.setColour (ui::col::brass.withAlpha (0.35f)); g.drawRoundedRectangle (r.reduced (2.5f), 5.0f, 1.0f);
    }

}

// The part that changes from page to page: room picture, module panels and labels, info bar frame.
void MainUI::paintPage (Graphics& g)
{
    auto& A = ui::Assets::get();
    // room picture
    {
        const bool hub = page == PFx;
        const Image* im = nullptr;
        switch (page)
        {
            case PHome: im = &A.room[0]; break;
            case PDrums: im = &A.room[1]; break;
            case PGuitar: im = &A.room[2]; break;
            case PBass: im = &A.room[3]; break;
            case PKeys: im = &A.room[4]; break;
            case PHorns: im = &A.room[5]; break;
            case PMaster: im = &A.room[6]; break;
            case PControl: im = &A.room[7]; break;
            case PFx: im = &A.fxRoom; break;
            case PTape: im = &A.hero[0]; break;
            case PSpring: im = &A.hero[1]; break;
            case PPlate: im = &A.hero[2]; break;
            default: im = &A.hero[3]; break;
        }
        auto rr = Rectangle<float> (14, 200, 1252, hub ? 440.0f : 358.0f);
        Path clip; clip.addRoundedRectangle (rr, 6.0f);
        ui::softShadow (g, rr, 6.0f, 9.0f, { 0, 4 }, 0.8f);
        g.saveState(); g.reduceClipRegion (clip);
        g.setColour (Colours::white);
        g.setImageResamplingQuality (ui::pixelScaleOf (g) * rr.getWidth() >= (float) im->getWidth() * 0.9f ? Graphics::mediumResamplingQuality : Graphics::highResamplingQuality);
        g.drawImage (*im, rr, RectanglePlacement::stretchToFit);
        g.restoreState();
        g.setColour (Colours::black.withAlpha (0.95f)); g.drawRoundedRectangle (rr.reduced (0.7f), 6.0f, 1.6f);
        g.setColour (Colour (0xffdeb260).withAlpha (0.7f)); g.drawRoundedRectangle (rr.reduced (2.2f), 5.0f, 1.3f);
        g.setColour (Colours::black.withAlpha (0.45f)); g.drawRoundedRectangle (rr.reduced (3.6f), 4.0f, 1.0f);
        {
            // title chip bottom left (hero pages: below the back button)
            const auto big = pageTitle (page); const auto small = pageSubtitle (page);
            auto fb = ui::font (26.0f, 3, 0.10f), fs = ui::font (12.0f, 2, 0.12f);
            const float w = fb.getStringWidthFloat (big) + fs.getStringWidthFloat (small) + 44.0f;
            auto chip = Rectangle<float> (rr.getX() + 12, rr.getBottom() - 52, w, 40.0f);
            g.setColour (Colours::black.withAlpha (0.62f)); g.fillRoundedRectangle (chip.expanded (4), 9.0f);
            g.setFont (fb); g.setColour (ui::col::brass.brighter (0.3f)); g.drawText (big, chip.withTrimmedLeft (10), Justification::centredLeft, false);
            g.setFont (fs); g.setColour (ui::col::cream);
            g.drawText (small, chip.withTrimmedLeft (10 + fb.getStringWidthFloat (big) + 12), Justification::centredLeft, false);
        }
    }

    // modules
    for (size_t i = 0; i < mods.size(); ++i)
    {
        const auto& m = mods[i];
        static const char* titles[] { "", "STEADYBASS", "FILTER", "COMPRESSOR", "TONE EQ", "LIMITER", "TAPE WARMTH", "REVERB",
                                      "INPUT", "", "OUTPUT", "LIMITER", "SIGNAL FLOW", "TAPE MACHINE", "TRANSPORT", "OUTPUT", "REVERB", "OUTPUT", "ECHO", "TAPE HEAT", "ECHO FILTER", "", "LISTEN", "MORE", "SPRING TYPE", "SPRING TUNING", "SPRING TONE", "OUTPUT", "COPPER PLATE", "PLATE TONE", "OUTPUT", "SUB  30 - 50 Hz", "SPEAKER CHECK", "DRUM SOURCE", "KICK + BASS LOCK", "MONITORS", "ROOM", "SPEAKER POSITION", "COLOUR", "LISTENING" };
        static const char* icons[] { "note", "bars", "wave", "gauge", "sliders", "gauge", "reel", "plate",
                                     "", "", "", "gauge", "", "reel", "reel", "", "plate", "", "ring", "reel", "wave", "", "speaker", "reel", "spring", "spring", "spring", "", "plate", "plate", "", "bars", "speaker", "drum", "bars", "speaker", "plate", "sliders", "gauge", "speaker" };
        String title = titles[(int) m.k], icon = icons[(int) m.k];
        if (m.k == MK::Src) { title = String (rp::sourceInfo ((rp::Sec) sec, gMode).title); icon = page == PDrums ? "drum" : (page == PKeys ? "note" : (page == PGuitar ? "wave" : "bars")); }
        if (m.k == MK::Eq && page == PKeys) title = "TONE EQ  /  PIANO VOICING";
        if (m.k == MK::Reverb) title = page == PGuitar ? "SPRING REVERB" : "PLATE REVERB";
        if (m.k == MK::RevMain) title = page == PSpring ? "SPRING TANK" : "COPPER PLATE";
        if (m.k == MK::FxCard) { const char* t[4] { "ECHO", "SPRING", "PLATE", "TAPE" }; const char* ic[4] { "ring", "spring", "plate", "reel" }; title = t[m.arg & 3]; icon = ic[m.arg & 3]; }
        ui::paintModule (g, m.r.toFloat(), materialOf ((int) m.k, m.arg), title, icon, (int) i + 2 + (int) page);
        paintModuleExtras (g, m);
    }

    // info bar: the panel and the fixed brand text are part of the picture; the changing text is drawn live
    {
        auto r = rInfo.toFloat();
        ui::paintPanel (g, r, A.dark, 20, {}, 0, false);
        g.setColour (Colour (0xff050403)); g.fillRoundedRectangle (r.reduced (6, 4), 3);
        auto in = r.reduced (14, 4);
        ui::engraved (g, "RIDDIM LAB  |  ECLECTIC PRODZ", in.removeFromLeft (230), ui::font (12.5f, 2, 0.12f), ui::col::brass, Justification::centredLeft);
    }
}

void MainUI::paintInfoLive (Graphics& g)
{
    auto r = rInfo.toFloat();
    auto in = r.reduced (14, 4);
    in.removeFromLeft (230);
    // clear the part that changes (the picture underneath already has the dark inset)
    g.setColour (Colour (0xff050403));
    g.fillRect (in.getX(), r.getY() + 4.0f, r.getRight() - 9.0f - in.getX(), r.getHeight() - 8.0f);
    auto right = in.removeFromRight (300);
    String rt = pageTitle (page) + "   LATENCY " + String (proc.getLatencySamples()) + " smp";
    if (showPerf) rt = String (perfFps, 0) + " FPS  WORST " + String (perfWorstShown, 0) + " MS   " + rt;
    ui::engraved (g, rt, right.withLeft (right.getX() - (showPerf ? 150.0f : 0.0f)), ui::font (12.5f, 2, 0.1f), ui::col::creamDim, Justification::centredRight);
    if (showPerf) in.removeFromRight (150.0f);
    ui::engraved (g, infoText.isEmpty() ? String ("Hover a control to see what it does.  Double-click a knob to reset it.") : infoText, in, ui::font (14.0f, 1, 0.03f),
                  infoText.isEmpty() ? ui::col::creamDim : ui::col::amber, Justification::centredLeft);
}

void MainUI::ensureBg (float ps)
{
    const int key = bgKeyNow();
    if (! bgDirty && bgCache.isValid() && std::abs (ps - bgScale) < 0.01f && key == bgKey) return;
    const int pw = jmax (1, roundToInt (kW * ps)), ph = jmax (1, roundToInt (kH * ps));
    const auto scaleTo = [] (const Image& im) { return AffineTransform::scale ((float) im.getWidth() / (float) kW, (float) im.getHeight() / (float) kH); };
    if (! chromeCache.isValid() || chromeCache.getWidth() != pw || chromeCache.getHeight() != ph)
    {
        chromeCache = Image (Image::RGB, pw, ph, false);
        Graphics cg (chromeCache);
        cg.addTransform (scaleTo (chromeCache));
        paintChrome (cg);
    }
    if (! bgCache.isValid() || bgCache.getWidth() != pw || bgCache.getHeight() != ph) bgCache = Image (Image::RGB, pw, ph, false);
    bgScale = ps; bgKey = key; bgDirty = false;
    Graphics ig (bgCache);
    {
        Graphics::ScopedSaveState ss (ig);
        ig.setImageResamplingQuality (Graphics::lowResamplingQuality);
        ig.drawImageAt (chromeCache, 0, 0);          // 1:1 copy in device pixels
    }
    ig.addTransform (scaleTo (bgCache));
    paintPage (ig);
}

void MainUI::paint (Graphics& g)
{
    const float ps = ui::pixelScaleOf (g);
    lastPs = ps;
    ensureBg (ps);
    {
        // copy the finished picture 1:1 onto the screen
        Graphics::ScopedSaveState ss (g);
        g.addTransform (AffineTransform::scale ((float) kW / (float) bgCache.getWidth(), (float) kH / (float) bgCache.getHeight()));
        g.setImageResamplingQuality (Graphics::lowResamplingQuality);
        g.setOpacity (1.0f);
        g.drawImageAt (bgCache, 0, 0);
    }
    if (g.getClipBounds().intersects (rInfo)) paintInfoLive (g);
}

//------------------------------------------------------------------ Piano Bang voicings (EQ + tape + source in one click)
void MainUI::applyPianoVoicing (int which)
{
    for (auto& [p, v] : rp::pianoVoicing (which)) setParam (rp::Keys, p, v);
    setInfo (which == 0 ? "OLD SCHOOL: upright piano on tape. Warm lows, forward mids, soft top, a little wobble."
                        : "MODERN: clean, bright, hammer-forward stage piano. Scooped mids, air on top, no tape colour.");
}

bool MainUI::pianoVoicingMatches (int which) const
{
    for (auto& [p, v] : rp::pianoVoicing (which))
        if (std::abs (proc.parameters.getRawParameterValue (rp::id (rp::Keys, p))->load() - v) > 0.011f * jmax (1.0f, std::abs (v))) return false;
    return true;
}

//------------------------------------------------------------------ meters / animation
void MainUI::timerCallback() { onFrame (true); }

void MainUI::onFrame (bool fromTimer)
{
    const double now = Time::getMillisecondCounterHiRes();
    const double since = now - lastFrameMs;
    if (fromTimer ? since < 70.0 : since < 12.5) return;     // timer only steps in if refresh callbacks stopped; displays faster than 60 Hz are capped at about 60
    lastFrameMs = now;
    if (! fromTimer && since < 250.0)
    {
        ++perfFrames; perfWorst = jmax (perfWorst, since);
        if (now - perfStart >= 1000.0)
        {
            perfFps = (float) (perfFrames * 1000.0 / (now - perfStart)); perfWorstShown = (float) perfWorst;
            perfFrames = 0; perfWorst = 0.0; perfStart = now;
            if (showPerf) repaint (rInfo);
        }
    }
    frameTick (jlimit (0.001f, 0.1f, (float) since * 0.001f));
}

void MainUI::frameTick (float dt)
{
    auto vu = [] (float peak) { return jlimit (0.0f, 1.04f, ui::vuPos (20.0f * std::log10 (jmax (peak, 1.0e-5f)) + 12.0f)); };
    // meter needles glide the same way at any frame rate (the old constants were tuned for 30 frames a second)
    auto glide = [dt] (float& n, float t) { const float k = t > n ? 0.35f : 0.10f; n += (t - n) * (1.0f - std::pow (1.0f - k, dt * 30.0f)); return n; };
    auto dbOf = [] (float p) { return 20.0f * std::log10 (jmax (p, 1.0e-5f)); };
    const float oL = proc.getOutputPeak (0), oR = proc.getOutputPeak (1);
    const float inP = jmax (proc.getInputPeak (0), proc.getInputPeak (1));
    meterL.setPosition (glide (needle[0], vu (oL)));
    meterR.setPosition (glide (needle[1], vu (oR)));
    ladder.setLevels (oL, oR, dt);
    if (fade.running()) fade.step (dt);
    if (animMode != 2)
    {
        // while a knob is being dragged the picture is redrawn less often so the drag stays smooth
        const bool dragging = ModifierKeys::currentModifiers.isAnyMouseButtonDown();
        const int every = animMode == 1 ? 4 : (dragging ? 3 : 1);
        roomFx.tick (jlimit (0.0f, 1.0f, jmax (oL, oR)), dt, (++animCount % every) == 0);
    }
    if (page == PHome)
    {
        meterIn.setPosition (glide (needle[2], vu (inP)));
        const float gr = proc.getCompGainReductionDb() + proc.getLimiterGainReductionDb();
        needle[3] += (ui::grPos (gr) - needle[3]) * (1.0f - std::pow (1.0f - (ui::grPos (gr) > needle[3] ? 0.35f : 0.12f), dt * 30.0f));
        meterGr.setPosition (needle[3]);
        ledIn.setDb (dbOf (inP)); ledOut.setDb (dbOf (jmax (oL, oR))); ledGr.setDb (proc.getLimiterGainReductionDb());
    }
    if (++slowCount < 6) return;       // the rest only follows host automation and presets, so ten checks a second are plenty
    slowCount = 0;
    if (page == PBass)
    {
        ledSub.setDb (dbOf (proc.getSubLevel()) + 6.0f);
        const int mode = juce::roundToInt (proc.parameters.getRawParameterValue (rp::id (rp::Bass, P::SpkMode))->load());
        for (int i = 0; i < 4; ++i) spkB[i].setToggleState (mode == i + 1, dontSendNotification);
    }
    if (page == PControl)
    {
        const int mm = juce::roundToInt (proc.parameters.getRawParameterValue (rp::id (rp::Master, P::CtlMon))->load());
        const int rr = juce::roundToInt (proc.parameters.getRawParameterValue (rp::id (rp::Master, P::CtlRoom))->load());
        for (int i = 0; i < 3; ++i) { monB[i].setToggleState (mm == i + 1, dontSendNotification); roomB[i].setToggleState (rr == i, dontSendNotification); }
    }
    if (page == PDrums)
    {
        const int key = proc.getSidechainState() * 10 + juce::roundToInt (proc.parameters.getRawParameterValue (rp::id (rp::Drums, P::LockOn))->load());
        if (key != lastScKey) { lastScKey = key; bgDirty = true; for (auto& m : mods) if (m.k == MK::Lock) repaint (m.r); }
        const int t = juce::roundToInt (proc.parameters.getRawParameterValue (rp::id (rp::Drums, P::DrumType))->load());
        for (int i = 0; i < 6; ++i) drumB[i].setToggleState (t == i, dontSendNotification);
    }
    if (page == PKeys) { oldSchoolB.setToggleState (pianoVoicingMatches (0), dontSendNotification); modernB.setToggleState (pianoVoicingMatches (1), dontSendNotification); }
}

//------------------------------------------------------------------ presets
static File animFile() { return File::getSpecialLocation (File::userApplicationDataDirectory).getChildFile ("Eclectic Prodz").getChildFile ("Riddim Lab").getChildFile ("animation.txt"); }
int MainUI::loadAnimMode() { auto f = animFile(); return f.existsAsFile() ? jlimit (0, 2, f.loadFileAsString().trim().getIntValue()) : 0; }
void MainUI::saveAnimMode (int m) { auto f = animFile(); f.getParentDirectory().createDirectory(); f.replaceWithText (String (m)); }

File MainUI::presetFolder()
{
    return File::getSpecialLocation (File::userApplicationDataDirectory).getChildFile ("Eclectic Prodz").getChildFile ("Riddim Lab").getChildFile ("Presets");
}

void MainUI::setParam (rp::Sec s, P p, float v)
{
    if (! rp::exists (s, p)) return;
    if (auto* prm = proc.parameters.getParameter (rp::id (s, p)))
    {
        prm->beginChangeGesture(); prm->setValueNotifyingHost (prm->convertTo0to1 (v)); prm->endChangeGesture();
    }
}

void MainUI::setCurrentPreset (const String& name, int factoryIndex, const File& file)
{
    currentName = name; currentFactory = factoryIndex; currentUserFile = file;
    proc.parameters.state.setProperty ("presetName_" + String ((int) page), name, nullptr);
    strip.setName (name);
}

void MainUI::refreshUserPresets()
{
    userPresets.clear();
    const auto& b = bankFor (page);
    if (b.list == nullptr) return;
    auto dir = presetFolder();
    if (! dir.isDirectory()) return;
    auto files = dir.findChildFiles (File::findFiles, false, "*.rlpreset");
    std::sort (files.begin(), files.end(), [] (const File& a, const File& c) { return a.getFileName().compareIgnoreCase (c.getFileName()) < 0; });
    for (auto& f : files)
        if (auto xml = parseXML (f))
            if (xml->getStringAttribute ("scope", "tab") == b.scope) userPresets.push_back (f);
}

void MainUI::applyFactoryPreset (int index)
{
    const auto& b = bankFor (page);
    if (b.list == nullptr || index < 0 || index >= (int) b.list->size()) return;
    const auto s = (rp::Sec) sec;
    if (b.savable.empty())      // whole tab
    {
        for (int i = 0; i < rp::NumP; ++i)
        {
            const auto p = (P) i;
            if (! rp::exists (s, p) || rp::spec (p).scope == 1 || p == P::InGain || p == P::OutGain || rp::monitorOnly (p)) continue;
            setParam (s, p, rp::defaultValue (s, p));
        }
    }
    else for (auto p : b.savable) setParam (s, p, rp::defaultValue (s, p));
    for (auto& [p, v] : (*b.list)[(size_t) index].v) setParam (s, p, v);
    setCurrentPreset ((*b.list)[(size_t) index].name, index, {});
}

void MainUI::applyUserPreset (const File& f)
{
    if (auto xml = parseXML (f))
    {
        const auto s = (rp::Sec) sec;
        for (auto* e : xml->getChildWithTagNameIterator ("P"))
            for (int i = 0; i < rp::NumP; ++i)
                if (e->getStringAttribute ("id") == rp::spec ((P) i).id && rp::exists (s, (P) i) && rp::spec ((P) i).scope != 1)
                    setParam (s, (P) i, (float) e->getDoubleAttribute ("value"));
        setCurrentPreset (xml->getStringAttribute ("name", f.getFileNameWithoutExtension()), -1, f);
    }
}

void MainUI::stepPreset (int delta)
{
    const auto& b = bankFor (page);
    if (b.list == nullptr) return;
    refreshUserPresets();
    const int nf = (int) b.list->size(), total = nf + (int) userPresets.size();
    int cur = currentFactory >= 0 ? currentFactory : (currentUserFile.existsAsFile() ? nf + (int) (std::find (userPresets.begin(), userPresets.end(), currentUserFile) - userPresets.begin()) : -1);
    cur = cur < 0 ? (delta > 0 ? 0 : total - 1) : (cur + delta + total) % total;
    if (cur < nf) applyFactoryPreset (cur); else applyUserPreset (userPresets[(size_t) (cur - nf)]);
}

void MainUI::showLoadMenu()
{
    const auto& b = bankFor (page);
    if (b.list == nullptr) return;
    refreshUserPresets();
    PopupMenu m;
    for (int i = 0; i < (int) b.list->size(); ++i) m.addItem (1 + i, String (i + 1).paddedLeft ('0', 2) + "   " + (*b.list)[(size_t) i].name + "   -   " + (*b.list)[(size_t) i].sub, true, i == currentFactory);
    if (! userPresets.empty())
    {
        m.addSeparator(); m.addSectionHeader ("MY PRESETS");
        for (size_t i = 0; i < userPresets.size(); ++i) m.addItem (101 + (int) i, userPresets[i].getFileNameWithoutExtension(), true, userPresets[i] == currentUserFile);
    }
    m.addSeparator();
    m.addItem (900, "Delete \"" + currentName + "\"", currentUserFile.existsAsFile());
    m.addItem (901, "Show my presets folder");
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&strip.nameB), [this, safe = SafePointer<MainUI> (this)] (int r)
    {
        if (safe == nullptr || r <= 0) return;
        const auto& bk = bankFor (page);
        if (r == 900) { deleteCurrentPreset(); return; }
        if (r == 901) { auto d = presetFolder(); d.createDirectory(); d.revealToUser(); return; }
        if (bk.list != nullptr && r <= (int) bk.list->size()) applyFactoryPreset (r - 1);
        else if (r >= 101 && r - 101 < (int) userPresets.size()) applyUserPreset (userPresets[(size_t) (r - 101)]);
    });
}

void MainUI::saveUserPreset()
{
    const auto& b = bankFor (page);
    if (b.list == nullptr) return;
    auto* w = new AlertWindow ("Save preset", String ("Name for this ") + pageTitle (page).toLowerCase() + " preset.", MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", currentName.isNotEmpty() && currentFactory < 0 ? currentName : String(), "Preset name");
    w->addButton ("Save", 1, KeyPress (KeyPress::returnKey));
    w->addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
    w->enterModalState (true, ModalCallbackFunction::create ([this, safe = SafePointer<MainUI> (this), w] (int result)
    {
        std::unique_ptr<AlertWindow> owner (w);
        if (safe == nullptr || result != 1) return;
        const auto name = w->getTextEditorContents ("name").trim();
        if (name.isEmpty()) return;
        const auto& bk = bankFor (page);
        auto dir = presetFolder(); dir.createDirectory();
        auto file = dir.getChildFile (File::createLegalFileName (bk.scope + " - " + name) + ".rlpreset");
        XmlElement xml ("RiddimPreset");
        xml.setAttribute ("name", name); xml.setAttribute ("scope", bk.scope); xml.setAttribute ("tab", rp::kSecName[sec]); xml.setAttribute ("version", 3);
        const auto s = (rp::Sec) sec;
        auto put = [&] (P p) { auto* e = xml.createNewChildElement ("P"); e->setAttribute ("id", rp::spec (p).id); e->setAttribute ("value", (double) proc.parameters.getRawParameterValue (rp::id (s, p))->load()); };
        if (bk.savable.empty())
        {
            for (int i = 0; i < rp::NumP; ++i)
            {
                const auto p = (P) i;
                if (! rp::exists (s, p) || rp::spec (p).scope == 1 || p == P::InGain || p == P::OutGain || rp::monitorOnly (p)) continue;
                put (p);
            }
        }
        else for (auto p : bk.savable) if (rp::exists (s, p)) put (p);
        if (xml.writeTo (file)) { refreshUserPresets(); setCurrentPreset (name, -1, file); }
        else AlertWindow::showMessageBoxAsync (MessageBoxIconType::WarningIcon, "Could not save", "The preset file could not be written to " + file.getFullPathName());
    }), false);
}

void MainUI::deleteCurrentPreset()
{
    if (! currentUserFile.existsAsFile()) return;
    const auto file = currentUserFile;
    AlertWindow::showOkCancelBox (MessageBoxIconType::QuestionIcon, "Delete preset", "Delete \"" + file.getFileNameWithoutExtension() + "\"?", "Delete", "Cancel", this,
        ModalCallbackFunction::create ([this, safe = SafePointer<MainUI> (this), file] (int r)
        {
            if (safe == nullptr || r != 1) return;
            file.deleteFile(); refreshUserPresets(); setCurrentPreset ({}, -1, {});
        }));
}

//------------------------------------------------------------------ settings menu: whole-plugin presets and options
void MainUI::showSettingsMenu()
{
    PopupMenu m;
    m.addSectionHeader ("WHOLE PLUGIN");
    m.addItem (1, "Save everything as a preset...");
    m.addItem (2, "Load a whole-plugin preset...");
    m.addSeparator();
    m.addItem (3, "Reset this tab to its starting settings", isInstrumentPage (page));
    m.addItem (4, "Show my presets folder");
    m.addSeparator();
    m.addSectionHeader ("ANIMATION");
    m.addItem (10, "Full  (smoothest)", true, animMode == 0);
    m.addItem (11, "Reduced  (lighter on slower computers)", true, animMode == 1);
    m.addItem (12, "Off  (still pictures, no page fade)", true, animMode == 2);
    m.addItem (13, "Show frame timer in the bottom bar", true, showPerf);
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&cog), [this, safe = SafePointer<MainUI> (this)] (int r)
    {
        if (safe == nullptr || r <= 0) return;
        if (r == 1) saveWholePlugin();
        else if (r == 2) loadWholePlugin();
        else if (r == 3)
        {
            const auto s = (rp::Sec) sec;
            for (int i = 0; i < rp::NumP; ++i) { const auto p = (P) i; if (rp::exists (s, p) && rp::spec (p).scope != 1) setParam (s, p, rp::defaultValue (s, p)); }
            setCurrentPreset ({}, -1, {});
        }
        else if (r == 4) { auto d = presetFolder(); d.createDirectory(); d.revealToUser(); }
        else if (r >= 10 && r <= 12) { animMode = r - 10; saveAnimMode (animMode); roomFx.setActive (animMode != 2); }
        else if (r == 13) { showPerf = ! showPerf; repaint (rInfo); }
    });
}

void MainUI::saveWholePlugin()
{
    auto dir = presetFolder().getParentDirectory().getChildFile ("Whole Plugin"); dir.createDirectory();
    chooser = std::make_shared<FileChooser> ("Save the whole plugin as a preset", dir.getChildFile ("My Riddim Lab.rlplugin"), "*.rlplugin");
    chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting,
        [this, safe = SafePointer<MainUI> (this)] (const FileChooser& fc)
        {
            if (safe == nullptr) return;
            auto f = fc.getResult(); if (f == File()) return;
            f = f.withFileExtension ("rlplugin");
            if (auto xml = proc.parameters.copyState().createXml()) xml->writeTo (f);
        });
}

void MainUI::loadWholePlugin()
{
    auto dir = presetFolder().getParentDirectory().getChildFile ("Whole Plugin"); dir.createDirectory();
    chooser = std::make_shared<FileChooser> ("Load a whole-plugin preset", dir, "*.rlplugin");
    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
        [this, safe = SafePointer<MainUI> (this)] (const FileChooser& fc)
        {
            if (safe == nullptr) return;
            auto f = fc.getResult(); if (! f.existsAsFile()) return;
            if (auto xml = parseXML (f))
                if (xml->hasTagName (proc.parameters.state.getType()))
                {
                    proc.parameters.replaceState (ValueTree::fromXml (*xml));
                    sec = jlimit (0, 5, roundToInt (proc.parameters.getRawParameterValue ("activeSection")->load()));
                    gMode = jlimit (0, 2, roundToInt (proc.parameters.getRawParameterValue ("guitarMode")->load()));
                    selectPage (isInstrumentPage (page) ? pageOfSection (sec) : page, gMode, false);
                }
        });
}

//==============================================================================
RiddimLabAudioProcessorEditor::RiddimLabAudioProcessorEditor (RiddimLabAudioProcessor& p)
    : AudioProcessorEditor (&p), main (p)
{
    addAndMakeVisible (main);
    setResizeLimits (640, 450, 2560, 1800);
    getConstrainer()->setFixedAspectRatio ((double) MainUI::kW / (double) MainUI::kH);
    setResizable (true, true);
    float scale = 1.0f;
    if (auto* d = Desktop::getInstance().getDisplays().getPrimaryDisplay())
        scale = jlimit (0.55f, 1.0f, jmin ((d->userArea.getHeight() - 90) / (float) MainUI::kH, (d->userArea.getWidth() - 40) / (float) MainUI::kW));
    setSize (roundToInt (MainUI::kW * scale), roundToInt (MainUI::kH * scale));
}

void RiddimLabAudioProcessorEditor::resized()
{
    const float s = (float) getWidth() / (float) MainUI::kW;
    main.setTransform (AffineTransform::scale (s));
    main.setBounds (0, 0, MainUI::kW, MainUI::kH);
}
