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
            { "VOCAL TOOLS", "Subtle vocal echo, plate verb",
              { { P::Feedback, 0.32f }, { P::Mix, 0.22f }, { P::Cutoff, 2400 }, { P::Resonance, 0.22f }, { P::Heat, 0.12f }, { P::Division, 0 },
                { P::TapeSat, 0.10f }, { P::TapeTone, 0.75f }, { P::Wow, 0.02f }, { P::Flutter, 0.02f }, { P::Wear, 0.03f },
                { P::Width, 0.20f }, { P::Damping, 0.55f }, { P::RevType, 1 }, { P::RevMix, 0.18f }, { P::RevDecay, 0.45f },
                { P::CompOn, 1 }, { P::CompThresh, -20 }, { P::CompRatio, 3 }, { P::CompMakeup, 3 }, { P::EqHigh, 1.5f } } },
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
    const char* names[7] { "MASTER", "DRUMS", "GUITAR", "BASS", "PIANO BANG", "HORNS", "VOCAL" };
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
    addKnob (K::SbAmount, "INTENSITY", 68, 0); addKnob (K::SbPunch, "PUNCH", 58, 0); addKnob (K::SbWeight, "WEIGHT", 58, 0);
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
    pFilt = pw(); pComp = pw(); pEq = pw(); pTape = pw(); pEcho = pw(); pRev = pw(); pLim = pw(); pSrc = pw(); pSteady = pw();
    for (int i = 0; i < 9; ++i) flow[i] = addLamp (std::make_unique<ui::Lamp> (ui::Lamp::LampOnly, "", 7.0f));
    lSat->onInfo = lAnalog->onInfo = lSync->onInfo = lSoft->onInfo = lBypass->onInfo = lThrow->onInfo = lPanic->onInfo = info;

    cFilterMode  = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "LOW-PASS", "HIGH-PASS", "BAND-PASS" }, true, 12.5f));
    cFilterRoute = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "ECHO", "INPUT" }, false, 12.0f));
    cCompMode    = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "GENTLE", "PUNCH", "LIMIT" }, true, 12.5f));
    cTapeType    = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "CLEAN", "VINTAGE", "WORN" }, false, 12.0f));
    cEchoType    = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "TAPE", "DIGITAL", "PING-PONG" }, true, 12.5f));
    cRevType     = addChoice (std::make_unique<ui::ChoiceLamps> (StringArray { "SPRING", "PLATE" }, false, 14.0f));

    // Piano Bang voicing buttons
    for (auto* b : { &oldSchoolB, &modernB })
    {
        b->getProperties().set ("role", "brass"); b->setClickingTogglesState (false); addAndMakeVisible (*b);
    }
    oldSchoolB.setButtonText ("OLD SCHOOL"); oldSchoolB.onClick = [this] { applyPianoVoicing (0); };
    modernB.setButtonText ("MODERN");        modernB.onClick = [this] { applyPianoVoicing (1); };
    oldSchoolB.setTooltip ("Upright piano on tape: warm, dark, a little wobble");
    modernB.setTooltip ("Clean, bright, hammer-forward digital stage piano");

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
    const bool hasSrc = rp::avail (s, rp::MSrc), hasSteady = rp::avail (s, rp::MSteady), hasFilt = rp::avail (s, rp::MFilt),
               hasComp = rp::avail (s, rp::MComp), isMaster = (s == rp::Master);
    const auto shown = [&] (P p)
    {
        switch (p)
        {
            case P::Cutoff: case P::Resonance: return hasFilt;
            case P::CompThresh: case P::CompRatio: case P::CompMakeup: return hasComp;
            case P::LimThresh: case P::LimRelease: case P::LimCeiling: return isMaster;
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
            // source knobs: i-th source knob of the current tab/mode
            const int i = srcIdx++;
            p = rp::sourceParam (s, gMode, i);
            kd->k->setVisible (hasSrc);
            if (! hasSrc) { kd->k->bind (proc.parameters, {}, 0, 0.0); continue; }
            kd->k->setCaption (String (src.names[i]));
            kd->k->help = srcHelp (s, gMode, i);
            kd->k->bind (proc.parameters, idOf (p), 0, rp::defaultValue (s, p));
            continue;
        }
        const bool vis = shown (p);
        kd->k->setVisible (vis);
        kd->k->bind (proc.parameters, vis ? idOf (p) : String(), kd->fmt, rp::defaultValue (s, p));
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

    // signal flow: only the modules this tab really has
    flowItems.clear();
    if (hasSrc)    flowItems.push_back ({ "SOURCE", P::SrcOn, false });
    if (hasSteady) flowItems.push_back ({ "STEADY", P::SbOn, false });
    if (hasFilt)   flowItems.push_back ({ "FILTER", P::FiltOn, false });
    if (hasComp)   flowItems.push_back ({ "COMP", P::CompOn, false });
    flowItems.push_back ({ "EQ", P::EqOn, false }); flowItems.push_back ({ "TAPE", P::TapeOn, false });
    flowItems.push_back ({ "ECHO", P::EchoOn, false }); flowItems.push_back ({ "REVERB", P::RevOn, false });
    if (isMaster)  flowItems.push_back ({ "LIMITER", P::LimOn, true });
    for (int i = 0; i < 9; ++i)
    {
        const bool used = i < (int) flowItems.size();
        flow[i]->setVisible (used);
        flow[i]->bindParam (proc.parameters, used ? (flowItems[(size_t) i].master ? rp::id (rp::Master, flowItems[(size_t) i].param) : idOf (flowItems[(size_t) i].param)) : String());
    }

    syncAtt.reset();
    syncAtt = std::make_unique<ParameterAttachment> (*proc.parameters.getParameter (rp::id (s, P::Sync)), [this] (float v) { if ((v > 0.5f) != syncOn) updateEchoTimeBinding(); });
    layoutControls();
}

