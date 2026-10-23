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
            { "FOUNDATION RIDDIMS", "Filtered skank echo, vintage tape",
              { { P::Feedback, 0.52f }, { P::Mix, 0.31f }, { P::Cutoff, 2100 }, { P::Resonance, 0.48f }, { P::Heat, 0.36f }, { P::Division, 1 },
                { P::TapeSat, 0.32f }, { P::TapeTone, 0.60f }, { P::Wow, 0.08f }, { P::Flutter, 0.06f }, { P::Wear, 0.20f }, { P::TapeType, 1 },
                { P::RevType, 0 }, { P::RevMix, 0.14f }, { P::RevDecay, 0.45f }, { P::CompOn, 1 }, { P::CompThresh, -16 }, { P::CompRatio, 3 }, { P::CompMakeup, 2 } } },
            { "ROOTS & CULTURE", "Dark worn tape, long repeats",
              { { P::Feedback, 0.38f }, { P::Mix, 0.22f }, { P::Cutoff, 3200 }, { P::Resonance, 0.30f }, { P::Heat, 0.22f }, { P::Division, 0 },
                { P::TapeSat, 0.30f }, { P::TapeTone, 0.55f }, { P::Wow, 0.10f }, { P::Flutter, 0.06f }, { P::Wear, 0.25f }, { P::TapeType, 2 },
                { P::Width, 0.18f }, { P::Damping, 0.50f }, { P::RevType, 1 }, { P::RevMix, 0.12f }, { P::RevDecay, 0.55f } } },
            { "DUB SESSIONS", "Runaway feedback, dub reverb",
              { { P::Feedback, 0.74f }, { P::Mix, 0.36f }, { P::Cutoff, 3600 }, { P::Resonance, 0.62f }, { P::Heat, 0.45f }, { P::Division, 5 },
                { P::TapeSat, 0.42f }, { P::TapeTone, 0.68f }, { P::Wow, 0.12f }, { P::Flutter, 0.09f }, { P::Wear, 0.30f }, { P::TapeType, 1 },
                { P::RevType, 4 }, { P::RevMix, 0.22f }, { P::RevDecay, 0.70f } } },
            { "LOVERS ROCK", "Soft, bright repeats with chorus",
              { { P::Feedback, 0.34f }, { P::Mix, 0.24f }, { P::Cutoff, 4500 }, { P::Resonance, 0.25f }, { P::Heat, 0.18f }, { P::Division, 2 },
                { P::TapeSat, 0.15f }, { P::TapeTone, 0.80f }, { P::Wow, 0.06f }, { P::Flutter, 0.03f }, { P::Wear, 0.05f }, { P::TapeType, 0 },
                { P::Width, 0.22f }, { P::Damping, 0.35f }, { P::RevType, 1 }, { P::RevMix, 0.20f }, { P::RevDecay, 0.50f },
                { P::ModOn, 1 }, { P::ModType, 0 }, { P::ModMix, 0.22f }, { P::ModRate, 0.7f }, { P::ModDepth, 0.5f } } },
            { "ROCKSTEADY", "Short, clean slap echo",
              { { P::Feedback, 0.26f }, { P::Mix, 0.20f }, { P::Cutoff, 5200 }, { P::Resonance, 0.20f }, { P::Heat, 0.20f }, { P::Division, 4 }, { P::EchoType, 1 },
                { P::TapeSat, 0.22f }, { P::TapeTone, 0.70f }, { P::Wow, 0.04f }, { P::Flutter, 0.03f }, { P::Wear, 0.08f }, { P::TapeType, 0 },
                { P::RevType, 3 }, { P::RevMix, 0.12f }, { P::RevDecay, 0.30f } } },
            { "DANCEHALL VIBES", "Bright ping-pong echoes",
              { { P::Feedback, 0.48f }, { P::Mix, 0.30f }, { P::Cutoff, 6500 }, { P::Resonance, 0.40f }, { P::Heat, 0.30f }, { P::Division, 3 }, { P::EchoType, 2 },
                { P::TapeSat, 0.20f }, { P::TapeTone, 0.85f }, { P::Wow, 0.03f }, { P::Flutter, 0.02f }, { P::Wear, 0.0f }, { P::TapeType, 0 },
                { P::Width, 0.25f }, { P::Damping, 0.30f }, { P::DlyOn, 1 }, { P::DlyMix, 0.14f }, { P::DlyFeedback, 0.3f }, { P::DlyDiv, 1 },
                { P::RevType, 3 }, { P::RevMix, 0.10f } } },
            { "VOCAL TOOLS", "Subtle vocal echo, plate verb",
              { { P::Feedback, 0.32f }, { P::Mix, 0.22f }, { P::Cutoff, 2400 }, { P::Resonance, 0.22f }, { P::Heat, 0.12f }, { P::Division, 0 },
                { P::TapeSat, 0.10f }, { P::TapeTone, 0.75f }, { P::Wow, 0.02f }, { P::Flutter, 0.02f }, { P::Wear, 0.03f },
                { P::Width, 0.20f }, { P::Damping, 0.55f }, { P::RevType, 1 }, { P::RevMix, 0.18f }, { P::RevDecay, 0.45f },
                { P::CompOn, 1 }, { P::CompThresh, -20 }, { P::CompRatio, 3 }, { P::CompMakeup, 3 }, { P::EqHigh, 1.5f } } },
            { "MIX TEMPLATES", "Glue, tape and a touch of air",
              { { P::Feedback, 0.20f }, { P::Mix, 0.08f }, { P::Cutoff, 6000 }, { P::Resonance, 0.2f }, { P::Heat, 0.1f }, { P::Division, 1 },
                { P::CompOn, 1 }, { P::CompThresh, -14 }, { P::CompRatio, 2 }, { P::CompMakeup, 2 }, { P::CompMode, 0 },
                { P::EqLow, 1 }, { P::EqHigh, 1.5f }, { P::TapeSat, 0.25f }, { P::TapeType, 0 }, { P::TapeTone, 0.85f },
                { P::Wow, 0.0f }, { P::Flutter, 0.0f }, { P::Wear, 0.0f }, { P::RevType, 2 }, { P::RevMix, 0.05f }, { P::RevDecay, 0.4f } } }
        };
        return f;
    }

    String srcHelp (rp::Sec s, int mode, int i)
    {
        static const char* drums[3]  { "Boost or soften the front of every hit", "Lengthen or tighten the ring after the hit", "Add stick crack in the upper mids" };
        static const char* chop[3]   { "How quickly each chord is cut off", "How far the signal drops between chops", "Pick click on the chop" };
        static const char* pick[3]   { "Lift the pick sound", "Sharpen or soften the pluck", "High-pass the body to thin the guitar" };
        static const char* rhythm[3] { "Low-mid weight", "Round, saturated top end", "Gentle compression to bind the strum" };
        static const char* bass[3]   { "Parallel compression on the low end", "Boost the fundamental", "Upper harmonics so it speaks on small speakers" };
        static const char* keys[3]   { "Top-end air", "Tremolo depth", "Tremolo speed" };
        static const char* horns[3]  { "Presence around 2.4 kHz", "Weight around 350 Hz", "Round off the top end" };
        static const char* vocal[3]  { "Tame harsh sibilance", "Add sparkle above 11 kHz", "Chest warmth and a little saturation" };
        switch (s)
        {
            case rp::Drums:  return drums[i];
            case rp::Guitar: return mode == 0 ? chop[i] : (mode == 1 ? pick[i] : rhythm[i]);
            case rp::Bass:   return bass[i];
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
            case P::ModRate: return "Modulation speed";
            case P::ModDepth: return "Modulation amount";
            case P::ModMix: return "Modulation level against the dry signal";
            case P::DlyDiv: return "Tempo-synced time of the second delay";
            case P::DlyFeedback: return "Repeats of the second delay";
            case P::DlyMix: return "Second delay level";
            case P::LimThresh: return "Level where the limiter starts working";
            case P::LimRelease: return "How fast the limiter lets go";
            case P::LimCeiling: return "The output can never go above this level";
            default: return "";
        }
    }
}

