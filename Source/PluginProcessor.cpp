#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rp;

RiddimLabAudioProcessor::RiddimLabAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
        .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    for (int s = 0; s < NumSec; ++s)
        for (int p = 0; p < NumP; ++p)
            if (exists ((Sec) s, (P) p))
                ptr[s][p] = parameters.getRawParameterValue (id ((Sec) s, (P) p));

    pBypass = parameters.getRawParameterValue ("bypass");
    pThrow = parameters.getRawParameterValue ("throw");
    pPanic = parameters.getRawParameterValue ("panic");
    pSection = parameters.getRawParameterValue ("activeSection");
    pGuitarMode = parameters.getRawParameterValue ("guitarMode");
}

RiddimLabAudioProcessor::APVTS::ParameterLayout RiddimLabAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout layout;
    using Float = juce::AudioParameterFloat;
    using Choice = juce::AudioParameterChoice;
    using Bool = juce::AudioParameterBool;

    // Global controls (same ids as before).
    layout.add (std::make_unique<Bool> ("throw", "Feedback Throw", false));
    layout.add (std::make_unique<Bool> ("panic", "Panic / Clear", false));
    layout.add (std::make_unique<Bool> ("bypass", "Effect Bypass", false));
    layout.add (std::make_unique<juce::AudioParameterInt> ("activeSection", "Active Section", 0, 6, 0));
    layout.add (std::make_unique<juce::AudioParameterInt> ("guitarMode", "Guitar Mode", 0, 2, 0));

    for (int s = 0; s < NumSec; ++s)
        for (int pi = 0; pi < NumP; ++pi)
        {
            const auto sec = (Sec) s; const auto p = (P) pi;
            if (! exists (sec, p)) continue;
            const auto& sp = spec (p);
            const auto pid = id (sec, p);
            const auto name = displayName (sec, p);
            const float def = defaultValue (sec, p);
            switch (sp.kind)
            {
                case KFloat:
                {
                    juce::NormalisableRange<float> r (sp.lo, sp.hi, sp.step);
                    r.skew = sp.skew;
                    layout.add (std::make_unique<juce::AudioParameterFloat> (pid, name, r, def));
                    break;
                }
                case KChoice:
                    layout.add (std::make_unique<juce::AudioParameterChoice> (pid, name, choiceList (p), (int) def));
                    break;
                case KBool:
                    layout.add (std::make_unique<juce::AudioParameterBool> (pid, name, def > 0.5f));
                    break;
            }
        }
    return layout;
}

float RiddimLabAudioProcessor::raw (Sec s, P p) const noexcept
{
    if (auto* a = ptr[s][p]) return a->load();
    return defaultValue (s, p);
}

void RiddimLabAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    L.assign ((size_t) kChunk, 0.0f); R.assign ((size_t) kChunk, 0.0f);
    source.prepare (sr); comp.prepare (sr); steady.prepare (sr); sub.prepare (sr); spk.prepare (sr); ctl.prepare (sr); drumVoice.prepare (sr); scLock.prepare (sr); for (auto& q : busLp) { q.lowpass (sr, 150.0, 0.707); q.reset(); } eq.prepare (sr); tape.prepare (sr);
    echo.prepare (sr, kChunk); delay.prepare (sr, kChunk); mod.prepare (sr, kChunk);
    reverb.prepare (sr); limiter.prepare (sr);
    for (int c = 0; c < 2; ++c)
    {
        inFilt[c].setIdentity(); inFilt[c].reset();
        anaLow[c].lowShelf (sr, 120.0, 0.7, 1.0); anaLow[c].reset();
        anaHigh[c].highShelf (sr, 9000.0, 0.7, -2.0); anaHigh[c].reset();
    }
    inGainS.reset (sr, 0.02); outGainS.reset (sr, 0.02);
    primed = false;
    echoWasOn = delayWasOn = modWasOn = revWasOn = true;
    setLatencySamples (tape.latencySamples());
}