//------------------------------------------------------------------ layout
void MainUI::place (Component& c, float cx, int top, int w, int h) { c.setBounds (roundToInt (cx - w * 0.5f), top, w, h); }

String MainUI::tabTitle (int section) { return section == rp::Keys ? String ("PIANO BANG") : String (rp::kSecName[section]); }

void MainUI::resized()
{
    rHeaderArt = { 16, 12, 568, 138 };
    rVuPanel = { 592, 12, 408, 138 };
    rPreset  = { 1008, 12, 258, 138 };
    rLibrary = { 14, 156, 192, 692 };
    rInfo    = { 14, 856, 1252, 28 };
    const int x0 = 214;
    rInput  = { x0, 208, 260, 146 };       rMeters = { x0 + 268, 208, 516, 146 };    rOutput = { x0 + 792, 208, 260, 146 };
    rTape   = { x0, 500, 346, 210 };       rEcho = { x0 + 354, 500, 392, 210 };      rReverb = { x0 + 754, 500, 298, 210 };
    rFlow   = { x0, 718, 1052, 130 };

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
    layoutControls();
}

// Everything that depends on which tab is showing.
void MainUI::layoutControls()
{
    const auto s = (rp::Sec) sec;
    const bool hasSrc = rp::avail (s, rp::MSrc), hasSteady = rp::avail (s, rp::MSteady), hasFilt = rp::avail (s, rp::MFilt),
               hasComp = rp::avail (s, rp::MComp), isMaster = (s == rp::Master), isKeys = (s == rp::Keys);

    auto knobAt = [&] (P p, const Rectangle<int>& panel, float cx, int top)
    {
        for (auto& kd : knobs)
            if (kd->p == p && ! (p >= P::Src1 && p <= P::Src9)) { place (*kd->k, panel.getX() + cx, panel.getY() + top, kd->k->preferredWidth(), kd->k->preferredHeight()); return; }
    };
    auto lampAt = [&] (Component* c, const Rectangle<int>& panel, int x, int y, int w, int h) { c->setBounds (panel.getX() + x, panel.getY() + y, w, h); };
    auto power  = [&] (ui::Lamp* l, const Rectangle<int>& panel) { l->setBounds (panel.getX() + 14, panel.getY() + 4, 20, 20); };

    // Row A
    knobAt (P::InGain, rInput, 130, 28);   lampAt (lSat, rInput, 74, 126, 112, 18);
    knobAt (P::OutGain, rOutput, 130, 28); lampAt (lAnalog, rOutput, 64, 126, 130, 18);
    meterIn.setBounds (rMeters.getX() + 24, rMeters.getY() + 6, 220, 122);
    meterGr.setBounds (rMeters.getRight() - 244, rMeters.getY() + 6, 220, 122);

    // Row B: only the panels this tab has, widths shared out, contents centred in each
    rowB.clear();
    if (hasSteady) rowB.push_back ({ BKind::Steady, {} });
    if (hasSrc)    rowB.push_back ({ BKind::Src, {} });
    if (hasFilt)   rowB.push_back ({ BKind::Filter, {} });
    if (hasComp)   rowB.push_back ({ BKind::Comp, {} });
    rowB.push_back ({ BKind::Eq, {} });
    if (isMaster)  rowB.push_back ({ BKind::Limiter, {} });
    auto natural = [&] (BKind k) { switch (k) { case BKind::Src: return isKeys ? 430 : 270; case BKind::Steady: return 440; case BKind::Filter: return 328; case BKind::Comp: return 400; case BKind::Eq: return 308; default: return 308; } };
    {
        const int x0 = 214, total = 1052, gap = 8, n = (int) rowB.size();
        int sum = 0; for (auto& b : rowB) sum += natural (b.kind);
        const int extra = (total - gap * (n - 1) - sum) / n;
        int x = x0;
        for (size_t i = 0; i < rowB.size(); ++i)
        {
            const int w = (i + 1 == rowB.size()) ? (x0 + total - x) : natural (rowB[i].kind) + extra;
            rowB[i].r = { x, 362, w, 130 }; x += w + gap;
        }
    }
    // hide everything of row B first, then place what is used
    for (auto* c : std::initializer_list<Component*> { pFilt, pComp, pEq, pLim, pSrc, pSteady, cFilterMode, cFilterRoute, cCompMode, lSoft, &oldSchoolB, &modernB }) c->setVisible (false);
    for (auto* k : limKnobs) k->setVisible (false);
    for (auto* k : srcKnobs) k->setVisible (false);
    for (auto& kd : knobs) if (kd->p == P::SbAmount || kd->p == P::SbPunch || kd->p == P::SbWeight) kd->k->setVisible (false);

    for (auto& b : rowB)
    {
        const auto& R = b.r;
        const auto C = [&] (int cw) { return R.withSizeKeepingCentre (cw, R.getHeight()); };
        switch (b.kind)
        {
            case BKind::Filter:
            {
                const auto c = C (328); pFilt->setVisible (true); power (pFilt, R);
                knobAt (P::Cutoff, c, 54, 30);  knobAt (P::Resonance, c, 142, 30);
                lampAt (cFilterMode, c, 214, 52, 106, 51); lampAt (cFilterRoute, c, 214, 5, 108, 20);
                cFilterMode->setVisible (true); cFilterRoute->setVisible (true);
                break;
            }
            case BKind::Comp:
            {
                const auto c = C (400); pComp->setVisible (true); power (pComp, R);
                knobAt (P::CompThresh, c, 54, 30); knobAt (P::CompRatio, c, 142, 30); knobAt (P::CompMakeup, c, 230, 30);
                lampAt (cCompMode, c, 302, 46, 90, 51); cCompMode->setVisible (true);
                break;
            }
            case BKind::Eq:
            {
                const auto c = C (308); pEq->setVisible (true); power (pEq, R);
                const float cxs[4] { 42, 116, 190, 264 }; const P ps[4] { P::EqLow, P::EqMid, P::EqHigh, P::EqPres };
                for (int i = 0; i < 4; ++i) knobAt (ps[i], c, cxs[i] + 2, 32);
                break;
            }
            case BKind::Limiter:
            {
                const auto c = C (308); pLim->setVisible (true); power (pLim, R); lSoft->setVisible (true);
                const float cxs[3] { 46, 120, 194 };
                for (int i = 0; i < 3; ++i) { place (*limKnobs[(size_t) i], c.getX() + cxs[i], c.getY() + 30, limKnobs[(size_t) i]->preferredWidth(), limKnobs[(size_t) i]->preferredHeight()); limKnobs[(size_t) i]->setVisible (true); }
                lampAt (lSoft, c, 232, 54, 70, 28);
                break;
            }
            case BKind::Src:
            {
                const int cw = isKeys ? 430 : 270; const auto c = C (cw); pSrc->setVisible (true); power (pSrc, R);
                const float cxs[3] { 46, 136, 226 };
                for (int i = 0; i < 3; ++i) { place (*srcKnobs[(size_t) i], c.getX() + cxs[i], c.getY() + 30, srcKnobs[(size_t) i]->preferredWidth(), srcKnobs[(size_t) i]->preferredHeight()); srcKnobs[(size_t) i]->setVisible (true); }
                if (isKeys)
                {
                    lampAt (&oldSchoolB, c, 292, 50, 130, 30); lampAt (&modernB, c, 292, 86, 130, 30);
                    oldSchoolB.setVisible (true); modernB.setVisible (true);
                }
                break;
            }
            case BKind::Steady:
            {
                const auto c = C (440); pSteady->setVisible (true); power (pSteady, R);
                knobAt (P::SbAmount, c, 56, 26); knobAt (P::SbPunch, c, 142, 32); knobAt (P::SbWeight, c, 216, 32);
                for (auto& kd : knobs) if (kd->p == P::SbAmount || kd->p == P::SbPunch || kd->p == P::SbWeight) kd->k->setVisible (true);
                break;
            }
        }
    }

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
    lampAt (cRevType, rReverb, 70, 160, 190, 24);

    // Row D: signal flow strip
    {
        const int n = (int) flowItems.size();
        const float xa = (float) rFlow.getX() + 20.0f, first = xa + 74.0f, span = 560.0f;
        const float pitch = n > 1 ? span / (float) (n - 1) : 0.0f;
        for (int i = 0; i < 9; ++i)
        {
            const float cx = first + pitch * (float) i;
            flow[i]->setBounds (roundToInt (cx) - 12, rFlow.getY() + 54 - 12, 24, 24);
        }
        const int by = rFlow.getY() + 46, bw = 96;
        lBypass->setBounds (rFlow.getX() + 748, by, bw, 34); lThrow->setBounds (rFlow.getX() + 748 + bw + 8, by, bw, 34); lPanic->setBounds (rFlow.getX() + 748 + 2 * (bw + 8), by, bw, 34);
    }
    repaint();
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
    const auto dim = ui::col::creamDim;
    auto small = [&] (const String& t, Rectangle<float> r, Justification j = Justification::centredLeft) { ui::engraved (g, t, r, ui::font (11, 2, 0.14f), dim, j); };

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
    ui::engraved (g, "INPUT LEVEL", { (float) rMeters.getX() + 24, (float) rMeters.getBottom() - 17.0f, 220, 14 }, ui::font (12.5f, 2, 0.12f), ui::col::cream);
    ui::engraved (g, sec == rp::Bass ? "GAIN REDUCTION  /  STEADYBASS" : "GAIN REDUCTION", { (float) rMeters.getRight() - 244, (float) rMeters.getBottom() - 17.0f, 220, 14 }, ui::font (12.5f, 2, 0.12f), ui::col::cream);

    // row B
    for (auto& b : rowB)
    {
        const auto& R = b.r;
        const auto C = [&] (int cw) { return R.withSizeKeepingCentre (cw, R.getHeight()); };
        switch (b.kind)
        {
            case BKind::Filter:
            {
                const auto c = C (328); P_ (R, A.green, 10, "FILTER", sh, 36);
                small ("TYPE", { (float) c.getX() + 214, (float) c.getY() + 36.0f, 100, 14 });
                small ("ROUTE", { (float) c.getX() + 160, (float) c.getY() + 6.0f, 48, 18 }, Justification::centredRight);
                break;
            }
            case BKind::Comp:
            {
                const auto c = C (400); P_ (R, A.green, 11, "COMPRESSOR", sh, 36);
                small ("MODE", { (float) c.getX() + 302, (float) c.getY() + 32.0f, 90, 14 });
                break;
            }
            case BKind::Eq: P_ (R, A.green, 12, sec == rp::Keys ? "TONE EQ  /  PIANO VOICING" : "TONE EQ", sh, 36); break;
            case BKind::Limiter: P_ (R, A.green, 18, "LIMITER", sh, 36); break;
            case BKind::Src:
            {
                P_ (R, A.brown, 19, rp::sourceInfo ((rp::Sec) sec, gMode).title, sh, 36);
                if (sec == rp::Keys)
                {
                    const auto c = C (430);
                    small ("VOICING", { (float) c.getX() + 292, (float) c.getY() + 32.0f, 130, 14 }, Justification::centred);
                    small ("EQ + TAPE PRESET", { (float) c.getX() + 280, (float) c.getY() + 118.0f, 154, 12 }, Justification::centred);
                }
                break;
            }
            case BKind::Steady:
            {
                const auto c = C (440); P_ (R, A.brown, 21, "STEADYBASS  /  WEIGHT-KEEPING LEVELLER", sh, 36);
                const char* lines[4] { "EVENS OUT NOTE LEVELS", "LOW END STAYS FULL", "ATTACK COMES THROUGH", "NO LATENCY" };
                for (int i = 0; i < 4; ++i)
                    ui::engraved (g, lines[i], { (float) c.getX() + 268.0f, (float) c.getY() + 40.0f + 20.0f * i, 172.0f, 16.0f }, ui::font (11.5f, 2, 0.1f), i == 0 ? ui::col::amber : dim, Justification::centredLeft);
                break;
            }
        }
    }
    // row C
    P_ (rTape, A.brown, 13, "TAPE MACHINE", sh, 36);  P_ (rEcho, A.green, 14, "DUB ECHO", sh, 36);  P_ (rReverb, A.brown, 15, "REVERB", sh, 36);
    small ("TYPE", { (float) rEcho.getX() + 292, (float) rEcho.getY() + 44.0f, 90, 14 });
    small ("SPRING  OR  PLATE", { (float) rReverb.getX() + 14, (float) rReverb.getY() + 140.0f, 270, 14 }, Justification::centred);

    // row D: signal flow
    {
        P_ (rFlow, A.green2, 9, "SIGNAL FLOW", sh);
        const float y = (float) rFlow.getY() + 54.0f;
        const float xa = (float) rFlow.getX() + 20.0f, xb = xa + 74.0f + 560.0f + 62.0f;
        g.setColour (Colours::black.withAlpha (0.8f)); g.drawLine (xa + 26, y + 1.0f, xb - 26, y + 1.0f, 3.0f);
        g.setColour (ui::col::brassDark.withAlpha (0.9f)); g.drawLine (xa + 26, y, xb - 26, y, 1.6f);
        ui::engraved (g, "IN", { xa - 6, y - 8.0f, 30, 16 }, ui::font (12.5f, 2, 0.1f), ui::col::brass, Justification::centred);
        ui::engraved (g, "OUT", { xb - 28, y - 8.0f, 36, 16 }, ui::font (12.5f, 2, 0.1f), ui::col::brass, Justification::centred);
        const int n = (int) flowItems.size();
        const float first = xa + 74.0f, pitch = n > 1 ? 560.0f / (float) (n - 1) : 0.0f;
        for (int i = 0; i < n; ++i)
        {
            const float cx = first + pitch * (float) i;
            ui::engraved (g, flowItems[(size_t) i].label, { cx - 40.0f, y + 16.0f, 80.0f, 16.0f }, ui::font (12.5f, 2, 0.08f), ui::col::cream);
            if (i + 1 < n)
            {
                const float ax = cx + pitch * 0.5f; Path t; t.addTriangle (ax - 3, y - 3.5f, ax - 3, y + 3.5f, ax + 3.5f, y); g.setColour (ui::col::brass); g.fillPath (t);
            }
        }
        ui::engraved (g, "Lamps switch each stage on and off.  The master limiter always guards the output.", { (float) rFlow.getX() + 20.0f, (float) rFlow.getBottom() - 38.0f, 700.0f, 16.0f },
                      ui::font (11.5f, 1, 0.03f), dim, Justification::centredLeft);
    }

    // info bar
    {
        auto r = rInfo.toFloat();
        ui::paintPanel (g, r, A.dark, 20, {}, 0, false);
        g.setColour (Colour (0xff050403)); g.fillRoundedRectangle (r.reduced (6, 4), 3);
        auto in = r.reduced (14, 4);
        ui::engraved (g, "RIDDIM LAB  |  ECLECTIC PRODZ", in.removeFromLeft (230), ui::font (12.5f, 2, 0.12f), ui::col::brass, Justification::centredLeft);
        auto right = in.removeFromRight (300);
        ui::engraved (g, tabTitle (sec) + "   LATENCY " + String (proc.getLatencySamples()) + " smp", right, ui::font (12.5f, 2, 0.1f), ui::col::creamDim, Justification::centredRight);
        ui::engraved (g, infoText.isEmpty() ? String ("Hover a control to see what it does.  Double-click a knob to reset it.") : infoText, in, ui::font (14.0f, 1, 0.03f),
                      infoText.isEmpty() ? ui::col::creamDim : ui::col::amber, Justification::centredLeft);
    }
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
    if (sec == rp::Keys) { oldSchoolB.setToggleState (pianoVoicingMatches (0), dontSendNotification); modernB.setToggleState (pianoVoicingMatches (1), dontSendNotification); }
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
    auto* w = new AlertWindow ("Save preset", String ("Name for this ") + tabTitle (sec) + " preset. It saves the settings of the current tab.", MessageBoxIconType::NoIcon, this);
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