//==============================================================================
struct MainUI::LibItem : public TextButton
{
    int index = 0; String name, sub;
    void paintButton (Graphics& g, bool over, bool down) override
    {
        getLookAndFeel().drawButtonBackground (g, *this, Colours::transparentBlack, over, down);
        auto r = getLocalBounds().toFloat().reduced (3.0f);
        auto th = r.removeFromLeft (64.0f).withSizeKeepingCentre (64.0f, 46.0f);
        auto& im = ui::Assets::get().thumbs[index];
        g.setImageResamplingQuality (Graphics::highResamplingQuality);
        g.drawImage (im, th);
        g.setColour (Colours::black.withAlpha (0.8f)); g.drawRect (th, 1.0f);
        const bool on = getToggleState();
        r.removeFromLeft (6.0f);
        auto top = r.removeFromTop (r.getHeight() * 0.46f);
        ui::engraved (g, name, top.translated (0, 3), ui::font (12.5f, 2, 0.03f), on ? ui::col::amber : ui::col::cream, Justification::centredLeft);
        g.setFont (ui::font (11.0f, 0, 0.0f));
        g.setColour (ui::col::creamDim);
        g.drawFittedText (sub, r.toNearestInt().translated (0, 1), Justification::topLeft, 2, 0.9f);
    }
};