bool RiddimLabAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (! (in == out && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()))) return false;
    if (layouts.inputBuses.size() > 1)
    {
        const auto sc = layouts.getChannelSet (true, 1);
        if (! sc.isDisabled() && sc != juce::AudioChannelSet::mono() && sc != juce::AudioChannelSet::stereo()) return false;
    }
    return true;
}

void RiddimLabAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = juce::jmin (2, buffer.getNumChannels());
    const int total = buffer.getNumSamples();
    if (channels == 0 || total == 0 || L.empty()) return;

    double bpm = 120.0;
    if (auto* playHead = getPlayHead())
        if (auto pos = playHead->getPosition())
            if (auto hostBpm = pos->getBpm())
                bpm = juce::jlimit (30.0, 300.0, *hostBpm);

    const int sec = juce::jlimit (0, NumSec - 1, juce::roundToInt (pSection->load()));
    const int gMode = juce::jlimit (0, 2, juce::roundToInt (pGuitarMode->load()));
    const bool bypass = pBypass->load() > 0.5f, thr = pThrow->load() > 0.5f, panic = pPanic->load() > 0.5f;

    // optional host sidechain input (second input bus)
    juce::AudioBuffer<float> scBuf;
    bool scOn = false;
    if (getBusCount (true) > 1)
        if (auto* sb = getBus (true, 1))
            if (sb->isEnabled() && sb->getNumberOfChannels() > 0) { scBuf = getBusBuffer (buffer, true, 1); scOn = scBuf.getNumChannels() > 0 && scBuf.getNumSamples() >= total; }

    float ip[2] { 0, 0 }, op[2] { 0, 0 }, cg = 0, lg = 0;
    for (int start = 0; start < total; start += kChunk)
    {
        const int n = juce::jmin (kChunk, total - start);
        for (int i = 0; i < n; ++i)
        {
            L[(size_t) i] = buffer.getSample (0, start + i);
            R[(size_t) i] = channels > 1 ? buffer.getSample (1, start + i) : L[(size_t) i];
        }
        runChunk (L.data(), R.data(), n, sec, bypass, thr, panic, bpm, gMode,
                  scOn ? scBuf.getReadPointer (0) + start : nullptr, scOn ? scBuf.getReadPointer (juce::jmin (1, scBuf.getNumChannels() - 1)) + start : nullptr);
        for (int i = 0; i < n; ++i)
        {
            buffer.setSample (0, start + i, L[(size_t) i]);
            if (channels > 1) buffer.setSample (1, start + i, R[(size_t) i]);
            op[0] = juce::jmax (op[0], std::abs (L[(size_t) i])); op[1] = juce::jmax (op[1], std::abs (R[(size_t) i]));
        }
        cg = juce::jmax (cg, compGr.load()); lg = juce::jmax (lg, limGr.load());
        ip[0] = juce::jmax (ip[0], ipk[0]); ip[1] = juce::jmax (ip[1], ipk[1]);
    }
    for (int c = 0; c < 2; ++c)
    {
        outPeak[c].store (juce::jmax (op[c], outPeak[c].load() * 0.82f), std::memory_order_relaxed);
        inPeak[c].store  (juce::jmax (ip[c], inPeak[c].load()  * 0.82f), std::memory_order_relaxed);
    }
}

