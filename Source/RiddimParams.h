#pragma once
#include <JuceHeader.h>
#include <array>

// =============================================================================
//  Riddim Lab parameter model.
//
//  Seven "profiles" (the tabs): Master, Drums, Guitar, Bass, Keys, Horns, Vocal.
//  Each profile owns a full set of settings; the audio engine runs the profile
//  that is currently selected (the global "activeSection" parameter).
//
//  Master keeps the ORIGINAL parameter IDs (division, time, feedback, mix, cutoff,
//  resonance, drive, width, damping, tapeSat, tapeTone, wow, flutter, wear,
//  filterMode, sync) so existing sessions keep working. The other profiles use
//  "<profile>_<id>" (e.g. drums_cutoff).
// =============================================================================
namespace rp
{
enum Sec { Master = 0, Drums, Guitar, Bass, Keys, Horns, Vocal, NumSec };

inline constexpr const char* kSecId[NumSec]   { "master", "drums", "guitar", "bass", "keys", "horns", "vocal" };
inline constexpr const char* kSecName[NumSec] { "MASTER", "DRUMS", "GUITAR", "BASS", "KEYS", "HORNS", "VOCAL" };

enum P
{
    InGain, InSat, OutGain, Analog,
    FiltOn, Cutoff, Resonance, FilterMode, FilterRoute,
    CompOn, CompThresh, CompRatio, CompMakeup, CompMode,
    EqOn, EqLow, EqMid, EqHigh, EqPres,
    TapeOn, TapeSat, TapeTone, Wow, Flutter, Wear, TapeType,
    EchoOn, EchoType, Division, Sync, EchoTime, Feedback, Mix, Heat, Width, Damping,
    RevOn, RevType, RevPre, RevDecay, RevMix,
    ModOn, ModType, ModRate, ModDepth, ModMix,
    DlyOn, DlyType, DlyDiv, DlyFeedback, DlyMix,
    LimOn, LimThresh, LimRelease, LimCeiling, LimSoft,
    SrcOn, Src1, Src2, Src3, Src4, Src5, Src6, Src7, Src8, Src9,
    SbOn, SbAmount, SbPunch, SbWeight,
    TapeSpeed, TapeStab, TapeMix,
    SpChar, SpLen, SpDrive, SpFb, SpTreble, SpBass, SpStereo,
    PlDamp, PlLowCut, PlWidth, PlTone,
    Head1, Head2, Head3, EchoRev,
    SubLevel, SubHarm, SpkMode, SpkLevel,
    DrumType, LockOn, LockAmt,
    CtlMon, CtlRoom, CtlWidth, CtlDist, CtlToe, CtlColour, CtlLevel, CtlMono, CtlPhones,
    NumP
};

enum Kind { KFloat, KChoice, KBool };

struct Spec
{
    const char* id;          // base id (master uses it as-is)
    const char* name;
    Kind kind;
    float lo, hi, step, skew;
    const char* choices;     // '|' separated
    int scope;               // 0 every profile, 1 master only, 2 instrument profiles only, 3 guitar only
};

inline const Spec& spec (P p)
{
    static const std::array<Spec, NumP> table
    {{
        { "inGain",     "Input Gain",       KFloat,  -24.0f, 24.0f, 0.1f, 1.0f, nullptr, 0 },
        { "inSat",      "Input Saturation", KBool,   0, 1, 1, 1, nullptr, 0 },
        { "outGain",    "Output Gain",      KFloat,  -24.0f, 24.0f, 0.1f, 1.0f, nullptr, 0 },
        { "analogTone", "Analog Tone",      KBool,   0, 1, 1, 1, nullptr, 0 },

        { "filtOn",     "Filter On",        KBool,   0, 1, 1, 1, nullptr, 0 },
        { "cutoff",     "Filter Cutoff",    KFloat,  180.0f, 12000.0f, 1.0f, 0.35f, nullptr, 0 },
        { "resonance",  "Resonance",        KFloat,  0.1f, 0.92f, 0.001f, 1.0f, nullptr, 0 },
        { "filterMode", "Filter Type",      KChoice, 0, 2, 1, 1, "Low-pass|High-pass|Band-pass", 0 },
        { "filterRoute","Filter Route",     KChoice, 0, 1, 1, 1, "Echo|Input", 0 },

        { "compOn",     "Compressor On",    KBool,   0, 1, 1, 1, nullptr, 0 },
        { "compThresh", "Comp Threshold",   KFloat,  -40.0f, 0.0f, 0.1f, 1.0f, nullptr, 0 },
        { "compRatio",  "Comp Ratio",       KFloat,  1.0f, 20.0f, 0.1f, 0.5f, nullptr, 0 },
        { "compMakeup", "Comp Makeup",      KFloat,  0.0f, 18.0f, 0.1f, 1.0f, nullptr, 0 },
        { "compMode",   "Comp Mode",        KChoice, 0, 2, 1, 1, "Gentle|Punch|Limit", 0 },

        { "eqOn",       "Tone EQ On",       KBool,   0, 1, 1, 1, nullptr, 0 },
        { "eqLow",      "EQ Low",           KFloat,  -12.0f, 12.0f, 0.1f, 1.0f, nullptr, 0 },
        { "eqMid",      "EQ Mid",           KFloat,  -12.0f, 12.0f, 0.1f, 1.0f, nullptr, 0 },
        { "eqHigh",     "EQ High",          KFloat,  -12.0f, 12.0f, 0.1f, 1.0f, nullptr, 0 },
        { "eqPres",     "EQ Presence",      KFloat,  -12.0f, 12.0f, 0.1f, 1.0f, nullptr, 0 },

        { "tapeOn",     "Tape On",          KBool,   0, 1, 1, 1, nullptr, 0 },
        { "tapeSat",    "Tape Saturation",  KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 0 },
        { "tapeTone",   "Tape Tone",        KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 0 },
        { "wow",        "Wow",              KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 0 },
        { "flutter",    "Flutter",          KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 0 },
        { "wear",       "Tape Wear",        KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 0 },
        { "tapeType",   "Tape Type",        KChoice, 0, 2, 1, 1, "Clean|Weight|Vintage", 0 },

        { "echoOn",     "Echo On",          KBool,   0, 1, 1, 1, nullptr, 0 },
        { "echoType",   "Echo Type",        KChoice, 0, 2, 1, 1, "Tape|Digital|Ping-pong", 0 },
        { "division",   "Division",         KChoice, 0, 5, 1, 1, "1/4|1/8|1/8D|1/8T|1/16|1/4D", 0 },
        { "sync",       "Tempo Sync",       KBool,   0, 1, 1, 1, nullptr, 0 },
        { "time",       "Free Time (ms)",   KFloat,  30.0f, 1200.0f, 1.0f, 1.0f, nullptr, 0 },
        { "feedback",   "Feedback",         KFloat,  0.0f, 0.94f, 0.001f, 1.0f, nullptr, 0 },
        { "mix",        "Echo Mix",         KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 0 },
        { "drive",      "Tape Heat",        KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 0 },
        { "width",      "Stereo Offset",    KFloat,  0.0f, 0.5f, 0.001f, 1.0f, nullptr, 0 },
        { "damping",    "Repeat Damping",   KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 0 },

        { "revOn",      "Reverb On",        KBool,   0, 1, 1, 1, nullptr, 0 },
        { "revType",    "Reverb Type",      KChoice, 0, 1, 1, 1, "Spring|Plate", 0 },
        { "revPre",     "Reverb Pre-delay", KFloat,  0.0f, 120.0f, 1.0f, 1.0f, nullptr, 0 },
        { "revDecay",   "Reverb Decay",     KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 0 },
        { "revMix",     "Reverb Mix",       KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 0 },

        { "modOn",      "Modulation On",    KBool,   0, 1, 1, 1, nullptr, 0 },
        { "modType",    "Modulation Type",  KChoice, 0, 2, 1, 1, "Chorus|Phaser|Flanger", 0 },
        { "modRate",    "Mod Rate",         KFloat,  0.05f, 8.0f, 0.01f, 0.4f, nullptr, 0 },
        { "modDepth",   "Mod Depth",        KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 0 },
        { "modMix",     "Mod Mix",          KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 0 },

        { "dlyOn",      "Delay On",         KBool,   0, 1, 1, 1, nullptr, 0 },
        { "dlyType",    "Delay Type",       KChoice, 0, 2, 1, 1, "Analog|Digital|Ping-pong", 0 },
        { "dlyDiv",     "Delay Time",       KChoice, 0, 6, 1, 1, "1/16|1/8|1/4|3/8|1/2|3/4|1/1", 0 },
        { "dlyFeedback","Delay Feedback",   KFloat,  0.0f, 0.9f, 0.001f, 1.0f, nullptr, 0 },
        { "dlyMix",     "Delay Mix",        KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 0 },

        { "limOn",      "Limiter On",       KBool,   0, 1, 1, 1, nullptr, 1 },
        { "limThresh",  "Limiter Threshold", KFloat,  -24.0f, 0.0f, 0.1f, 1.0f, nullptr, 1 },
        { "limRelease", "Limiter Release",  KFloat,  10.0f, 1000.0f, 1.0f, 0.5f, nullptr, 1 },
        { "limCeiling", "Limiter Ceiling",  KFloat,  -12.0f, 0.0f, 0.1f, 1.0f, nullptr, 1 },
        { "limSoft",    "Soft Clip",        KBool,   0, 1, 1, 1, nullptr, 1 },

        { "srcOn",      "Source Voicing On", KBool,   0, 1, 1, 1, nullptr, 2 },
        { "src1",       "Source 1",         KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 2 },
        { "src2",       "Source 2",         KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 2 },
        { "src3",       "Source 3",         KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 2 },
        { "src4",       "Pick 1",           KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 3 },
        { "src5",       "Pick 2",           KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 3 },
        { "src6",       "Pick 3",           KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 3 },
        { "src7",       "Rhythm 1",         KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 3 },
        { "src8",       "Rhythm 2",         KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 3 },
        { "src9",       "Rhythm 3",         KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 3 },

        { "sbOn",       "SteadyBass On",    KBool,   0, 1, 1, 1, nullptr, 4 },
        { "sbAmount",   "SteadyBass Amount",KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 4 },
        { "sbPunch",    "SteadyBass Punch", KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 4 },
        { "sbWeight",   "SteadyBass Weight",KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 4 },

        { "tapeSpeed",  "Tape Speed",       KChoice, 0, 2, 1, 1, "7.5 IPS|15 IPS|30 IPS", 0 },
        { "tapeStab",   "Tape Stability",   KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 0 },
        { "tapeMix",    "Tape Mix",         KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 0 },

        { "spChar",     "Spring Character", KChoice, 0, 6, 1, 1, "Drip|Roots|Twang|Heavy|Surf|Dark Tank|Shimmer", 0 },
        { "spLen",      "Spring Length",    KChoice, 0, 2, 1, 1, "Short|Med|Long", 0 },
        { "spDrive",    "Spring Drive",     KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 0 },
        { "spFb",       "Spring Feedback",  KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 0 },
        { "spTreble",   "Spring Treble",    KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 0 },
        { "spBass",     "Spring Bass",      KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 0 },
        { "spStereo",   "Spring Stereo",    KChoice, 0, 1, 1, 1, "Stereo|Mono", 0 },

        { "plDamp",     "Plate Damping",    KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 0 },
        { "plLowCut",   "Plate Low Cut",    KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 0 },
        { "plWidth",    "Plate Width",      KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 0 },
        { "plTone",     "Plate Tone",       KChoice, 0, 2, 1, 1, "Bright|Natural|Dark", 0 },

        { "head1",      "Echo Head 1",      KBool,   0, 1, 1, 1, nullptr, 0 },
        { "head2",      "Echo Head 2",      KBool,   0, 1, 1, 1, nullptr, 0 },
        { "head3",      "Echo Head 3",      KBool,   0, 1, 1, 1, nullptr, 0 },
        { "echoRev",    "Reverb In Loop",   KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 0 },

        { "subLevel",   "Sub Level",        KFloat,  -12.0f, 12.0f, 0.1f, 1.0f, nullptr, 4 },
        { "subHarm",    "Sub Harmonics",    KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 4 },
        { "spkMode",    "Speaker Check",    KChoice, 0, 4, 1, 1, "Off|Phone|Laptop|MacBook|Monitor", 4 },
        { "spkLevel",   "Speaker Check Level", KFloat, -12.0f, 12.0f, 0.1f, 1.0f, nullptr, 4 },

        { "drumType",   "Drum Type",        KChoice, 0, 5, 1, 1, "Kick|Snare|Rim|Hat|Toms|Loop", 5 },
        { "lockOn",     "Kick Bass Lock",   KChoice, 0, 2, 1, 1, "Off|EQ Lock|Sidechain", 5 },
        { "lockAmt",    "Lock Amount",      KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 5 },

        { "ctlMon",     "Listening Monitor", KChoice, 0, 3, 1, 1, "Off|Nearfield|Midfield|Main", 1 },
        { "ctlRoom",    "Listening Room",   KChoice, 0, 2, 1, 1, "Studio|Control Room|Live Room", 1 },
        { "ctlWidth",   "Speaker Angle",    KFloat,  20.0f, 90.0f, 1.0f, 1.0f, nullptr, 1 },
        { "ctlDist",    "Listening Distance", KFloat, 0.5f, 6.0f, 0.1f, 1.0f, nullptr, 1 },
        { "ctlToe",     "Speaker Toe-in",   KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 1 },
        { "ctlColour",  "Analog Colour",    KFloat,  0.0f, 1.0f, 0.001f, 1.0f, nullptr, 1 },
        { "ctlLevel",   "Listening Level",  KFloat,  -24.0f, 6.0f, 0.1f, 1.0f, nullptr, 1 },
        { "ctlMono",    "Listening Mono",   KBool,   0, 1, 1, 1, nullptr, 1 },
        { "ctlPhones",  "Listening Headphones", KBool, 0, 1, 1, 1, nullptr, 1 },
    }};
    return table[(size_t) p];
}

// Is this parameter registered for this profile?
inline bool exists (Sec s, P p)
{
    const int sc = spec (p).scope;
    if (sc == 0) return true;
    if (sc == 1) return s == Master;
    if (sc == 2) return s != Master;
    if (sc == 4) return s == Bass;
    if (sc == 5) return s == Drums;
    return s == Guitar && (p >= Src4 && p <= Src9);   // guitar-only (Src1..3 exist for every instrument)
}

// Listening aids that must never be saved into presets.
inline bool monitorOnly (P p) { return p == SpkMode || p == SpkLevel || (p >= CtlMon && p <= CtlPhones); }
// Listening (Control tab) settings are global: they always live on the Master profile.
inline bool isControl (P p) { return p >= CtlMon && p <= CtlPhones; }

inline juce::String id (Sec s, P p)
{
    // The three guitar controls that existed before keep their original IDs.
    if (s == Guitar)
    {
        if (p == Src1) return "guitarChopTightness";
        if (p == Src4) return "guitarPickBrightness";
        if (p == Src7) return "guitarRhythmBody";
    }
    const juce::String base (spec (p).id);
    return s == Master ? base : juce::String (kSecId[s]) + "_" + base;
}

inline juce::String displayName (Sec s, P p)
{
    if (s == Master) return spec (p).name;
    juce::String n (kSecName[s]);
    n = n.substring (0, 1) + n.substring (1).toLowerCase();
    return n + " " + spec (p).name;
}

// Default values per profile. Instrument tabs ship with tailored starting settings.
inline float defaultValue (Sec s, P p)
{
    // ---- neutral / Master defaults (the original plugin's defaults where they existed)
    float v = 0.0f;
    switch (p)
    {
        case InGain: case OutGain: v = 0.0f; break;
        case InSat: case Analog: v = 0.0f; break;
        case FiltOn: case EqOn: case TapeOn: case EchoOn: case RevOn: case ModOn: case DlyOn: case LimOn: case SrcOn: v = 1.0f; break;
        case Cutoff: v = 2800.0f; break;
        case Resonance: v = 0.35f; break;
        case FilterMode: case FilterRoute: v = 0.0f; break;
        case CompOn: v = 0.0f; break;
        case CompThresh: v = 0.0f; break;
        case CompRatio: v = 2.5f; break;
        case CompMakeup: v = 0.0f; break;
        case CompMode: v = 0.0f; break;
        case EqLow: case EqMid: case EqHigh: case EqPres: v = 0.0f; break;
        case TapeSat: v = 0.18f; break;
        case TapeTone: v = 0.72f; break;
        case Wow: v = 0.05f; break;
        case Flutter: v = 0.04f; break;
        case Wear: v = 0.0f; break;
        case TapeType: v = 0.0f; break;
        case EchoType: v = 0.0f; break;
        case Division: v = 2.0f; break;
        case Sync: v = 1.0f; break;
        case EchoTime: v = 360.0f; break;
        case Feedback: v = 0.46f; break;
        case Mix: v = 0.26f; break;
        case Heat: v = 0.28f; break;
        case Width: v = 0.14f; break;
        case Damping: v = 0.42f; break;
        case RevType: v = 1.0f; break;
        case RevPre: v = 20.0f; break;
        case RevDecay: v = 0.4f; break;
        case RevMix: v = 0.0f; break;
        case ModType: v = 0.0f; break;
        case ModRate: v = 0.8f; break;
        case ModDepth: v = 0.5f; break;
        case ModMix: v = 0.0f; break;
        case DlyType: v = 0.0f; break;
        case DlyDiv: v = 2.0f; break;
        case DlyFeedback: v = 0.3f; break;
        case DlyMix: v = 0.0f; break;
        case LimThresh: v = 0.0f; break;
        case LimRelease: v = 120.0f; break;
        case LimCeiling: v = -0.3f; break;
        case LimSoft: v = 0.0f; break;
        case TapeSpeed: v = 1.0f; break;
        case TapeStab: v = 1.0f; break;
        case TapeMix: v = 1.0f; break;
        case SpChar: v = 1.0f; break;
        case SpLen: v = 1.0f; break;
        case SpDrive: v = 0.25f; break;
        case SpFb: v = 0.30f; break;
        case SpTreble: case SpBass: v = 0.5f; break;
        case SpStereo: v = 0.0f; break;
        case PlDamp: v = 0.40f; break;
        case PlLowCut: v = 0.15f; break;
        case PlWidth: v = 1.0f; break;
        case PlTone: v = 1.0f; break;
        case Head1: v = 1.0f; break;
        case Head2: case Head3: v = 0.0f; break;
        case EchoRev: v = 0.0f; break;
        case SubLevel: v = 0.0f; break;
        case SubHarm: v = 0.25f; break;
        case SpkMode: v = 0.0f; break;
        case SpkLevel: v = 0.0f; break;
        case DrumType: v = 5.0f; break;
        case LockOn: v = 0.0f; break;
        case LockAmt: v = 0.5f; break;
        case CtlMon: v = 0.0f; break;
        case CtlRoom: v = 1.0f; break;
        case CtlWidth: v = 60.0f; break;
        case CtlDist: v = 2.4f; break;
        case CtlToe: v = 0.5f; break;
        case CtlColour: v = 0.35f; break;
        case CtlLevel: case CtlMono: case CtlPhones: v = 0.0f; break;
        case SbOn: v = 1.0f; break;
        case SbAmount: case SbPunch: case SbWeight: v = 0.5f; break;
        case Src1: case Src2: case Src3: case Src4: case Src5: case Src6: case Src7: case Src8: case Src9: v = 0.5f; break;
        default: break;
    }

    auto setIf = [&] (Sec who, P which, float value) { if (s == who && p == which) v = value; };
    // ---- tailored instrument defaults
    // DRUMS
    setIf (Drums, CompOn, 1); setIf (Drums, CompThresh, -16); setIf (Drums, CompRatio, 4); setIf (Drums, CompMakeup, 3); setIf (Drums, CompMode, 1);
    setIf (Drums, TapeSat, 0.30f); setIf (Drums, Wow, 0.02f); setIf (Drums, Flutter, 0.02f);
    setIf (Drums, Mix, 0.12f); setIf (Drums, Feedback, 0.30f); setIf (Drums, Division, 1);
    setIf (Drums, RevType, 0); setIf (Drums, RevMix, 0.10f); setIf (Drums, RevDecay, 0.35f);
    setIf (Drums, EqLow, 2); setIf (Drums, EqHigh, 2);
    setIf (Drums, Src1, 0.65f); setIf (Drums, Src2, 0.45f); setIf (Drums, Src3, 0.40f);
    // GUITAR
    setIf (Guitar, CompOn, 1); setIf (Guitar, CompThresh, -18); setIf (Guitar, CompRatio, 3); setIf (Guitar, CompMakeup, 2);
    setIf (Guitar, Mix, 0.30f); setIf (Guitar, Feedback, 0.50f); setIf (Guitar, Division, 2); setIf (Guitar, Cutoff, 2400); setIf (Guitar, TapeSat, 0.20f);
    setIf (Guitar, RevType, 0); setIf (Guitar, RevMix, 0.12f);
    setIf (Guitar, Src1, 0.60f); setIf (Guitar, Src2, 0.55f); setIf (Guitar, Src3, 0.35f);
    setIf (Guitar, Src4, 0.50f); setIf (Guitar, Src5, 0.40f); setIf (Guitar, Src6, 0.20f);
    setIf (Guitar, Src7, 0.50f); setIf (Guitar, Src8, 0.30f); setIf (Guitar, Src9, 0.30f);
    // BASS
        setIf (Bass, Mix, 0.10f); setIf (Bass, Feedback, 0.30f); setIf (Bass, Division, 3); setIf (Bass, Cutoff, 1500); setIf (Bass, TapeSat, 0.25f);
    setIf (Bass, EqLow, 2); setIf (Bass, EqHigh, -2);
    setIf (Bass, Src1, 0.50f); setIf (Bass, Src2, 0.50f); setIf (Bass, Src3, 0.25f);
    // KEYS
    setIf (Keys, Mix, 0.24f); setIf (Keys, Feedback, 0.40f); setIf (Keys, Division, 2);
    setIf (Keys, RevType, 1); setIf (Keys, RevMix, 0.20f); setIf (Keys, RevDecay, 0.5f);
    setIf (Keys, ModType, 0); setIf (Keys, ModMix, 0.25f); setIf (Keys, ModRate, 0.6f);
    setIf (Keys, Src1, 0.50f); setIf (Keys, Src2, 0.30f); setIf (Keys, Src3, 0.40f);
    // HORNS
    setIf (Horns, Mix, 0.22f); setIf (Horns, Feedback, 0.38f); setIf (Horns, Division, 0);
    setIf (Horns, RevType, 1); setIf (Horns, RevMix, 0.18f); setIf (Horns, RevDecay, 0.55f);
    setIf (Horns, EqPres, 3);
    setIf (Horns, Src1, 0.50f); setIf (Horns, Src2, 0.50f); setIf (Horns, Src3, 0.30f);
    // VOCAL
    setIf (Vocal, CompOn, 1); setIf (Vocal, CompThresh, -20); setIf (Vocal, CompRatio, 3); setIf (Vocal, CompMakeup, 3);
    setIf (Vocal, Mix, 0.20f); setIf (Vocal, Feedback, 0.35f); setIf (Vocal, Division, 0);
    setIf (Vocal, RevType, 1); setIf (Vocal, RevMix, 0.18f); setIf (Vocal, EqHigh, 1.5f);
    setIf (Vocal, Src1, 0.40f); setIf (Vocal, Src2, 0.40f); setIf (Vocal, Src3, 0.30f);
    return v;
}

inline juce::StringArray choiceList (P p) { return juce::StringArray::fromTokens (spec (p).choices, "|", ""); }

// Source panel: knob names for each profile (and guitar mode).
struct SourceInfo { const char* title; const char* names[3]; };
inline SourceInfo sourceInfo (Sec s, int guitarMode)
{
    switch (s)
    {
        case Drums:  return { "PUNCH  /  TRANSIENTS", { "ATTACK", "SUSTAIN", "SNAP" } };
        case Guitar: return guitarMode == 0 ? SourceInfo { "GUITAR  /  CHOP", { "TIGHTNESS", "DEPTH", "CLICK" } }
                          : guitarMode == 1 ? SourceInfo { "GUITAR  /  PICK", { "BRIGHTNESS", "ATTACK", "LOW CUT" } }
                                            : SourceInfo { "GUITAR  /  RHYTHM", { "BODY", "WARMTH", "GLUE" } };
        case Bass:   return { "ROOTS PRESSURE", { "PRESSURE", "WEIGHT", "GROWL" } };
        case Keys:   return { "PIANO  BANG", { "BANG", "RING", "HAMMER" } };
        case Horns:  return { "BRASS", { "BITE", "BODY", "SOFTEN" } };
        case Vocal:  return { "VOCAL TOOLS", { "DE-ESS", "AIR", "WARMTH" } };
        default:     return { "", { "", "", "" } };
    }
}

// Which source parameters are shown for the current guitar mode.
inline P sourceParam (Sec s, int guitarMode, int knob)
{
    const int base = (s == Guitar ? guitarMode * 3 : 0) + knob;
    return (P) ((int) Src1 + base);
}


// Which processing modules exist on which tab. The audio engine skips modules that
// a tab does not offer, so nothing hidden can colour the sound.
enum Module { MSrc, MSteady, MFilt, MComp, MEq, MTape, MEcho, MRev, MLim };
inline bool avail (Sec s, Module m)
{
    switch (m)
    {
        case MSrc:    return s != Master && s != Bass;
        case MSteady: return s == Bass;
        case MFilt:   return s == Master || s == Guitar || s == Horns;
        case MComp:   return s == Drums || s == Vocal || s == Master;
        case MLim:    return s == Master;
        default:      return true;
    }
}

// Piano Bang voicings: one click sets EQ + tape + source knobs on the Keys tab.
// 0 OLD SCHOOL: upright piano on tape. Warm lows, forward honky mids, rolled-off top, a little tape wobble.
// 1 MODERN: clean stage-piano tone in the Yamaha Motif ES mould. Scooped boxy mids, bright hammer and air, no tape colour.
inline const std::vector<std::pair<P, float>>& pianoVoicing (int which)
{
    static const std::vector<std::pair<P, float>> old
        { { EqOn, 1 }, { EqLow, 3.0f }, { EqMid, 1.5f }, { EqHigh, -4.5f }, { EqPres, 1.0f },
          { TapeOn, 1 }, { TapeType, 1 }, { TapeSat, 0.38f }, { TapeTone, 0.50f }, { Wow, 0.08f }, { Flutter, 0.05f }, { Wear, 0.15f },
          { Src1, 0.62f }, { Src2, 0.30f }, { Src3, 0.55f } };
    static const std::vector<std::pair<P, float>> modern
        { { EqOn, 1 }, { EqLow, 1.0f }, { EqMid, -2.0f }, { EqHigh, 3.5f }, { EqPres, 3.0f },
          { TapeOn, 1 }, { TapeType, 0 }, { TapeSat, 0.08f }, { TapeTone, 0.92f }, { Wow, 0.0f }, { Flutter, 0.0f }, { Wear, 0.0f },
          { Src1, 0.50f }, { Src2, 0.55f }, { Src3, 0.72f } };
    return which == 0 ? old : modern;
}

} // namespace rp