//==============================================================================
MainUI::MainUI (RiddimLabAudioProcessor& p) : proc (p)
{
    setLookAndFeel (&look);
    ui::Assets::get();
    setBufferedToImage (true);
    setOpaque (true);

    auto info = [this] (const String& s) { setInfo (s); };

    // ---- nav tabs
    const char* names[7] { "MASTER", "DRUMS", "GUITAR", "BASS", "KEYS", "HORNS", "VOCAL" };
    for (int i = 0; i < 7; ++i)
    {
        nav[i].setButtonText (names[i]);
        nav[i].getProperties().set ("role", "nav");
        nav[i].setClickingTogglesState (false);
        nav[i].onClick = [this, i] { selectTab (i, gMode, true); };
        addAndMakeVisible (nav[i]);
    }
    const char* subNames[3] { "CHOP", "PICK", "RHYTHM" };
    for (int i = 0; i < 3; ++i)
    {
        sub[i].setButtonText (subNames[i]);
        sub[i].getProperties().set ("role", "subnav");
        sub[i].onClick = [this, i] { selectTab (rp::Guitar, i, true); };
        addAndMakeVisible (sub[i]);
    }

    // ---- knobs (captions as in the mockup)
    using K = P;
    addKnob (K::InGain, "GAIN", 68, 3);   addKnob (K::OutGain, "LEVEL", 68, 3);
    addKnob (K::Cutoff, "CUTOFF", 68, 1);  addKnob (K::Resonance, "RESONANCE", 68, 0);
    addKnob (K::CompThresh, "THRESHOLD", 68, 3); addKnob (K::CompRatio, "RATIO", 68, 4); addKnob (K::CompMakeup, "MAKEUP", 68, 3);
    addKnob (K::EqLow, "LOW", 58, 3); addKnob (K::EqMid, "MID", 58, 3); addKnob (K::EqHigh, "HIGH", 58, 3); addKnob (K::EqPres, "PRESENCE", 58, 3);
    addKnob (K::TapeSat, "SATURATE", 46, 0); addKnob (K::TapeTone, "TONE", 46, 0); addKnob (K::Wow, "WOW", 46, 0);
    addKnob (K::Flutter, "FLUTTER", 46, 0); addKnob (K::Wear, "WEAR", 46, 0);
    echoTimeKnob = addKnob (K::EchoTime, "TIME", 68, 2, true);
    addKnob (K::Feedback, "FEEDBACK", 68, 0); addKnob (K::Mix, "MIX", 68, 0);
    addKnob (K::Heat, "TAPE HEAT", 46, 0); addKnob (K::Width, "OFFSET", 46, 0); addKnob (K::Damping, "DAMPING", 46, 0);
    addKnob (K::RevPre, "PRE-DELAY", 68, 2); addKnob (K::RevDecay, "DECAY", 68, 0); addKnob (K::RevMix, "MIX", 68, 0);
    addKnob (K::ModRate, "RATE", 58, 7); addKnob (K::ModDepth, "DEPTH", 58, 0); addKnob (K::ModMix, "MIX", 58, 0);
    addKnob (K::DlyDiv, "TIME", 58, 5); addKnob (K::DlyFeedback, "FEEDBACK", 58, 0); addKnob (K::DlyMix, "MIX", 58, 0);
    for (auto k : { K::LimThresh, K::LimRelease, K::LimCeiling })
    {
        auto* kn = addKnob (k, k == K::LimThresh ? "THRESHOLD" : (k == K::LimRelease ? "RELEASE" : "CEILING"), 58, k == K::LimRelease ? 2 : 3);
        limKnobs.push_back (kn);
    }
    for (int i = 0; i < 3; ++i) srcKnobs.push_back (addKnob (K::Src1, "", 58, 0));
    // the three source knobs are distinguished by index (their parameter changes with the tab)
    {
        int idx = 0;
        for (auto& kd : knobs) if (kd->p == K::Src1) { kd->p = (P) ((int) K::Src1 + idx); ++idx; }   // placeholder order; rebound in rebindAll
    }
    for (auto& kd : knobs) { kd->k->onInfo = info; if (kd->p < K::Src1 || kd->p > K::Src9) kd->k->help = kHelp (kd->p); }

    // ---- lamps and choices
    lSat = addLamp (std::make_unique<ui::Lamp> (ui::Lamp::LampLabelRight, "SATURATION", 5.5f));
    lAnalog = addLamp (std::make_unique<ui::Lamp> (ui::Lamp::LampLabelRight, "ANALOG TONE", 5.5f));
    lSync = addLamp (std::make_unique<ui::Lamp> (ui::Lamp::LampLabelRight, "TEMPO SYNC", 5.5f));
    lSoft = addLamp (std::make_unique<ui::Lamp> (ui::Lamp::Plate, "SOFT CLIP"));
    lBypass = addLamp (std::make_unique<ui::Lamp> (ui::Lamp::Plate, "BYPASS"));
    lThrow = addLamp (std::make_unique<ui::Lamp> (ui::Lamp::Momentary, "THROW"));
    lPanic = addLamp (std::make_unique<ui::Lamp> (ui::Lamp::Momentary, "PANIC"));
    auto pw = [&] { return addLamp (std::make_unique<ui::Lamp> (ui::Lamp::LampOnly, "", 5.0f)); };
    pFilt = pw(); pComp = pw(); pEq = pw(); pTape = pw(); pEcho = pw(); pRev = pw(); pMod = pw(); pDly = pw(); pLim = pw(); pSrc = pw();
    for (int i = 0; i < 10; ++i) flow[i] = addLamp (std::make_unique<ui::Lamp> (ui::Lamp::LampOnly, "", 7.0f));
    lSat->onInfo = lAnalog->onInfo = lSync->onInfo = lSoft->onInfo = lBypass->onInfo = lThrow->onInfo = lPanic->onInfo = info;

    cFilterMode  = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "LOW-PASS", "HIGH-PASS", "BAND-PASS" }, true, 12.5f));
    cFilterRoute = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "ECHO", "INPUT" }, false, 12.0f));
    cCompMode    = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "GENTLE", "PUNCH", "LIMIT" }, true, 12.5f));
    cTapeType    = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "CLEAN", "VINTAGE", "WORN" }, false, 12.0f));
    cEchoType    = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "TAPE", "DIGITAL", "PING-PONG" }, true, 12.5f));
    cRevType     = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "SPRING", "PLATE", "HALL", "ROOM", "DUB" }, false, 13.0f));
    cModType     = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "CHORUS", "PHASER", "FLANGER" }, true, 12.5f));
    cDlyType     = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "ANALOG", "DIGITAL", "PING-PONG" }, true, 12.5f));

    // ---- meters
    for (auto* m : { &meterL, &meterR, &meterIn, &meterGr }) addAndMakeVisible (m);
    addAndMakeVisible (ladder);
    addAndMakeVisible (deck);

    // ---- library + presets
    const auto& fp = factoryPresets();
    for (int i = 0; i < (int) fp.size(); ++i)
    {
        auto it = std::make_unique<LibItem>();
        it->index = i; it->name = fp[(size_t) i].name; it->sub = fp[(size_t) i].sub;
        it->getProperties().set ("role", "lib");
        it->setClickingTogglesState (true); it->setRadioGroupId (7423);
        it->onClick = [this, i] { applyFactoryPreset (i); };
        addAndMakeVisible (*it);
        lib.push_back (std::move (it));
    }
    presetBox.setTextWhenNothingSelected ("INIT");
    presetBox.setJustificationType (Justification::centredLeft);
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id >= 1 && id <= (int) factoryPresets().size()) applyFactoryPreset (id - 1);
        else if (id >= 101 && id - 101 < (int) userPresets.size()) applyUserPreset (userPresets[(size_t) (id - 101)]);
    };
    addAndMakeVisible (presetBox);
    struct B { TextButton* b; const char* t; std::function<void()> f; };
    for (auto& b : std::initializer_list<B> {
            { &prevB, "<", [this] { stepPreset (-1); } }, { &nextB, ">", [this] { stepPreset (1); } },
            { &loadB, "LOAD", [this] { showLoadMenu(); } }, { &saveB, "SAVE", [this] { saveUserPreset(); } },
            { &delB, "DELETE", [this] { deleteCurrentPreset(); } }, { &browseB, "BROWSE", [this] { auto d = presetFolder(); d.createDirectory(); d.revealToUser(); } } })
    {
        b.b->setButtonText (b.t); b.b->getProperties().set ("role", "brass"); b.b->onClick = b.f; addAndMakeVisible (*b.b);
    }
    presetInfo.setFont (ui::font (13.0f, 1)); presetInfo.setColour (Label::textColourId, ui::col::creamDim);
    presetInfo.setJustificationType (Justification::centredLeft); presetInfo.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (presetInfo);

    // ---- state: tab + sync
    secAtt = std::make_unique<ParameterAttachment> (*proc.parameters.getParameter ("activeSection"), [this] (float v) { if (roundToInt (v) != sec) selectTab (roundToInt (v), gMode, false); });
    modeAtt = std::make_unique<ParameterAttachment> (*proc.parameters.getParameter ("guitarMode"), [this] (float v) { if (roundToInt (v) != gMode) selectTab (sec, roundToInt (v), false); });
    sec = jlimit (0, 6, roundToInt (proc.parameters.getRawParameterValue ("activeSection")->load()));
    gMode = jlimit (0, 2, roundToInt (proc.parameters.getRawParameterValue ("guitarMode")->load()));
    selectTab (sec, gMode, false);

    refreshPresetBox();
    setCurrentPreset (proc.parameters.state.getProperty ("presetName", "").toString(), -1, {});
    setSize (kW, kH);
    startTimerHz (30);
}

MainUI::~MainUI()
{
    stopTimer();
    secAtt.reset(); modeAtt.reset(); syncAtt.reset();
    knobs.clear(); lamps.clear(); choices.clear();
    setLookAndFeel (nullptr);
}