void RiddimLabAudioProcessor::runChunk (float* l, float* r, int n, int sec, bool bypass, bool throwOn, bool panic, double bpm, int gMode, const float* scL, const float* scR)
{
    const Sec s = (Sec) sec;
    const auto g = [&] (P p) { return raw (s, p); };

    // --- input stage -------------------------------------------------------
    inGainS.setTargetValue (riddim::dbToGain (g (InGain)));
    if (! primed) { inGainS.setCurrentAndTargetValue (riddim::dbToGain (g (InGain))); outGainS.setCurrentAndTargetValue (riddim::dbToGain (g (OutGain))); primed = true; }
    outGainS.setTargetValue (riddim::dbToGain (g (OutGain)));
    ipk[0] = ipk[1] = 0.0f;

    if (bypass)
    {
        // Keep the signal through the tape delay line so latency stays constant.
        riddim::TapeStage::Params tp; tp.on = false;
        tape.process (l, r, n, tp);
        compGr.store (0.0f); limGr.store (0.0f);
        return;
    }

    const bool inSat = g (InSat) > 0.5f;
    for (int i = 0; i < n; ++i)
    {
        const float gn = inGainS.getNextValue();
        l[i] *= gn; r[i] *= gn;
        ipk[0] = juce::jmax (ipk[0], std::abs (l[i])); ipk[1] = juce::jmax (ipk[1], std::abs (r[i]));
        if (inSat) { l[i] = riddim::softSat (l[i], 0.45f); r[i] = riddim::softSat (r[i], 0.45f); }
    }

    // --- source voicing (instrument tabs) ----------------------------------
    if (rp::avail (s, rp::MSrc) && g (SrcOn) > 0.5f)
    {
        float k[3];
        for (int i = 0; i < 3; ++i) k[i] = g (sourceParam (s, gMode, i));
        source.process (l, r, n, s == Keys ? (int) Drums : sec, gMode, k);   // Piano Bang reuses the transient voicing
    }

    if (s == Drums) drumVoice.voice (l, r, n, juce::roundToInt (g (DrumType)));

    // --- SteadyBass (Bass tab) ----------------------------------------------
    float sbGr = 0.0f;
    if (rp::avail (s, rp::MSteady) && g (SbOn) > 0.5f)
    {
        riddim::SteadyBass::Params sp; sp.amount = g (SbAmount); sp.punch = g (SbPunch); sp.weight = g (SbWeight);
        sbGr = steady.process (l, r, n, sp);
    }
    else steady.reset();

    // --- filter on the input (route = Input) -------------------------------
    const bool filtOn = rp::avail (s, rp::MFilt) && g (FiltOn) > 0.5f;
    const int route = juce::roundToInt (g (FilterRoute));
    const int fmode = juce::jlimit (0, 2, juce::roundToInt (g (FilterMode)));
    const float cutoff = g (Cutoff), res = g (Resonance);
    const double q = 0.5 * std::pow (16.0, (res - 0.1) / 0.82);
    if (filtOn && route == 1)
    {
        for (int c = 0; c < 2; ++c)
        {
            if (fmode == 0) inFilt[c].lowpass (sr, cutoff, q);
            else if (fmode == 1) inFilt[c].highpass (sr, cutoff, q);
            else inFilt[c].bandpass (sr, cutoff, q);
        }
        for (int i = 0; i < n; ++i) { l[i] = inFilt[0].process (l[i]); r[i] = inFilt[1].process (r[i]); }
    }

    // --- compressor ---------------------------------------------------------
    float gr = 0.0f;
    if (rp::avail (s, rp::MComp) && g (CompOn) > 0.5f)
    {
        riddim::Compressor::Params cp;
        cp.threshDb = g (CompThresh); cp.ratio = g (CompRatio); cp.makeupDb = g (CompMakeup); cp.mode = juce::roundToInt (g (CompMode));
        gr = comp.process (l, r, n, cp);
    }
    else comp.reset();
    compGr.store (juce::jmax (gr, sbGr));

    // --- tone EQ ------------------------------------------------------------
    if (g (EqOn) > 0.5f) { eq.set (g (EqLow), g (EqMid), g (EqHigh), g (EqPres)); eq.process (l, r, n); }

    if (s == Drums)
    {
        const int lockMode = juce::roundToInt (g (LockOn));
        if (lockMode == 1) drumVoice.lock (l, r, n, g (LockAmt));
        else if (lockMode == 2)
        {
            if (scL != nullptr) { scState.store (1); scLock.process (l, r, n, g (LockAmt), scL, scR, 0.0f); }
            else if (riddim::BassBus::fresh()) { scState.store (2); scLock.process (l, r, n, g (LockAmt), nullptr, nullptr, riddim::BassBus::level().load()); }
            else { scState.store (0); scLock.reset(); }
        }
        else scLock.reset();
    }
    else if (s == Bass)
    {
        // let a Drums tab in this host hear what the bass is doing
        float pk = 0.0f;
        for (int i = 0; i < n; ++i) pk = juce::jmax (pk, std::abs (busLp[1].process (busLp[0].process (0.5f * (l[i] + r[i])))));
        riddim::BassBus::level().store (pk); riddim::BassBus::stamp().store (juce::Time::getMillisecondCounter());
    }

    // --- sub band (Bass tab) ---------------------------------------------------
    if (s == Bass)
    {
        riddim::SubStage::Params sb; sb.levelDb = g (SubLevel); sb.harm = g (SubHarm);
        sub.process (l, r, n, sb);
        subLvl.store (sub.getLevel());
    }

    // --- tape (always in the path so that reported latency is constant) -----
    {
        riddim::TapeStage::Params tp;
        tp.on = g (TapeOn) > 0.5f; tp.sat = g (TapeSat); tp.tone = g (TapeTone);
        tp.wow = g (Wow); tp.flutter = g (Flutter); tp.wear = g (Wear); tp.type = juce::roundToInt (g (TapeType));
        tp.speed = juce::jlimit (0, 2, juce::roundToInt (g (TapeSpeed))); tp.stab = g (TapeStab); tp.mix = g (TapeMix);
        tape.process (l, r, n, tp);
    }

    // --- echo ----------------------------------------------------------------
    const bool echoOn = g (EchoOn) > 0.5f;
    if (echoOn)
    {
        static const double beats[] = { 1.0, 0.5, 0.75, 1.0 / 3.0, 0.25, 1.5 };
        const double beatMs = 60000.0 / bpm;
        const double ms = g (Sync) > 0.5f ? beatMs * beats[juce::jlimit (0, 5, juce::roundToInt (g (Division)))] : (double) g (rp::EchoTime);
        const float base = (float) (ms * 0.001 * sr);
        riddim::DubEcho::Params ep;
        ep.delaySamples0 = base;
        ep.delaySamples1 = base * (1.0f + g (Width) * 0.5f);
        ep.feedback = throwOn ? juce::jmin (0.94f, g (Feedback) + 0.22f) : g (Feedback);
        ep.mix = g (Mix); ep.heat = g (Heat); ep.damping = g (Damping);
        ep.type = juce::roundToInt (g (EchoType));
        ep.heads[0] = g (Head1) > 0.5f; ep.heads[1] = g (Head2) > 0.5f; ep.heads[2] = g (Head3) > 0.5f;
        if (! (ep.heads[0] || ep.heads[1] || ep.heads[2])) ep.heads[0] = true;
        ep.revLoop = g (EchoRev);
        ep.panic = panic;
        if (filtOn && route == 0) { ep.cutoff = cutoff; ep.resonance = res; ep.filterMode = fmode; }
        else { ep.cutoff = 18000.0f; ep.resonance = 0.1f; ep.filterMode = 0; }
        echo.process (l, r, n, ep);
    }
    else if (echoWasOn) echo.reset();
    echoWasOn = echoOn;

    // --- rhythm delay --------------------------------------------------------
    const bool dlyOn = false;   // second delay removed from the interface
    if (dlyOn)
    {
        static const double dbeats[] = { 0.25, 0.5, 1.0, 1.5, 2.0, 3.0, 4.0 };
        riddim::RhythmDelay::Params dp;
        dp.delaySamples = (float) (60.0 / bpm * dbeats[juce::jlimit (0, 6, juce::roundToInt (g (DlyDiv)))] * sr);
        dp.feedback = g (DlyFeedback); dp.mix = g (DlyMix); dp.type = juce::roundToInt (g (DlyType));
        delay.process (l, r, n, dp);
    }
    else if (delayWasOn) delay.reset();
    delayWasOn = dlyOn;

    // --- modulation ----------------------------------------------------------
    const bool modOn = false;   // modulation removed from the interface
    if (modOn)
    {
        riddim::ModStage::Params mp;
        mp.type = juce::roundToInt (g (ModType)); mp.rate = g (ModRate); mp.depth = g (ModDepth); mp.mix = g (ModMix);
        mod.process (l, r, n, mp);
    }
    else if (modWasOn) mod.reset();
    modWasOn = modOn;

    // --- reverb --------------------------------------------------------------
    const bool revOn = g (RevOn) > 0.5f;
    if (revOn)
    {
        riddim::ReverbStage::Params rv;
        rv.type = juce::roundToInt (g (RevType)); rv.preMs = g (RevPre); rv.decay = g (RevDecay); rv.mix = g (RevMix);
        rv.spring.character = juce::roundToInt (g (SpChar)); rv.spring.length = juce::roundToInt (g (SpLen)); rv.spring.drive = g (SpDrive);
        rv.spring.fb = g (SpFb); rv.spring.treble = g (SpTreble); rv.spring.bass = g (SpBass); rv.spring.mono = juce::roundToInt (g (SpStereo)) == 1;
        rv.plate.damp = g (PlDamp); rv.plate.lowCut = g (PlLowCut); rv.plate.width = g (PlWidth); rv.plate.tone = juce::roundToInt (g (PlTone));
        reverb.process (l, r, n, rv);
    }
    else if (revWasOn) reverb.reset();
    revWasOn = revOn;

    // --- analog tone + output gain ------------------------------------------
    const bool analog = g (Analog) > 0.5f;
    for (int i = 0; i < n; ++i)
    {
        float a = l[i], b = r[i];
        if (analog)
        {
            a = riddim::softSat (anaHigh[0].process (anaLow[0].process (a)), 0.12f);
            b = riddim::softSat (anaHigh[1].process (anaLow[1].process (b)), 0.12f);
        }
        const float og = outGainS.getNextValue();
        l[i] = a * og; r[i] = b * og;
    }

    // --- master limiter (always the Master tab's settings: final safety stage)
    float lgr = 0.0f;
    if (raw (Master, LimOn) > 0.5f)
    {
        riddim::Limiter::Params lp;
        lp.threshDb = raw (Master, LimThresh); lp.releaseMs = raw (Master, LimRelease);
        lp.ceilingDb = raw (Master, LimCeiling); lp.soft = raw (Master, LimSoft) > 0.5f;
        lgr = limiter.process (l, r, n, lp);
    }
    limGr.store (lgr);

    // --- speaker check: listening aid only, never applied to an offline bounce
    if (s == Bass && ! isNonRealtime()) spk.process (l, r, n, juce::roundToInt (g (SpkMode)), g (SpkLevel));

    // --- control-room listening (global settings live on the Master profile); never applied to an offline bounce
    if (! isNonRealtime())
    {
        riddim::ControlRoom::Params cp;
        cp.mon = juce::roundToInt (raw (Master, CtlMon)); cp.room = juce::roundToInt (raw (Master, CtlRoom));
        cp.angle = raw (Master, CtlWidth); cp.dist = raw (Master, CtlDist); cp.toe = raw (Master, CtlToe); cp.colour = raw (Master, CtlColour);
        cp.levelDb = raw (Master, CtlLevel); cp.mono = raw (Master, CtlMono) > 0.5f; cp.phones = raw (Master, CtlPhones) > 0.5f;
        ctl.process (l, r, n, cp);
    }
}

juce::AudioProcessorEditor* RiddimLabAudioProcessor::createEditor()
{
    return new RiddimLabAudioProcessorEditor (*this);
}

void RiddimLabAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void RiddimLabAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RiddimLabAudioProcessor();
}