ui::Knob* MainUI::addKnob (P p, const String& caption, int diameter, int fmt, bool echoTime)
{
    auto kd = std::make_unique<KnobDef>();
    kd->k = std::make_unique<ui::Knob> (caption, diameter); kd->p = p; kd->fmt = fmt; kd->echoTime = echoTime;
    addAndMakeVisible (*kd->k);
    auto* raw = kd->k.get();
    knobs.push_back (std::move (kd));
    return raw;
}
ui::Lamp* MainUI::addLamp (std::unique_ptr<ui::Lamp> l) { addAndMakeVisible (*l); lamps.push_back (std::move (l)); return lamps.back().get(); }
ui::ChoiceLamps* MainUI::addChoice (std::unique_ptr<ui::ChoiceLamps> c) { addAndMakeVisible (*c); choices.push_back (std::move (c)); return choices.back().get(); }

void MainUI::setInfo (const String& s) { if (s != infoText) { infoText = s; repaint (rInfo); } }

//------------------------------------------------------------------ tab handling and binding
void MainUI::selectTab (int section, int guitarMode, bool writeParams)
{
    sec = jlimit (0, 6, section); gMode = jlimit (0, 2, guitarMode);
    if (writeParams)
    {
        for (auto [id, v] : { std::pair<const char*, int> { "activeSection", sec }, { "guitarMode", gMode } })
            if (auto* prm = proc.parameters.getParameter (id)) { prm->beginChangeGesture(); prm->setValueNotifyingHost (prm->convertTo0to1 ((float) v)); prm->endChangeGesture(); }
    }
    for (int i = 0; i < 7; ++i) nav[i].setToggleState (i == sec, dontSendNotification);
    for (int i = 0; i < 3; ++i) sub[i].setToggleState (sec == rp::Guitar && i == gMode, dontSendNotification);
    rebindAll();
    repaint();
}

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
    auto idOf = [&] (P p) { return rp::exists (s, p) ? rp::id (s, p) : String(); };
    const auto src = rp::sourceInfo (s, gMode);
    int srcIdx = 0;
    for (auto& kd : knobs)
    {
        if (kd->echoTime) continue;
        P p = kd->p;
        if (p >= P::Src1 && p <= P::Src9)
        {
            // source knobs: i-th source knob of the current tab/mode
            const int i = srcIdx++;
            p = rp::sourceParam (s, gMode, i);
            kd->k->setCaption (s == rp::Master ? String() : String (src.names[i]));
            kd->k->help = srcHelp (s, gMode, i);
            kd->k->bind (proc.parameters, idOf (p), 0, rp::defaultValue (s, p));
            kd->k->setVisible (s != rp::Master);
            continue;
        }
        const bool isLim = (p == P::LimThresh || p == P::LimRelease || p == P::LimCeiling);
        kd->k->setVisible (isLim ? s == rp::Master : true);
        kd->k->bind (proc.parameters, idOf (p), kd->fmt, rp::defaultValue (s, p),
                     p == P::DlyDiv ? rp::choiceList (P::DlyDiv) : StringArray());
    }
    updateEchoTimeBinding();

    auto bindL = [&] (ui::ParamWidget* w, P p) { w->bindParam (proc.parameters, idOf (p)); };
    bindL (lSat, P::InSat); bindL (lAnalog, P::Analog); bindL (lSync, P::Sync);
    lSoft->bindParam (proc.parameters, rp::id (rp::Master, P::LimSoft)); lSoft->setVisible (s == rp::Master);
    lBypass->bindParam (proc.parameters, "bypass"); lThrow->bindParam (proc.parameters, "throw"); lPanic->bindParam (proc.parameters, "panic");
    bindL (pFilt, P::FiltOn); bindL (pComp, P::CompOn); bindL (pEq, P::EqOn); bindL (pTape, P::TapeOn); bindL (pEcho, P::EchoOn);
    bindL (pRev, P::RevOn); bindL (pMod, P::ModOn); bindL (pDly, P::DlyOn);
    pLim->bindParam (proc.parameters, rp::id (rp::Master, P::LimOn)); pLim->setVisible (s == rp::Master);
    bindL (pSrc, P::SrcOn); pSrc->setVisible (s != rp::Master);
    const P order[10] { P::SrcOn, P::FiltOn, P::CompOn, P::EqOn, P::TapeOn, P::EchoOn, P::DlyOn, P::ModOn, P::RevOn, P::LimOn };
    for (int i = 0; i < 10; ++i)
        flow[i]->bindParam (proc.parameters, i == 9 ? rp::id (rp::Master, P::LimOn) : idOf (order[i]));
    bindL (cFilterMode, P::FilterMode); bindL (cFilterRoute, P::FilterRoute); bindL (cCompMode, P::CompMode); bindL (cTapeType, P::TapeType);
    bindL (cEchoType, P::EchoType); bindL (cRevType, P::RevType); bindL (cModType, P::ModType); bindL (cDlyType, P::DlyType);

    syncAtt.reset();
    syncAtt = std::make_unique<ParameterAttachment> (*proc.parameters.getParameter (rp::id (s, P::Sync)), [this] (float v) { if ((v > 0.5f) != syncOn) updateEchoTimeBinding(); });
}

//------------------------------------------------------------------ layout
void MainUI::place (Component& c, float cx, int top, int w, int h) { c.setBounds (roundToInt (cx - w * 0.5f), top, w, h); }

void MainUI::resized()
{
    rHeaderArt = { 16, 12, 568, 138 };
    rVuPanel = { 592, 12, 408, 138 };
    rPreset  = { 1008, 12, 258, 138 };
    rLibrary = { 14, 156, 192, 692 };
    rInfo    = { 14, 856, 1252, 28 };
    const int x0 = 214;
    rInput  = { x0, 208, 156, 146 };       rMeters = { x0 + 164, 208, 432, 146 };
    rOutput = { x0 + 604, 208, 156, 146 }; rFlow = { x0 + 768, 208, 284, 146 };
    rFilter = { x0, 362, 328, 130 };       rComp = { x0 + 336, 362, 400, 130 };      rEq = { x0 + 744, 362, 308, 130 };
    rTape   = { x0, 500, 346, 210 };       rEcho = { x0 + 354, 500, 392, 210 };      rReverb = { x0 + 754, 500, 298, 210 };
    rMod    = { x0, 718, 328, 130 };       rDelay = { x0 + 336, 718, 400, 130 };     rLimSrc = { x0 + 744, 718, 308, 130 };

    // nav
    {
        const int h = 44, y = 156, g = 6; int x = x0;
        for (int i = 0; i < 7; ++i)
        {
            if (i == rp::Guitar)
            {
                nav[i].setBounds (x, y, 200, 23);
                for (int k = 0; k < 3; ++k) sub[k].setBounds (x + k * 67, y + 24, 66, 20);
                x += 200 + g;
            }
            else { nav[i].setBounds (x, y, 136, h); x += 136 + g; }
        }
    }
    auto knobAt = [&] (P p, const Rectangle<int>& panel, float cx, int top)
    {
        for (auto& kd : knobs)
            if (kd->p == p && ! (p >= P::Src1 && p <= P::Src9)) { place (*kd->k, panel.getX() + cx, panel.getY() + top, kd->k->preferredWidth(), kd->k->preferredHeight()); return; }
    };
    auto lampAt = [&] (Component* c, const Rectangle<int>& panel, int x, int y, int w, int h) { c->setBounds (panel.getX() + x, panel.getY() + y, w, h); };
    auto power  = [&] (ui::Lamp* l, const Rectangle<int>& panel) { l->setBounds (panel.getX() + 14, panel.getY() + 4, 20, 20); };

    // Row A
    knobAt (P::InGain, rInput, 78, 28);   lampAt (lSat, rInput, 24, 126, 112, 18);
    knobAt (P::OutGain, rOutput, 78, 28); lampAt (lAnalog, rOutput, 14, 126, 130, 18);
    meterIn.setBounds (rMeters.getX() + 10, rMeters.getY() + 6, 200, 122);
    meterGr.setBounds (rMeters.getRight() - 210, rMeters.getY() + 6, 200, 122);
    {   // signal flow
        for (int i = 0; i < 10; ++i)
        {
            const int row = i / 5, col = i % 5;
            const float cx = rFlow.getX() + 30 + col * 56.0f; const int cy = rFlow.getY() + (row == 0 ? 50 : 86);
            flow[i]->setBounds (roundToInt (cx) - 11, cy - 11, 22, 22);
        }
        const int by = rFlow.getY() + 110, bw = 82;
        lBypass->setBounds (rFlow.getX() + 12, by, bw, 26); lThrow->setBounds (rFlow.getX() + 12 + bw + 6, by, bw, 26); lPanic->setBounds (rFlow.getX() + 12 + 2 * (bw + 6), by, bw, 26);
    }
    // Row B
    power (pFilt, rFilter); power (pComp, rComp); power (pEq, rEq);
    knobAt (P::Cutoff, rFilter, 54, 30);  knobAt (P::Resonance, rFilter, 142, 30);
    lampAt (cFilterMode, rFilter, 214, 52, 106, 51); lampAt (cFilterRoute, rFilter, 214, 5, 108, 20);
    knobAt (P::CompThresh, rComp, 54, 30); knobAt (P::CompRatio, rComp, 142, 30); knobAt (P::CompMakeup, rComp, 230, 30);
    lampAt (cCompMode, rComp, 302, 46, 90, 51);
    { const float cxs[4] { 42, 116, 190, 264 }; const P ps[4] { P::EqLow, P::EqMid, P::EqHigh, P::EqPres };
      for (int i = 0; i < 4; ++i) knobAt (ps[i], rEq, cxs[i] + 2, 32); }
    // Row C
    power (pTape, rTape); power (pEcho, rEcho); power (pRev, rReverb);
    lampAt (cTapeType, rTape, 180, 5, 156, 20);
    deck.setBounds (rTape.getX() + 8, rTape.getY() + 32, 330, 96);
    { const float cxs[5] { 48, 111, 174, 237, 300 }; const P ps[5] { P::TapeSat, P::TapeTone, P::Wow, P::Flutter, P::Wear };
      for (int i = 0; i < 5; ++i) knobAt (ps[i], rTape, cxs[i] + 0, 130); }
    for (auto& kd : knobs) if (kd->echoTime) place (*kd->k, rEcho.getX() + 52, rEcho.getY() + 30, kd->k->preferredWidth(), kd->k->preferredHeight());
    knobAt (P::Feedback, rEcho, 140, 30); knobAt (P::Mix, rEcho, 228, 30);
    knobAt (P::Heat, rEcho, 52, 130); knobAt (P::Width, rEcho, 140, 130); knobAt (P::Damping, rEcho, 228, 130);
    lampAt (cEchoType, rEcho, 292, 60, 96, 51); lampAt (lSync, rEcho, 292, 126, 100, 18);
    knobAt (P::RevPre, rReverb, 56, 30); knobAt (P::RevDecay, rReverb, 150, 30); knobAt (P::RevMix, rReverb, 244, 30);
    lampAt (cRevType, rReverb, 14, 156, 270, 24);
    // Row D
    power (pMod, rMod); power (pDly, rDelay); power (pLim, rLimSrc); power (pSrc, rLimSrc);
    knobAt (P::ModRate, rMod, 46, 30); knobAt (P::ModDepth, rMod, 124, 30); knobAt (P::ModMix, rMod, 202, 30);
    lampAt (cModType, rMod, 240, 48, 84, 51);
    knobAt (P::DlyDiv, rDelay, 46, 30); knobAt (P::DlyFeedback, rDelay, 124, 30); knobAt (P::DlyMix, rDelay, 202, 30);
    lampAt (cDlyType, rDelay, 288, 48, 106, 51);
    { const float cxs[3] { 46, 120, 194 };
      for (int i = 0; i < 3; ++i)
      {
          place (*limKnobs[(size_t) i], rLimSrc.getX() + cxs[i], rLimSrc.getY() + 30, limKnobs[(size_t) i]->preferredWidth(), limKnobs[(size_t) i]->preferredHeight());
          place (*srcKnobs[(size_t) i], rLimSrc.getX() + cxs[i], rLimSrc.getY() + 30, srcKnobs[(size_t) i]->preferredWidth(), srcKnobs[(size_t) i]->preferredHeight());
      } }
    lampAt (lSoft, rLimSrc, 232, 54, 70, 28);

    // header
    meterL.setBounds (rVuPanel.getX() + 8, rVuPanel.getY() + 12, 160, 100);
    meterR.setBounds (rVuPanel.getRight() - 168, rVuPanel.getY() + 12, 160, 100);
    ladder.setBounds (rVuPanel.getCentreX() - 18, rVuPanel.getY() + 14, 36, 96);
    {
        auto r = rPreset.reduced (12, 0);
        presetBox.setBounds (r.getX(), rPreset.getY() + 34, r.getWidth() - 62, 30);
        prevB.setBounds (r.getRight() - 58, rPreset.getY() + 34, 28, 30); nextB.setBounds (r.getRight() - 28, rPreset.getY() + 34, 28, 30);
        const int bw = (r.getWidth() - 3 * 4) / 4;
        Button* bs[4] { &loadB, &saveB, &delB, &browseB };
        for (int i = 0; i < 4; ++i) bs[i]->setBounds (r.getX() + i * (bw + 4), rPreset.getY() + 74, bw, 28);
        presetInfo.setBounds (r.getX(), rPreset.getY() + 108, r.getWidth(), 22);
    }
    // library
    {
        auto r = rLibrary.reduced (8, 0).withTrimmedTop (38);
        for (auto& it : lib) it->setBounds (r.removeFromTop (62)), r.removeFromTop (3);
    }
}

//------------------------------------------------------------------ painting (cached; the live parts are child components)
void MainUI::paint (Graphics& g)
{
    auto& A = ui::Assets::get();
    // wood frame + steel back plate
    g.setTiledImageFill (A.wood, 0, 0, 1.0f); g.fillAll();
    g.setGradientFill (ColourGradient (Colours::black.withAlpha (0.0f), kW * 0.5f, kH * 0.5f, Colours::black.withAlpha (0.55f), 0, 0, true)); g.fillAll();
    auto plate = Rectangle<float> (7, 7, kW - 14.0f, kH - 14.0f);
    g.setColour (Colours::black); g.fillRoundedRectangle (plate.expanded (2), 6);
    g.setTiledImageFill (A.dark, 0, 0, 1.0f); g.fillRoundedRectangle (plate, 5);
    g.setGradientFill (ColourGradient (Colours::black.withAlpha (0.0f), kW * 0.5f, kH * 0.4f, Colours::black.withAlpha (0.55f), 0, 0, true)); g.fillRoundedRectangle (plate, 5);
    g.setColour (ui::col::brass.withAlpha (0.55f)); g.drawRoundedRectangle (plate.reduced (1.0f), 5, 1.4f);
    for (auto c : { Point<float> (kW * 0.5f, 3.5f), Point<float> (kW * 0.5f, kH - 3.5f) }) ui::drawScrew (g, c, 7.0f, 1);

    const float sh = 28.0f;
    auto P_ = [&] (Rectangle<int> r, const Image& tex, int seed, const String& title, float strip, float inset = 22.0f) { ui::paintPanel (g, r.toFloat(), tex, seed, title, strip, true, inset); };

    // header
    g.setImageResamplingQuality (Graphics::highResamplingQuality);
    g.drawImage (A.header, rHeaderArt.toFloat(), RectanglePlacement::centred);
    P_ (rVuPanel, A.dark, 3, {}, 0);
    ui::engraved (g, "OUTPUT  LEFT", { (float) rVuPanel.getX() + 8, (float) rVuPanel.getY() + 114, 160, 18 }, ui::font (13, 2, 0.12f), ui::col::cream);
    ui::engraved (g, "OUTPUT  RIGHT", { (float) rVuPanel.getRight() - 168, (float) rVuPanel.getY() + 114, 160, 18 }, ui::font (13, 2, 0.12f), ui::col::cream);
    P_ (rPreset, A.green2, 4, "PRESET", 28);
    // library
    P_ (rLibrary, A.dark, 5, "LIBRARY", 30);
    {
        auto plateR = Rectangle<float> ((float) rLibrary.getX() + 10, (float) rLibrary.getBottom() - 74.0f, rLibrary.getWidth() - 20.0f, 62.0f);
        g.setTiledImageFill (A.brass, 0, 0, 0.5f); g.fillRoundedRectangle (plateR, 3);
        g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.28f), 0, plateR.getY(), Colours::black.withAlpha (0.45f), 0, plateR.getBottom(), false)); g.fillRoundedRectangle (plateR, 3);
        g.setColour (Colours::black.withAlpha (0.75f)); g.drawRoundedRectangle (plateR, 3, 1.2f);
        ui::engraved (g, "ECLECTIC PRODZ", plateR.removeFromTop (34).translated (0, 6), ui::font (21, 3, 0.1f), Colour (0xff2b1b06), Justification::centred, 0.9f);
        ui::engraved (g, "KINGSTON  SOUND  LABORATORY", plateR, ui::font (11, 2, 0.14f), Colour (0xff3b2808), Justification::centredTop, 0.9f);
    }
    // row A
    P_ (rInput, A.green, 6, "INPUT", sh);  P_ (rOutput, A.green, 7, "OUTPUT", sh);
    P_ (rMeters, A.dark, 8, {}, 0);
    ui::engraved (g, "INPUT LEVEL", { (float) rMeters.getX() + 10, (float) rMeters.getBottom() - 17.0f, 200, 14 }, ui::font (12.5f, 2, 0.12f), ui::col::cream);
    ui::engraved (g, "GAIN REDUCTION", { (float) rMeters.getRight() - 210, (float) rMeters.getBottom() - 17.0f, 200, 14 }, ui::font (12.5f, 2, 0.12f), ui::col::cream);
    P_ (rFlow, A.green2, 9, "SIGNAL FLOW", sh);
    {   // cable between lamps, IN / OUT tags
        const char* lab[10] { "SRC", "FILT", "COMP", "EQ", "TAPE", "ECHO", "DLY", "MOD", "REV", "LIM" };
        for (int row = 0; row < 2; ++row)
        {
            const float y = (float) rFlow.getY() + (row == 0 ? 50 : 86);
            const float xa = (float) rFlow.getX() + 12, xb = (float) rFlow.getX() + 30 + 4 * 56.0f + 14;
            g.setColour (Colours::black.withAlpha (0.8f)); g.drawLine (xa, y + 1.0f, xb, y + 1.0f, 3.0f);
            g.setColour (ui::col::brassDark.withAlpha (0.9f)); g.drawLine (xa, y, xb, y, 1.6f);
            for (int c = 0; c < 4; ++c) { const float ax = (float) rFlow.getX() + 30 + c * 56.0f + 28.0f; Path t; t.addTriangle (ax - 3, y - 3.5f, ax - 3, y + 3.5f, ax + 3.5f, y); g.setColour (ui::col::brass); g.fillPath (t); }
            for (int c = 0; c < 5; ++c)
                ui::engraved (g, lab[row * 5 + c], { (float) rFlow.getX() + 30 + c * 56.0f - 26.0f, y + 12.0f, 52.0f, 14.0f }, ui::font (11.5f, 2, 0.08f), ui::col::cream);
        }
    }
    // row B
    P_ (rFilter, A.green, 10, "FILTER", sh, 36);   P_ (rComp, A.green, 11, "COMPRESSOR", sh, 36);  P_ (rEq, A.green, 12, "TONE EQ", sh, 36);
    ui::engraved (g, "TYPE", { (float) rFilter.getX() + 214, (float) rFilter.getY() + 36.0f, 100, 14 }, ui::font (11, 2, 0.14f), ui::col::creamDim, Justification::centredLeft);
    ui::engraved (g, "ROUTE", { (float) rFilter.getX() + 160, (float) rFilter.getY() + 6.0f, 48, 18 }, ui::font (11, 2, 0.14f), ui::col::creamDim, Justification::centredRight);
    ui::engraved (g, "MODE", { (float) rComp.getX() + 302, (float) rComp.getY() + 32.0f, 90, 14 }, ui::font (11, 2, 0.14f), ui::col::creamDim, Justification::centredLeft);
    // row C
    P_ (rTape, A.brown, 13, "TAPE MACHINE", sh, 36);  P_ (rEcho, A.green, 14, "ECHO", sh, 36);  P_ (rReverb, A.brown, 15, "REVERB", sh, 36);
    ui::engraved (g, "TYPE", { (float) rEcho.getX() + 292, (float) rEcho.getY() + 44.0f, 90, 14 }, ui::font (11, 2, 0.14f), ui::col::creamDim, Justification::centredLeft);
    ui::engraved (g, "REVERB TYPE", { (float) rReverb.getX() + 14, (float) rReverb.getY() + 140.0f, 150, 14 }, ui::font (11, 2, 0.14f), ui::col::creamDim, Justification::centredLeft);
    // row D
    P_ (rMod, A.green, 16, "MODULATION", sh, 36);  P_ (rDelay, A.green, 17, "DELAY", sh, 36);
    ui::engraved (g, "TYPE", { (float) rMod.getX() + 240, (float) rMod.getY() + 32.0f, 84, 14 }, ui::font (11, 2, 0.14f), ui::col::creamDim, Justification::centredLeft);
    ui::engraved (g, "TYPE", { (float) rDelay.getX() + 288, (float) rDelay.getY() + 32.0f, 100, 14 }, ui::font (11, 2, 0.14f), ui::col::creamDim, Justification::centredLeft);
    if (sec == rp::Master) P_ (rLimSrc, A.green, 18, "LIMITER", sh, 36);
    else P_ (rLimSrc, A.brown, 19, rp::sourceInfo ((rp::Sec) sec, gMode).title, sh, 36);
    if (sec != rp::Master)
    {
        auto r = Rectangle<float> ((float) rLimSrc.getX() + 228, (float) rLimSrc.getY() + 34, 74, 88);
        ui::engraved (g, "SOURCE", r.removeFromTop (14), ui::font (11, 2, 0.14f), ui::col::creamDim, Justification::centred);
        ui::engraved (g, sec == rp::Guitar ? String (gMode == 0 ? "CHOP" : gMode == 1 ? "PICK" : "RHYTHM") : String (rp::kSecName[sec]), r.removeFromTop (30), ui::font (13, 2, 0.08f), ui::col::amber, Justification::centred);
    }

    // info bar
    {
        auto r = rInfo.toFloat();
        ui::paintPanel (g, r, A.dark, 20, {}, 0, false);
        g.setColour (Colour (0xff050403)); g.fillRoundedRectangle (r.reduced (6, 4), 3);
        auto in = r.reduced (14, 4);
        ui::engraved (g, "RIDDIM LAB  |  ECLECTIC PRODZ", in.removeFromLeft (230), ui::font (12.5f, 2, 0.12f), ui::col::brass, Justification::centredLeft);
        auto right = in.removeFromRight (300);
        ui::engraved (g, String (rp::kSecName[sec]) + "   LATENCY " + String (proc.getLatencySamples()) + " smp", right, ui::font (12.5f, 2, 0.1f), ui::col::creamDim, Justification::centredRight);
        ui::engraved (g, infoText.isEmpty() ? String ("Hover a control to see what it does.  Double-click a knob to reset it.") : infoText, in, ui::font (14.0f, 1, 0.03f),
                      infoText.isEmpty() ? ui::col::creamDim : ui::col::amber, Justification::centredLeft);
    }
}

//------------------------------------------------------------------ meters / animation
void MainUI::timerCallback()
{
    auto vu = [] (float peak) { return jlimit (0.0f, 1.04f, ui::vuPos (20.0f * std::log10 (jmax (peak, 1.0e-5f)) + 12.0f)); };
    auto glide = [] (float& n, float t) { n += (t - n) * (t > n ? 0.35f : 0.10f); return n; };
    const float oL = proc.getOutputPeak (0), oR = proc.getOutputPeak (1);
    const float inP = jmax (proc.getInputPeak (0), proc.getInputPeak (1));
    meterL.setPosition (glide (needle[0], vu (oL)));
    meterR.setPosition (glide (needle[1], vu (oR)));
    meterIn.setPosition (glide (needle[2], vu (inP)));
    const float gr = proc.getCompGainReductionDb() + proc.getLimiterGainReductionDb();
    needle[3] += (ui::grPos (gr) - needle[3]) * (ui::grPos (gr) > needle[3] ? 0.35f : 0.12f);
    meterGr.setPosition (needle[3]);
    ladder.setLevels (oL, oR);
    deck.setActivity (jmax (oL, oR) > 0.002f);
}

//------------------------------------------------------------------ presets
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
    proc.parameters.state.setProperty ("presetName", name, nullptr);
    for (size_t i = 0; i < lib.size(); ++i) lib[i]->setToggleState ((int) i == factoryIndex, dontSendNotification);
    if (factoryIndex >= 0) presetBox.setSelectedId (factoryIndex + 1, dontSendNotification);
    else if (file.existsAsFile())
    {
        const int idx = userPresets.size() ? (int) (std::find (userPresets.begin(), userPresets.end(), file) - userPresets.begin()) : -1;
        if (idx >= 0 && idx < (int) userPresets.size()) presetBox.setSelectedId (101 + idx, dontSendNotification); else presetBox.setSelectedId (0, dontSendNotification);
    }
    else presetBox.setSelectedId (0, dontSendNotification);
    if (name.isNotEmpty() && factoryIndex < 0 && ! file.existsAsFile()) presetBox.setText (name, dontSendNotification);
    presetInfo.setText (factoryIndex >= 0 ? String (factoryIndex + 1).paddedLeft ('0', 2) + " / " + String ((int) factoryPresets().size()).paddedLeft ('0', 2) + "   " + factoryPresets()[(size_t) factoryIndex].sub
                                         : (name.isEmpty() ? String ("No preset loaded") : (file.existsAsFile() ? String ("User preset") : String ("Session state"))), dontSendNotification);
}

void MainUI::applyFactoryPreset (int index)
{
    const auto& fp = factoryPresets();
    if (index < 0 || index >= (int) fp.size()) return;
    const auto s = (rp::Sec) sec;
    for (int i = 0; i < rp::NumP; ++i)
    {
        const auto p = (P) i;
        if (! rp::exists (s, p) || rp::spec (p).scope == 1 || p == P::InGain || p == P::OutGain) continue;
        setParam (s, p, rp::defaultValue (s, p));
    }
    for (auto& [p, v] : fp[(size_t) index].v) setParam (s, p, v);
    setCurrentPreset (fp[(size_t) index].name, index, {});
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

void MainUI::refreshPresetBox()
{
    presetBox.clear (dontSendNotification);
    const auto& fp = factoryPresets();
    for (int i = 0; i < (int) fp.size(); ++i) presetBox.addItem (String (i + 1).paddedLeft ('0', 2) + "   " + fp[(size_t) i].name, i + 1);
    userPresets.clear();
    auto dir = presetFolder();
    if (dir.isDirectory())
    {
        auto files = dir.findChildFiles (File::findFiles, false, "*.rlpreset");
        std::sort (files.begin(), files.end(), [] (const File& a, const File& b) { return a.getFileName().compareIgnoreCase (b.getFileName()) < 0; });
        for (auto& f : files) userPresets.push_back (f);
    }
    if (! userPresets.empty())
    {
        presetBox.addSeparator();
        presetBox.addSectionHeading ("USER PRESETS");
        for (size_t i = 0; i < userPresets.size(); ++i) presetBox.addItem (userPresets[i].getFileNameWithoutExtension(), 101 + (int) i);
    }
}

void MainUI::stepPreset (int delta)
{
    const int nf = (int) factoryPresets().size(), total = nf + (int) userPresets.size();
    int cur = currentFactory >= 0 ? currentFactory : (currentUserFile.existsAsFile() ? nf + (int) (std::find (userPresets.begin(), userPresets.end(), currentUserFile) - userPresets.begin()) : -1);
    cur = cur < 0 ? (delta > 0 ? 0 : total - 1) : (cur + delta + total) % total;
    if (cur < nf) applyFactoryPreset (cur); else applyUserPreset (userPresets[(size_t) (cur - nf)]);
}

void MainUI::showLoadMenu()
{
    refreshPresetBox();
    PopupMenu m; PopupMenu fm, um;
    const auto& fp = factoryPresets();
    for (int i = 0; i < (int) fp.size(); ++i) fm.addItem (1 + i, fp[(size_t) i].name, true, i == currentFactory);
    for (size_t i = 0; i < userPresets.size(); ++i) um.addItem (101 + (int) i, userPresets[i].getFileNameWithoutExtension());
    m.addSubMenu ("Factory", fm);
    if (! userPresets.empty()) m.addSubMenu ("User", um); else m.addItem (-1, "User (none saved yet)", false);
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (loadB), [this, safe = SafePointer<MainUI> (this)] (int r)
    {
        if (safe == nullptr || r <= 0) return;
        if (r <= (int) factoryPresets().size()) applyFactoryPreset (r - 1);
        else if (r >= 101 && r - 101 < (int) userPresets.size()) applyUserPreset (userPresets[(size_t) (r - 101)]);
    });
}

void MainUI::saveUserPreset()
{
    auto* w = new AlertWindow ("Save preset", String ("Name for this ") + rp::kSecName[sec] + " preset. It saves the settings of the current tab.", MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", currentName.isNotEmpty() && currentFactory < 0 ? currentName : String(), "Preset name");
    w->addButton ("Save", 1, KeyPress (KeyPress::returnKey));
    w->addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
    w->enterModalState (true, ModalCallbackFunction::create ([this, safe = SafePointer<MainUI> (this), w] (int result)
    {
        std::unique_ptr<AlertWindow> owner (w);
        if (safe == nullptr || result != 1) return;
        const auto name = w->getTextEditorContents ("name").trim();
        if (name.isEmpty()) return;
        auto dir = presetFolder(); dir.createDirectory();
        auto file = dir.getChildFile (File::createLegalFileName (name) + ".rlpreset");
        XmlElement xml ("RiddimPreset");
        xml.setAttribute ("name", name); xml.setAttribute ("tab", rp::kSecName[sec]); xml.setAttribute ("version", 2);
        const auto s = (rp::Sec) sec;
        for (int i = 0; i < rp::NumP; ++i)
        {
            const auto p = (P) i;
            if (! rp::exists (s, p) || rp::spec (p).scope == 1 || p == P::InGain || p == P::OutGain) continue;
            auto* e = xml.createNewChildElement ("P");
            e->setAttribute ("id", rp::spec (p).id);
            e->setAttribute ("value", (double) proc.parameters.getRawParameterValue (rp::id (s, p))->load());
        }
        if (xml.writeTo (file)) { refreshPresetBox(); setCurrentPreset (name, -1, file); }
        else AlertWindow::showMessageBoxAsync (MessageBoxIconType::WarningIcon, "Could not save", "The preset file could not be written to " + file.getFullPathName());
    }), false);
}

void MainUI::deleteCurrentPreset()
{
    if (! currentUserFile.existsAsFile())
    {
        AlertWindow::showMessageBoxAsync (MessageBoxIconType::InfoIcon, "Delete preset", "Only presets you saved yourself can be deleted. Load one of your own presets first, then press DELETE.");
        return;
    }
    const auto file = currentUserFile;
    AlertWindow::showOkCancelBox (MessageBoxIconType::QuestionIcon, "Delete preset", "Delete \"" + file.getFileNameWithoutExtension() + "\"?", "Delete", "Cancel", this,
        ModalCallbackFunction::create ([this, safe = SafePointer<MainUI> (this), file] (int r)
        {
            if (safe == nullptr || r != 1) return;
            file.deleteFile(); refreshPresetBox(); setCurrentPreset ({}, -1, {});
        }));
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
