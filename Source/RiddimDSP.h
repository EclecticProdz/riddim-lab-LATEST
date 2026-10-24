#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>

// =============================================================================
//  Riddim Lab DSP building blocks.
//  Every module here is plain C++ with no plugin or UI dependencies, so each
//  one can be tested offline on test signals.
// =============================================================================
namespace riddim
{
constexpr float kPi = 3.14159265358979323846f;

inline float dbToGain (float db) noexcept   { return std::exp (db * 0.11512925f); }
inline float gainToDb (float g) noexcept    { return 20.0f * std::log10 (juce::jmax (g, 1.0e-9f)); }
inline float onePoleCoef (float seconds, double sr) noexcept
{
    return seconds <= 0.0f ? 1.0f : 1.0f - std::exp (-1.0f / (seconds * (float) sr));
}

// Soft saturation with UNITY gain for small signals (so turning "drive" up adds
// colour and peak compression instead of adding level).
inline float softSat (float x, float amount) noexcept
{
    const float a = 1.0f + amount * 7.0f;
    return std::tanh (x * a) / a;
}

//==============================================================================
// RBJ biquad (allocation free, safe to retune on the audio thread)
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    void reset() noexcept { z1 = z2 = 0; }
    float process (float x) noexcept
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void set (double nb0, double nb1, double nb2, double na0, double na1, double na2) noexcept
    {
        b0 = (float) (nb0 / na0); b1 = (float) (nb1 / na0); b2 = (float) (nb2 / na0);
        a1 = (float) (na1 / na0); a2 = (float) (na2 / na0);
    }
    void setIdentity() noexcept { b0 = 1; b1 = b2 = a1 = a2 = 0; }

    void lowShelf (double fs, double f, double q, double gainDb) noexcept
    {
        const double A = std::pow (10.0, gainDb / 40.0), w = 2.0 * kPi * f / fs, c = std::cos (w), s = std::sin (w);
        const double al = s / (2.0 * q), sA = 2.0 * std::sqrt (A) * al;
        set (A * ((A + 1) - (A - 1) * c + sA), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sA),
             (A + 1) + (A - 1) * c + sA, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sA);
    }
    void highShelf (double fs, double f, double q, double gainDb) noexcept
    {
        const double A = std::pow (10.0, gainDb / 40.0), w = 2.0 * kPi * f / fs, c = std::cos (w), s = std::sin (w);
        const double al = s / (2.0 * q), sA = 2.0 * std::sqrt (A) * al;
        set (A * ((A + 1) + (A - 1) * c + sA), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sA),
             (A + 1) - (A - 1) * c + sA, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sA);
    }
    void peak (double fs, double f, double q, double gainDb) noexcept
    {
        const double A = std::pow (10.0, gainDb / 40.0), w = 2.0 * kPi * f / fs, c = std::cos (w), al = std::sin (w) / (2.0 * q);
        set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void lowpass (double fs, double f, double q) noexcept
    {
        const double w = 2.0 * kPi * f / fs, c = std::cos (w), al = std::sin (w) / (2.0 * q);
        set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void highpass (double fs, double f, double q) noexcept
    {
        const double w = 2.0 * kPi * f / fs, c = std::cos (w), al = std::sin (w) / (2.0 * q);
        set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void bandpass (double fs, double f, double q) noexcept
    {
        const double w = 2.0 * kPi * f / fs, c = std::cos (w), al = std::sin (w) / (2.0 * q);
        set (al, 0, -al, 1 + al, -2 * c, 1 - al);
    }
};

// Peak follower with separate attack / release (seconds)
struct Follower
{
    float v = 0, a = 1, r = 1;
    void setTimes (float attack, float release, double sr) noexcept { a = onePoleCoef (attack, sr); r = onePoleCoef (release, sr); }
    float process (float x) noexcept { v += (x > v ? a : r) * (x - v); return v; }
    void reset() noexcept { v = 0; }
};

//==============================================================================
//  COMPRESSOR  (stereo-linked, soft knee, feed-forward, dB-domain smoothing)
//==============================================================================
class Compressor
{
public:
    struct Params { float threshDb = 0.0f, ratio = 1.0f, makeupDb = 0.0f; int mode = 0; };

    void prepare (double sampleRate) { sr = sampleRate; reset(); }
    void reset() { grDb = 0.0f; lastGr = 0.0f; }

    // returns the largest gain reduction (positive dB) seen in this block
    float process (float* l, float* r, int n, const Params& p)
    {
        float attack = 0.020f, release = 0.250f, knee = 8.0f, ratio = p.ratio;
        if (p.mode == 1)      { attack = 0.030f;  release = 0.090f; knee = 3.0f; }       // Punch
        else if (p.mode == 2) { attack = 0.0004f; release = 0.060f; knee = 0.0f; ratio = juce::jmax (ratio, 20.0f); } // Limit
        const float ca = onePoleCoef (attack, sr), cr = onePoleCoef (release, sr);
        const float slope = 1.0f / juce::jmax (1.0f, ratio) - 1.0f;
        const float makeup = dbToGain (p.makeupDb);
        float maxGr = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            const float level = gainToDb (juce::jmax (std::abs (l[i]), std::abs (r[i])));
            const float over = level - p.threshDb;
            float target;
            if (knee > 0.0f && 2.0f * over > -knee && 2.0f * over < knee)
            {
                const float t = over + knee * 0.5f;
                target = slope * t * t / (2.0f * knee);
            }
            else
                target = over > 0.0f ? slope * over : 0.0f;
            grDb += (target < grDb ? ca : cr) * (target - grDb);
            const float g = dbToGain (grDb) * makeup;
            l[i] *= g; r[i] *= g;
            maxGr = juce::jmax (maxGr, -grDb);
        }
        lastGr = maxGr;
        return maxGr;
    }
    float lastGainReduction() const noexcept { return lastGr; }

private:
    double sr = 48000.0; float grDb = 0.0f, lastGr = 0.0f;
};

//==============================================================================
//  TONE EQ  (low shelf / mid bell / high shelf / presence bell)
//==============================================================================
class ToneEQ
{
public:
    void prepare (double sampleRate) { sr = sampleRate; reset(); cached[0] = cached[1] = cached[2] = cached[3] = 1000.0f; }
    void reset() { for (auto& c : ch) for (auto& b : c) b.reset(); }

    void set (float lowDb, float midDb, float highDb, float presDb)
    {
        const float v[4] { lowDb, midDb, highDb, presDb };
        bool changed = false;
        for (int i = 0; i < 4; ++i) if (std::abs (v[i] - cached[i]) > 1.0e-4f) { cached[i] = v[i]; changed = true; }
        if (! changed) return;
        for (auto& c : ch)
        {
            c[0].lowShelf (sr, 100.0, 0.707, lowDb);
            c[1].peak (sr, 800.0, 0.8, midDb);
            c[2].highShelf (sr, 7000.0, 0.707, highDb);
            c[3].peak (sr, 3500.0, 0.9, presDb);
        }
    }
    void process (float* l, float* r, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            float a = l[i], b = r[i];
            for (int k = 0; k < 4; ++k) { a = ch[0][(size_t) k].process (a); b = ch[1][(size_t) k].process (b); }
            l[i] = a; r[i] = b;
        }
    }
private:
    double sr = 48000.0;
    std::array<std::array<Biquad, 4>, 2> ch;
    float cached[4] { 1000, 1000, 1000, 1000 };
};

//==============================================================================
//  SOURCE VOICINGS - the tailored part of each instrument tab (3 knobs each)
//    1 DRUMS  : ATTACK / SUSTAIN / SNAP         (transient shaper + presence)
//    2 GUITAR : CHOP  TIGHTNESS / DEPTH / CLICK
//               PICK  BRIGHTNESS / ATTACK / LOW CUT
//               RHYTHM BODY / WARMTH / GLUE
//    3 BASS   : ROOTS PRESSURE: PRESSURE / WEIGHT / GROWL
//    4 KEYS   : SHEEN / SWAY (tremolo depth) / RATE
//    5 HORNS  : BITE / BODY / SOFTEN
//    6 VOCAL  : DE-ESS / AIR / WARMTH
//==============================================================================
class SourceStage
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        fast.setTimes (0.0005f, 0.012f, sr);
        slow.setTimes (0.015f, 0.140f, sr);
        bassComp.setTimes (0.003f, 0.090f, sr);
        deEssEnv.setTimes (0.0008f, 0.040f, sr);
        glue.prepare (sr);
        reset();
    }
    void reset()
    {
        fast.reset(); slow.reset(); bassComp.reset(); deEssEnv.reset();
        for (auto& b : bq) for (auto& x : b) x.reset();
        for (auto& x : lows) x.reset();
        for (auto& x : lows2) x.reset();
        for (auto& x : harmLp) x.reset();
        for (auto& x : harmHp) x.reset();
        for (auto& x : deEssBp) x.reset();
        glue.reset();
        shaperGain = 1.0f; chopW = 1.0f; chopHold = 0; chopGain = 1.0f; trem = 0.0f;
        cachedKey = -1;
    }

    // l/r processed in place. sec: 1..6. mode: guitar mode 0..2. k: three knob values 0..1.
    void process (float* l, float* r, int n, int sec, int mode, const float* k)
    {
        if (sec < 1 || sec > 6) return;
        configure (sec, mode, k);
        for (int i = 0; i < n; ++i)
        {
            float a = l[i], b = r[i];
            const float peak = juce::jmax (std::abs (a), std::abs (b));
            const float f = fast.process (peak), s = slow.process (peak);
            switch (sec)
            {
                case 1: // DRUMS
                {
                    const float g = shaperTarget (f, s, attackDb, sustainDb);
                    a *= g; b *= g;
                    a = bq[0][0].process (a); b = bq[1][0].process (b);   // snap shelf
                    break;
                }
                case 2: // GUITAR
                {
                    if (mode == 0)
                    {
                        // CHOP: each strum restarts a short window; after it the sustain falls to a floor.
                        if (chopHold > 0) --chopHold;
                        if (chopHold == 0 && f > 1.8f * s && f > 0.01f) { chopW = 1.0f; chopHold = (int) (0.03 * sr); }
                        chopW *= chopDecay;
                        const float target = chopFloor + (1.0f - chopFloor) * chopW;
                        chopGain += 0.02f * (target - chopGain);
                        a *= chopGain; b *= chopGain;
                        a = bq[0][0].process (a); b = bq[1][0].process (b);
                    }
                    else if (mode == 1)
                    {
                        const float g = shaperTarget (f, s, attackDb, 0.0f);
                        a *= g; b *= g;
                        a = bq[0][0].process (a); b = bq[1][0].process (b);   // brightness shelf
                        a = bq[0][1].process (a); b = bq[1][1].process (b);   // low cut
                    }
                    else
                    {
                        a = bq[0][0].process (a); b = bq[1][0].process (b);   // body
                        a = softSat (a, bodySat) ; b = softSat (b, bodySat);
                        a = bq[0][1].process (a); b = bq[1][1].process (b);   // warmth
                    }
                    break;
                }
                case 3: // BASS - ROOTS PRESSURE
                {
                    const float lowA = lows2[0].process (lows[0].process (a));
                    const float lowB = lows2[1].process (lows[1].process (b));
                    const float lev = bassComp.process (juce::jmax (std::abs (lowA), std::abs (lowB)));
                    const float levDb = gainToDb (lev);
                    const float grDb = levDb > -32.0f ? (-32.0f - levDb) * (1.0f - 1.0f / 8.0f) : 0.0f;
                    const float g = dbToGain (grDb + 9.0f) * pressure;     // squashed low band, added in parallel
                    float ha = harmHp[0].process (std::tanh (4.0f * harmLp[0].process (a)));
                    float hb = harmHp[1].process (std::tanh (4.0f * harmLp[1].process (b)));
                    a = bq[0][0].process (a + lowA * g) + ha * growl;
                    b = bq[1][0].process (b + lowB * g) + hb * growl;
                    break;
                }
                case 4: // KEYS
                {
                    a = bq[0][0].process (a); b = bq[1][0].process (b);       // sheen
                    trem += tremInc; if (trem > 2.0f * kPi) trem -= 2.0f * kPi;
                    const float g = 1.0f - tremDepth * 0.5f * (1.0f + std::sin (trem));
                    a *= g; b *= g;
                    break;
                }
                case 5: // HORNS
                {
                    a = bq[0][0].process (a); b = bq[1][0].process (b);       // bite
                    a = bq[0][1].process (a); b = bq[1][1].process (b);       // body
                    a = juce::jmap (soften, a, softSat (a, 0.35f) * 1.0f);
                    b = juce::jmap (soften, b, softSat (b, 0.35f) * 1.0f);
                    a = bq[0][2].process (a); b = bq[1][2].process (b);       // soften roll-off
                    break;
                }
                case 6: // VOCAL
                {
                    const float ba = deEssBp[0].process (a), bb = deEssBp[1].process (b);
                    const float env = deEssEnv.process (juce::jmax (std::abs (ba), std::abs (bb)));
                    const float overDb = gainToDb (env) - deEssThreshDb;
                    const float redDb = juce::jlimit (0.0f, deEssMaxDb, overDb * 0.9f);
                    const float bandGain = dbToGain (-redDb) - 1.0f;           // change applied to the band only
                    a += ba * bandGain; b += bb * bandGain;
                    a = bq[0][0].process (a); b = bq[1][0].process (b);       // air
                    a = bq[0][1].process (a); b = bq[1][1].process (b);       // warmth shelf
                    a = juce::jmap (warmSat, a, softSat (a, 0.3f)); b = juce::jmap (warmSat, b, softSat (b, 0.3f));
                    break;
                }
            }
            l[i] = a; r[i] = b;
        }
        if (sec == 2 && mode == 2 && glueAmount > 0.001f)
        {
            Compressor::Params cp; cp.threshDb = -22.0f; cp.ratio = 1.0f + 2.0f * glueAmount; cp.makeupDb = 2.5f * glueAmount; cp.mode = 0;
            glue.process (l, r, n, cp);
        }
    }

private:
    float shaperTarget (float f, float s, float attDb, float susDb)
    {
        const float diff = gainToDb (f + 1.0e-6f) - gainToDb (s + 1.0e-6f);
        const float att = juce::jlimit (0.0f, 1.0f, diff / 9.0f);
        const float sus = juce::jlimit (0.0f, 1.0f, -diff / 9.0f);
        const float target = dbToGain (attDb * att + susDb * sus);
        shaperGain += 0.05f * (target - shaperGain);
        return shaperGain;
    }

    void configure (int sec, int mode, const float* k)
    {
        const int key = sec * 100 + mode;
        const bool same = key == cachedKey && k[0] == cachedK[0] && k[1] == cachedK[1] && k[2] == cachedK[2];
        if (same) return;
        cachedKey = key; cachedK[0] = k[0]; cachedK[1] = k[1]; cachedK[2] = k[2];
        for (int c = 0; c < 2; ++c)
            for (auto& b : bq[c]) b.setIdentity();
        auto both = [this] (int slot, auto&& fn) { for (int c = 0; c < 2; ++c) fn (bq[c][(size_t) slot]); };

        switch (sec)
        {
            case 1:
                attackDb = (k[0] - 0.5f) * 18.0f; sustainDb = (k[1] - 0.5f) * 18.0f;
                both (0, [&] (Biquad& b) { b.highShelf (sr, 3200.0, 0.707, k[2] * 7.0); });
                break;
            case 2:
                if (mode == 0)
                {
                    const float tau = 0.35f * std::pow (0.04f / 0.35f, k[0]);
                    chopDecay = std::exp (-1.0f / (tau * (float) sr));
                    chopFloor = dbToGain (-24.0f * k[1]);
                    both (0, [&] (Biquad& b) { b.highShelf (sr, 3000.0, 0.707, k[2] * 6.0); });
                }
                else if (mode == 1)
                {
                    attackDb = k[1] * 9.0f;
                    both (0, [&] (Biquad& b) { b.highShelf (sr, 4500.0, 0.707, k[0] * 9.0); });
                    if (k[2] > 0.02f) { const double f = 60.0 * std::pow (400.0 / 60.0, (double) k[2]); both (1, [&] (Biquad& b) { b.highpass (sr, f, 0.707); }); }
                }
                else
                {
                    bodySat = k[0] * 0.25f;
                    both (0, [&] (Biquad& b) { b.lowShelf (sr, 220.0, 0.707, k[0] * 6.0); });
                    both (1, [&] (Biquad& b) { b.highShelf (sr, 5500.0, 0.707, -k[1] * 7.0); });
                    glueAmount = k[2];
                }
                break;
            case 3:
                pressure = k[0] * 0.55f; growl = k[2] * 0.5f;
                for (int c = 0; c < 2; ++c) { lows[(size_t) c].lowpass (sr, 150.0, 0.707); lows2[(size_t) c].lowpass (sr, 150.0, 0.707);
                                              harmLp[(size_t) c].lowpass (sr, 380.0, 0.707); harmHp[(size_t) c].highpass (sr, 180.0, 0.707); }
                both (0, [&] (Biquad& b) { b.lowShelf (sr, 75.0, 0.707, k[1] * 9.0); });
                break;
            case 4:
                tremDepth = k[1] * 0.6f; tremInc = 2.0f * kPi * (0.6f * std::pow (7.0f / 0.6f, k[2])) / (float) sr;
                both (0, [&] (Biquad& b) { b.highShelf (sr, 9000.0, 0.707, k[0] * 8.0); });
                break;
            case 5:
                soften = k[2];
                both (0, [&] (Biquad& b) { b.peak (sr, 2400.0, 1.1, k[0] * 8.0); });
                both (1, [&] (Biquad& b) { b.peak (sr, 350.0, 0.8, k[1] * 5.0); });
                both (2, [&] (Biquad& b) { b.lowpass (sr, 16000.0 - 9000.0 * k[2], 0.707); });
                break;
            case 6:
                deEssMaxDb = k[0] * 12.0f;
                for (int c = 0; c < 2; ++c) deEssBp[(size_t) c].bandpass (sr, 6800.0, 1.4);
                both (0, [&] (Biquad& b) { b.highShelf (sr, 11000.0, 0.707, k[1] * 6.0); });
                both (1, [&] (Biquad& b) { b.lowShelf (sr, 180.0, 0.707, k[2] * 4.0); });
                warmSat = k[2] * 0.4f;
                break;
        }
    }

    double sr = 48000.0;
    Follower fast, slow, bassComp, deEssEnv;
    std::array<std::array<Biquad, 3>, 2> bq;
    std::array<Biquad, 2> lows, lows2, harmLp, harmHp, deEssBp;
    Compressor glue;
    int cachedKey = -1; float cachedK[3] { -1, -1, -1 };
    float attackDb = 0, sustainDb = 0, shaperGain = 1.0f;
    float chopDecay = 0.999f, chopFloor = 0.5f, chopW = 1.0f, chopGain = 1.0f; int chopHold = 0;
    float bodySat = 0, glueAmount = 0, pressure = 0, growl = 0, tremDepth = 0, tremInc = 0, trem = 0, soften = 0;
    float deEssMaxDb = 0, warmSat = 0; const float deEssThreshDb = -34.0f;
};

//==============================================================================
//  TAPE MACHINE: saturation, tone, wear (HF loss + hiss) and real wow/flutter
//  from a modulated delay line.  The line always adds a fixed 2 ms so latency
//  is constant (reported to the host).
//==============================================================================
class TapeStage
{
public:
    // type: 0 A Clean, 1 B Weight, 2 C Vintage.  speed: 0 7.5, 1 15, 2 30 IPS.
    // stab: 1 = perfectly stable transport, lower adds extra slow wow and drift.  mix: parallel blend with the dry (unmodulated) signal.
    struct Params { float sat = 0, tone = 1, wow = 0, flutter = 0, wear = 0; int type = 0; bool on = true; int speed = 1; float stab = 1.0f, mix = 1.0f; };

    static constexpr float kBaseDelaySeconds = 0.0020f;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        baseDelay = (float) std::lround (kBaseDelaySeconds * sr);
        size = (int) (sr * 0.006) + 8;
        for (auto& b : line) b.assign ((size_t) size, 0.0f);
        pos = 0; reset();
    }
    int latencySamples() const noexcept { return (int) std::lround (baseDelay); }
    void reset()
    {
        for (auto& b : line) std::fill (b.begin(), b.end(), 0.0f);
        for (auto& x : toneLp) x.reset();
        for (auto& x : wearLp) x.reset();
        for (auto& x : bump) x.reset();
        for (auto& x : preEmph) x.reset();
        wowPh = flutPh = flut2Ph = 0.0f; drift = 0.0f; cachedType = -1; cachedWear = -1; cachedTone = -1; cachedSpeed = -1;
    }

    void process (float* l, float* r, int n, const Params& p)
    {
        const float st = juce::jlimit (0.0f, 1.0f, 1.0f - p.stab);                         // 0 = stable
        const float typeWow = p.type == 2 ? 1.6f : (p.type == 1 ? 1.15f : 1.0f);
        const float wowAmt = juce::jmin (1.6f, p.wow + st * 0.45f), fluAmt = juce::jmin (1.6f, p.flutter + st * 0.22f);
        const float limit = baseDelay - 3.0f;                                               // never read ahead of the write head
        const float wowDepth = p.on ? wowAmt * typeWow * 0.0009f * (float) sr : 0.0f;       // samples (about +-0.4% pitch at max)
        const float fluDepth = p.on ? fluAmt * typeWow * 0.00006f * (float) sr : 0.0f;
        const float driftDepth = p.on ? st * 0.0006f * (float) sr : 0.0f;
        const float driftCoef = 1.0f - std::exp (-2.0f * kPi * 0.35f / (float) sr);
        const float wowInc = 2.0f * kPi * 0.65f / (float) sr, fluInc = 2.0f * kPi * 9.3f / (float) sr, flu2Inc = 2.0f * kPi * 13.7f / (float) sr;

        if (p.type != cachedType || p.wear != cachedWear || p.tone != cachedTone || p.speed != cachedSpeed) retune (p);
        const float speedDrive = p.speed == 0 ? 1.25f : (p.speed == 2 ? 0.8f : 1.0f);      // slower tape saturates earlier
        const float drive = p.sat * (p.type == 0 ? 0.85f : 1.0f) * speedDrive;
        const float bias = p.type == 0 ? 0.0f : (p.type == 1 ? 0.14f : 0.10f);             // asymmetry = even harmonics
        const float blend = p.on ? juce::jmin (1.0f, p.sat * 1.6f) : 0.0f;
        const float hiss = p.on ? p.wear * (p.type == 2 ? 0.0030f : 0.0016f) : 0.0f;
        const float wetMix = p.on ? juce::jlimit (0.0f, 1.0f, p.mix) : 1.0f;
        const int dryTap = (int) std::lround (baseDelay);

        for (int i = 0; i < n; ++i)
        {
            wowPh += wowInc; if (wowPh > 2.0f * kPi) wowPh -= 2.0f * kPi;
            flutPh += fluInc; if (flutPh > 2.0f * kPi) flutPh -= 2.0f * kPi;
            flut2Ph += flu2Inc; if (flut2Ph > 2.0f * kPi) flut2Ph -= 2.0f * kPi;
            drift += (rng.nextFloat() * 2.0f - 1.0f - drift) * driftCoef * 6.0f;
            drift = juce::jlimit (-1.0f, 1.0f, drift);
            const float mod = juce::jlimit (-limit, limit, wowDepth * std::sin (wowPh) + fluDepth * (0.7f * std::sin (flutPh) + 0.3f * std::sin (flut2Ph)) + driftDepth * drift);
            float* io[2] { l + i, r + i };
            for (int c = 0; c < 2; ++c)
            {
                float x = *io[c];
                auto& buf = line[(size_t) c];
                buf[(size_t) pos] = x;
                float dry = buf[(size_t) ((pos - dryTap + size) % size)];
                float rd = (float) pos - (baseDelay + mod);
                while (rd < 0) rd += (float) size;
                while (rd >= (float) size) rd -= (float) size;
                const int i0 = juce::jlimit (0, size - 1, (int) rd); const float fr = juce::jlimit (0.0f, 1.0f, rd - (float) i0);
                const int i1 = (i0 + 1) % size;
                x = buf[(size_t) i0] * (1.0f - fr) + buf[(size_t) i1] * fr;

                if (p.on)
                {
                    if (p.type == 1) x = preEmph[(size_t) c].process (x);               // B: weight - low end drives the tape harder
                    const float k = 1.0f + drive * 7.0f;
                    const float sat = (std::tanh ((x + bias) * k) - std::tanh (bias * k)) / (k * (1.0f - std::pow (std::tanh (bias * k), 2.0f)));
                    x = x + (sat - x) * blend;
                    if (p.type != 0) x = bump[(size_t) c].process (x);                  // head bump
                    x = toneLp[(size_t) c].process (x);
                    if (p.wear > 0.001f) x = wearLp[(size_t) c].process (x);
                    if (hiss > 0.0f) x += hiss * (rng.nextFloat() * 2.0f - 1.0f);
                    x = dry + (x - dry) * wetMix;
                }
                *io[c] = x;
            }
            if (++pos >= size) pos = 0;
        }
    }

private:
    void retune (const Params& p)
    {
        cachedType = p.type; cachedWear = p.wear; cachedTone = p.tone; cachedSpeed = p.speed;
        const double spd = p.speed == 0 ? 0.72 : (p.speed == 2 ? 1.30 : 1.0);            // slower speed = duller top end
        const double toneHz = 2200.0 * std::pow (20000.0 / 2200.0, (double) p.tone) * (p.speed == 1 ? 1.0 : (p.speed == 0 ? 0.8 : 1.0));
        const double maxHz = sr * 0.45;
        for (auto& f : toneLp) f.lowpass (sr, juce::jmin (toneHz * (p.speed == 2 ? 1.0 : 1.0), juce::jmin (maxHz, 20000.0 * spd)), 0.707);
        const double wearHz = p.wear > 0.001f ? 16000.0 * std::pow (4500.0 / 16000.0, (double) p.wear) * (p.type == 2 ? 0.8 : 1.0) * spd : maxHz;
        for (auto& f : wearLp) f.lowpass (sr, juce::jmin (wearHz, maxHz), 0.707);
        const double bumpHz = p.speed == 0 ? 60.0 : (p.speed == 1 ? 90.0 : 130.0);        // head bump moves with the tape speed
        for (auto& f : bump) f.peak (sr, bumpHz, 0.9, p.type == 1 ? 3.5 : 2.0);
        for (auto& f : preEmph) f.lowShelf (sr, 160.0, 0.7, 3.0);
    }

    double sr = 48000.0; float baseDelay = 96.0f; int size = 300, pos = 0;
    std::array<std::vector<float>, 2> line;
    std::array<Biquad, 2> toneLp, wearLp, bump, preEmph;
    float wowPh = 0, flutPh = 0, flut2Ph = 0, drift = 0;
    int cachedType = -1, cachedSpeed = -1; float cachedWear = -1, cachedTone = -1;
    juce::Random rng { 1234 };
};

//==============================================================================
//  Small building blocks for the reverbs
//==============================================================================
struct DLine
{
    std::vector<float> b; int w = 0, n = 1;
    void init (int len) { n = juce::jmax (4, len + 4); b.assign ((size_t) n, 0.0f); w = 0; }
    void clear() { std::fill (b.begin(), b.end(), 0.0f); w = 0; }
    void push (float x) { b[(size_t) w] = x; if (++w >= n) w = 0; }
    // sample written d samples ago (d = 0 is the newest)
    float tap (int d) const { int r = w - 1 - d; while (r < 0) r += n; return b[(size_t) r]; }
    float tapF (float d) const
    {
        const int i = (int) d; const float f = d - (float) i;
        return tap (i) * (1.0f - f) + tap (i + 1) * f;
    }
};

// Schroeder allpass with a (optionally modulated) delay: unity gain at every frequency.
struct Allpass
{
    DLine d; int len = 1; float g = 0.5f;
    void init (int length, float gain) { len = juce::jmax (1, length); g = gain; d.init (len + 40); }
    float process (float x, float mod = 0.0f)
    {
        const float del = d.tapF (juce::jmax (0.0f, (float) len - 1.0f + mod));
        const float w = x + g * del;
        const float y = -g * w + del;
        d.push (w);
        return y;
    }
};

//==============================================================================
//  DUB ECHO - the original delay engine (same buffer, timing, feedback,
//  stereo offset, damping and tape-heat structure) with the audit fixes:
//   * the filter / damping / heat now shape every repeat, including the first
//   * the filter resonance really resonates (and is kept loop-stable)
//   * feedback saturation has unity small-signal gain, so tails always decay
//   * the delay time glides instead of clicking
//   * wet/dry no longer lowers the dry signal much
//  plus Tape / Digital / Ping-pong character.
//==============================================================================
class DubEcho
{
public:
    struct Params
    {
        float delaySamples0 = 18000, delaySamples1 = 20000;
        float feedback = 0.46f, mix = 0.26f, cutoff = 2800.0f, resonance = 0.35f, heat = 0.28f, damping = 0.42f;
        int filterMode = 0;      // 0 low, 1 high, 2 band
        int type = 0;            // 0 tape, 1 digital, 2 ping-pong
        bool heads[3] { true, false, false };   // playback heads at 1x, 2x, 3x the delay time
        float revLoop = 0.0f;                    // diffusion (reverb) inside the feedback loop
        bool panic = false;
    };

    static constexpr float maxDelaySeconds = 4.0f;

    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate;
        length = juce::roundToInt (sampleRate * maxDelaySeconds) + maxBlock + 8;
        for (auto& b : buf) b.assign ((size_t) length, 0.0f);
        write = 0;
        juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, 1 };
        for (auto& f : filt) { f.prepare (spec); f.reset(); }
        mixS.reset (sampleRate, 0.025); fbS.reset (sampleRate, 0.035); cutS.reset (sampleRate, 0.03);
        heatS.reset (sampleRate, 0.03); dampS.reset (sampleRate, 0.03); d0S.reset (sampleRate, 0.06); d1S.reset (sampleRate, 0.06);
        first = true; panicGain = 1.0f; wasPanic = false;
        const float ks = (float) (sampleRate / 48000.0); const int lens[3] { 113, 191, 277 };
        for (int c = 0; c < 2; ++c) for (int i = 0; i < 3; ++i) diff[c][i].init (juce::jmax (2, (int) (lens[i] * ks * (c ? 1.13f : 1.0f))), 0.55f);
        diffLp[0] = diffLp[1] = 0.0f;
    }
    void reset()
    {
        for (auto& d : diff) for (auto& a : d) a.d.clear();
        for (auto& b : buf) std::fill (b.begin(), b.end(), 0.0f);
        for (auto& f : filt) f.reset();
        first = true;
    }
    float tailSeconds() const { return maxDelaySeconds * 4.0f; }

    void process (float* l, float* r, int n, const Params& p)
    {
        const float maxD = (float) (length - 4);
        const float d0 = juce::jlimit (1.0f, maxD, p.delaySamples0), d1 = juce::jlimit (1.0f, maxD, p.delaySamples1);
        if (first)
        {
            mixS.setCurrentAndTargetValue (p.mix); fbS.setCurrentAndTargetValue (p.feedback); cutS.setCurrentAndTargetValue (p.cutoff);
            heatS.setCurrentAndTargetValue (p.heat); dampS.setCurrentAndTargetValue (p.damping);
            d0S.setCurrentAndTargetValue (d0); d1S.setCurrentAndTargetValue (d1); first = false;
        }
        if (p.panic && ! wasPanic) { for (auto& b : buf) std::fill (b.begin(), b.end(), 0.0f); for (auto& f : filt) f.reset(); panicGain = 0.0f; }
        wasPanic = p.panic;

        mixS.setTargetValue (p.mix); fbS.setTargetValue (p.feedback); cutS.setTargetValue (p.cutoff);
        heatS.setTargetValue (p.heat); dampS.setTargetValue (p.damping); d0S.setTargetValue (d0); d1S.setTargetValue (d1);

        const auto type = p.filterMode == 1 ? juce::dsp::StateVariableTPTFilterType::highpass
                        : p.filterMode == 2 ? juce::dsp::StateVariableTPTFilterType::bandpass
                                            : juce::dsp::StateVariableTPTFilterType::lowpass;
        const float q = 0.5f * std::pow (16.0f, juce::jlimit (0.0f, 1.0f, (p.resonance - 0.1f) / 0.82f));   // 0.5 .. 8
        float* io[2] { l, r };
        const int nHeads = (p.heads[0] ? 1 : 0) + (p.heads[1] ? 1 : 0) + (p.heads[2] ? 1 : 0);
        const float wSum = (p.heads[0] ? 1.0f : 0.0f) + (p.heads[1] ? 0.8f : 0.0f) + (p.heads[2] ? 0.65f : 0.0f);
        const float headNorm = wSum > 0.01f ? 1.0f / wSum : 1.0f;     // loop gain can never exceed the feedback setting

        for (int i = 0; i < n; ++i)
        {
            const float mix = mixS.getNextValue();
            const float fb = juce::jlimit (0.0f, 0.94f, fbS.getNextValue());
            const float cutoff = juce::jmin (cutS.getNextValue(), (float) sr * 0.45f);
            const float heat = heatS.getNextValue(), damping = dampS.getNextValue();
            const float dl[2] { d0S.getNextValue(), d1S.getNextValue() };
            // a resonant peak must never push the loop gain above 1
            const float loopQ = juce::jmin (q, juce::jmax (0.7071f, 0.96f / juce::jmax (fb, 0.05f)));
            if (panicGain < 1.0f) panicGain = juce::jmin (1.0f, panicGain + 0.0008f);

            float in[2] { io[0][i], io[1][i] };
            float tap[2], fed[2];
            for (int c = 0; c < 2; ++c)
            {
                float rp = (float) write - dl[c];
                while (rp < 0.0f) rp += (float) length;
                if (rp >= (float) length) rp -= (float) length;
                const int i0 = juce::jlimit (0, length - 1, (int) rp); const float fr = juce::jlimit (0.0f, 1.0f, rp - (float) i0); const int i1 = (i0 + 1) % length;
                float delayed = buf[(size_t) c][(size_t) i0] * (1.0f - fr) + buf[(size_t) c][(size_t) i1] * fr;
                if (nHeads > 1 || ! p.heads[0])
                {
                    delayed = p.heads[0] ? delayed : 0.0f;
                    for (int h = 1; h < 3; ++h)
                        if (p.heads[h])
                        {
                            float rh = (float) write - juce::jmin (dl[c] * (float) (h + 1), maxD); while (rh < 0.0f) rh += (float) length;
                            if (rh >= (float) length) rh -= (float) length;
                            const int j0 = juce::jlimit (0, length - 1, (int) rh); const float fh = juce::jlimit (0.0f, 1.0f, rh - (float) j0); const int j1 = (j0 + 1) % length;
                            delayed += (buf[(size_t) c][(size_t) j0] * (1.0f - fh) + buf[(size_t) c][(size_t) j1] * fh) * (h == 1 ? 0.8f : 0.65f);
                        }
                    delayed *= headNorm;
                }
                if (p.revLoop > 0.001f)        // reverb in the loop: repeats smear into each other
                {
                    float d = delayed;
                    for (int a = 0; a < 3; ++a) d = diff[c][a].process (d);
                    diffLp[c] += (d - diffLp[c]) * 0.55f;
                    delayed += (diffLp[c] - delayed) * juce::jlimit (0.0f, 1.0f, p.revLoop);
                }
                auto& f = filt[(size_t) c];
                f.setType (type); f.setCutoffFrequency (cutoff); f.setResonance (loopQ);
                float x = f.processSample (0, delayed);
                x *= (1.0f - damping * 0.58f);
                if (p.type == 0) x = softSat (x, heat);                         // tape: saturates every repeat
                else if (p.type == 1) { /* digital: clean repeats */ }
                else x = softSat (x, heat * 0.5f);
                tap[c] = x;
            }
            if (p.type == 2)
            {
                // ping-pong: mono in -> left line, the repeats bounce right <-> left
                const float m = 0.5f * (in[0] + in[1]);
                fed[0] = m + tap[1] * fb;
                fed[1] = tap[0] * fb;
            }
            else
            {
                fed[0] = in[0] + tap[0] * fb;
                fed[1] = in[1] + tap[1] * fb;
            }
            const float dryGain = 1.0f - mix * mix;
            for (int c = 0; c < 2; ++c)
            {
                buf[(size_t) c][(size_t) write] = juce::jlimit (-1.25f, 1.25f, fed[c]);
                io[c][i] = in[c] * dryGain + tap[c] * mix * panicGain;
            }
            if (++write >= length) write = 0;
        }
    }

private:
    double sr = 48000.0; int length = 0, write = 0;
    std::array<std::vector<float>, 2> buf;
    std::array<juce::dsp::StateVariableTPTFilter<float>, 2> filt;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixS, fbS, cutS, heatS, dampS, d0S, d1S;
    bool first = true, wasPanic = false; float panicGain = 1.0f;
    std::array<std::array<Allpass, 3>, 2> diff; float diffLp[2] { 0, 0 };
};

//==============================================================================
//  RHYTHM DELAY  (second delay: analog / digital / ping-pong, tempo-synced)
//==============================================================================
class RhythmDelay
{
public:
    struct Params { float delaySamples = 12000, feedback = 0.3f, mix = 0.0f; int type = 0; };

    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate;
        length = (int) (sampleRate * 4.0) + maxBlock + 8;
        for (auto& b : buf) b.assign ((size_t) length, 0.0f);
        write = 0; first = true;
        dS.reset (sampleRate, 0.08); mS.reset (sampleRate, 0.03); fS.reset (sampleRate, 0.03);
        reset();
    }
    void reset() { for (auto& b : buf) std::fill (b.begin(), b.end(), 0.0f); for (auto& f : lp) f.reset(); for (auto& f : hp) f.reset(); first = true; }

    void process (float* l, float* r, int n, const Params& p)
    {
        const float maxD = (float) (length - 4);
        const float d = juce::jlimit (1.0f, maxD, p.delaySamples);
        if (first) { dS.setCurrentAndTargetValue (d); mS.setCurrentAndTargetValue (p.mix); fS.setCurrentAndTargetValue (p.feedback); first = false; }
        dS.setTargetValue (d); mS.setTargetValue (p.mix); fS.setTargetValue (p.feedback);
        if (p.type != cachedType)
        {
            cachedType = p.type;
            for (auto& f : lp) f.lowpass (sr, p.type == 0 ? 4200.0 : sr * 0.45, 0.707);
            for (auto& f : hp) f.highpass (sr, p.type == 0 ? 160.0 : 20.0, 0.707);
        }
        float* io[2] { l, r };
        for (int i = 0; i < n; ++i)
        {
            const float dl = dS.getNextValue(), mix = mS.getNextValue(), fb = juce::jlimit (0.0f, 0.92f, fS.getNextValue());
            float tap[2], in[2] { io[0][i], io[1][i] };
            for (int c = 0; c < 2; ++c)
            {
                float rp = (float) write - dl; while (rp < 0.0f) rp += (float) length;
                if (rp >= (float) length) rp -= (float) length;
                const int i0 = juce::jlimit (0, length - 1, (int) rp); const float fr = juce::jlimit (0.0f, 1.0f, rp - (float) i0); const int i1 = (i0 + 1) % length;
                float x = buf[(size_t) c][(size_t) i0] * (1.0f - fr) + buf[(size_t) c][(size_t) i1] * fr;
                x = hp[(size_t) c].process (lp[(size_t) c].process (x));
                if (p.type == 0) x = softSat (x, 0.35f);                         // analog: darker, slightly saturated repeats
                tap[c] = x;
            }
            float fed[2];
            if (p.type == 2) { const float m = 0.5f * (in[0] + in[1]); fed[0] = m + tap[1] * fb; fed[1] = tap[0] * fb; }
            else             { fed[0] = in[0] + tap[0] * fb; fed[1] = in[1] + tap[1] * fb; }
            for (int c = 0; c < 2; ++c)
            {
                buf[(size_t) c][(size_t) write] = juce::jlimit (-1.25f, 1.25f, fed[c]);
                io[c][i] = in[c] * (1.0f - mix * mix) + tap[c] * mix;
            }
            if (++write >= length) write = 0;
        }
    }
private:
    double sr = 48000.0; int length = 0, write = 0, cachedType = -1; bool first = true;
    std::array<std::vector<float>, 2> buf;
    std::array<Biquad, 2> lp, hp;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> dS, mS, fS;
};

//==============================================================================
//  SPRING TANK: a physical-style model.  Each spring is a dispersive allpass
//  cascade (low frequencies travel slower than highs, which is what makes the
//  "boing" chirp) inside a feedback loop with a delay line and a damping filter.
//  Seven characters differ in dispersion, loop time, regeneration and tone.
//==============================================================================
class SpringEngine
{
public:
    struct Params { int character = 1, length = 1; float drive = 0.25f, fb = 0.3f, treble = 0.5f, bass = 0.5f, decay = 0.4f; bool mono = false; };

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        for (auto& s : sp) { s.dl.init ((int) (sr * 0.12)); }
        reset();
    }
    void reset()
    {
        for (auto& s : sp)
        {
            s.dl.clear(); s.ap.fill (0.0f); s.lp = s.dc = s.dcIn = s.prev = 0.0f;
            s.inHp.reset(); s.inShelf.reset(); s.tBass.reset(); s.tTreb.reset(); s.dcOut.reset();
        }
        lfo = 0.0f; cachedChar = -1; cachedTb = -1.0f;
    }

    // in: pre-delayed mono-ish input (per channel); out: wet signal
    void process (const float* inL, const float* inR, float* outL, float* outR, int n, const Params& p)
    {
        if (p.character != cachedChar || p.treble != cachedTr || p.bass != cachedBs) retune (p);
        const auto& cd = chars[(size_t) juce::jlimit (0, 6, p.character)];
        const float lenScale = p.length == 0 ? 0.80f : (p.length == 2 ? 1.12f : 1.0f);
        float fb = cd.fb * lenScale * (0.80f + 0.40f * juce::jlimit (0.0f, 1.0f, p.decay)) + (p.fb - 0.30f) * 0.25f;
        fb = juce::jlimit (0.0f, 0.95f, fb);
        const float drive = p.drive * 0.8f;
        const float lpCoef = 1.0f - std::exp (-2.0f * kPi * cd.lpHz / (float) sr);
        const float lfoInc = 2.0f * kPi * cd.modRate / (float) sr;

        for (int i = 0; i < n; ++i)
        {
            lfo += lfoInc; if (lfo > 2.0f * kPi) lfo -= 2.0f * kPi;
            const float in = 0.5f * (inL[i] + inR[i]);
            float o[2];
            for (int c = 0; c < 2; ++c)
            {
                auto& s = sp[(size_t) c];
                const float skew = c == 0 ? 1.0f : 1.137f;                             // the second spring is a little longer
                float x = s.inHp.process (in);
                x = s.inShelf.process (x);
                x = softSat (x, drive);
                // feedback path: damped, DC-blocked
                float v = x + fb * s.prev;
                // dispersion: first-order allpass cascade
                const float a = cd.apCoef * (c == 0 ? 1.0f : 0.965f);
                for (int k = 0; k < cd.stages; ++k)
                {
                    const float y = -a * v + s.ap[(size_t) k];
                    s.ap[(size_t) k] = v + a * y;
                    v = y;
                }
                s.dl.push (v);
                float dTime = cd.loopMs * 0.001f * (float) sr * skew;
                dTime *= 1.0f + cd.modDepth * std::sin (lfo + (c ? 1.7f : 0.0f));
                const float dl = s.dl.tapF (juce::jlimit (4.0f, (float) s.dl.n - 6.0f, dTime));
                s.lp += (dl - s.lp) * lpCoef;
                const float hpOut = s.lp - s.dc; s.dc += hpOut * 0.0008f;               // remove DC from the loop
                s.prev = juce::jlimit (-2.0f, 2.0f, hpOut);
                float y = 0.55f * v + 0.45f * s.prev;
                y = s.tBass.process (y); y = s.tTreb.process (y);
                o[c] = y * cd.gain;
            }
            if (p.mono) { const float m = 0.5f * (o[0] + o[1]); o[0] = o[1] = m; }
            outL[i] = o[0]; outR[i] = o[1];
        }
    }

private:
    struct Char { int stages; float apCoef, loopMs, fb, lpHz, hpHz, shelfDb, modRate, modDepth, gain, outShelf; };
    // Drip, Roots, Twang, Heavy, Surf, Dark Tank, Shimmer
    const std::array<Char, 7> chars { {
        { 40, 0.68f, 26.0f, 0.78f, 4800.0f, 200.0f,  4.0f, 0.00f, 0.000f, 1.25f, 0.0f },
        { 28, 0.62f, 34.0f, 0.85f, 4200.0f, 160.0f,  0.0f, 0.00f, 0.000f, 1.25f, 0.0f },
        { 22, 0.55f, 22.0f, 0.80f, 7000.0f, 420.0f,  3.0f, 0.00f, 0.000f, 1.20f, 1.5f },
        { 34, 0.70f, 40.0f, 0.88f, 3200.0f, 110.0f, -2.0f, 0.00f, 0.000f, 1.30f, -1.0f },
        { 46, 0.72f, 30.0f, 0.85f, 6500.0f, 230.0f,  3.0f, 0.80f, 0.020f, 1.20f, 1.0f },
        { 30, 0.64f, 38.0f, 0.86f, 2200.0f, 140.0f, -3.0f, 0.00f, 0.000f, 1.35f, -2.0f },
        { 26, 0.58f, 28.0f, 0.86f, 9000.0f, 260.0f,  2.0f, 0.45f, 0.012f, 1.15f, 5.0f } } };

    void retune (const Params& p)
    {
        cachedChar = p.character; cachedTr = p.treble; cachedBs = p.bass; cachedTb = 0.0f;
        const auto& cd = chars[(size_t) juce::jlimit (0, 6, p.character)];
        for (auto& s : sp)
        {
            s.inHp.highpass (sr, cd.hpHz, 0.707);
            s.inShelf.highShelf (sr, 2200.0, 0.7, cd.shelfDb);
            s.tBass.lowShelf (sr, 180.0, 0.7, (p.bass - 0.5f) * 16.0f);
            s.tTreb.highShelf (sr, 3800.0, 0.7, (p.treble - 0.5f) * 16.0f + cd.outShelf);
        }
    }

    struct Spring
    {
        DLine dl; std::array<float, 64> ap {}; float lp = 0, dc = 0, dcIn = 0, prev = 0;
        Biquad inHp, inShelf, tBass, tTreb, dcOut;
    };
    double sr = 48000.0; std::array<Spring, 2> sp; float lfo = 0.0f;
    int cachedChar = -1; float cachedTr = -1.0f, cachedBs = -1.0f, cachedTb = -1.0f;
};

//==============================================================================
//  PLATE: figure-of-eight tank (two cross-coupled branches of modulated
//  allpasses, delays and damping) in the style of Dattorro's plate design.
//==============================================================================
class PlateEngine
{
public:
    struct Params { float decay = 0.4f, damp = 0.4f, lowCut = 0.15f, width = 1.0f; int tone = 1; };

    void prepare (double sampleRate)
    {
        sr = sampleRate; k = (float) (sr / 29761.0);
        auto S = [&] (int v) { return juce::jmax (2, (int) std::lround ((float) v * k)); };
        inAp[0].init (S (142), 0.75f); inAp[1].init (S (107), 0.75f); inAp[2].init (S (379), 0.625f); inAp[3].init (S (277), 0.625f);
        apMod[0].init (S (672), -0.70f); apMod[1].init (S (908), -0.70f);
        dl1[0].init (S (4453)); dl1[1].init (S (4217));
        ap2[0].init (S (1800), 0.50f); ap2[1].init (S (2656), 0.50f);
        dl2[0].init (S (3720)); dl2[1].init (S (3163));
        reset();
    }
    void reset()
    {
        for (auto& a : inAp) a.d.clear(); for (auto& a : apMod) a.d.clear(); for (auto& a : ap2) a.d.clear();
        for (auto& d : dl1) d.clear(); for (auto& d : dl2) d.clear();
        lpState[0] = lpState[1] = bw = 0.0f; fbL = fbR = 0.0f; lfo = 0.0f; cachedTone = -1; cachedLow = -1.0f;
        for (auto& f : outHp) f.reset(); for (auto& f : outShelf) f.reset();
    }

    void process (const float* inL, const float* inR, float* outL, float* outR, int n, const Params& p)
    {
        if (p.tone != cachedTone || p.lowCut != cachedLow) retune (p);
        const float dec = 0.30f + 0.65f * juce::jlimit (0.0f, 1.0f, p.decay);
        const float damp = 0.05f + 0.90f * juce::jlimit (0.0f, 1.0f, p.damp);
        const float bwCoef = p.tone == 0 ? 0.98f : (p.tone == 2 ? 0.55f : 0.80f);
        const float lfoInc = 2.0f * kPi * 0.9f / (float) sr, depth = 7.0f * k;
        auto T = [&] (int v) { return juce::jmax (0, (int) std::lround ((float) v * k)); };

        for (int i = 0; i < n; ++i)
        {
            lfo += lfoInc; if (lfo > 2.0f * kPi) lfo -= 2.0f * kPi;
            float x = 0.5f * (inL[i] + inR[i]);
            bw += (x - bw) * bwCoef; x = bw;
            for (auto& a : inAp) x = a.process (x);
            // left branch
            float l = apMod[0].process (x + dec * fbR, depth * std::sin (lfo));
            dl1[0].push (l);
            float yl = dl1[0].tap (dl1[0].n - 6);
            lpState[0] = (1.0f - damp) * yl + damp * lpState[0];
            float l2 = ap2[0].process (lpState[0] * dec);
            dl2[0].push (l2);
            fbL = dl2[0].tap (dl2[0].n - 6);
            // right branch
            float r = apMod[1].process (x + dec * fbL, depth * std::sin (lfo + 2.1f));
            dl1[1].push (r);
            float yr = dl1[1].tap (dl1[1].n - 6);
            lpState[1] = (1.0f - damp) * yr + damp * lpState[1];
            float r2 = ap2[1].process (lpState[1] * dec);
            dl2[1].push (r2);
            fbR = dl2[1].tap (dl2[1].n - 6);

            float oL = dl1[1].tap (T (266)) + dl1[1].tap (T (2974)) - ap2[1].d.tap (T (1913)) + dl2[1].tap (T (1996)) - dl1[0].tap (T (1990)) - ap2[0].d.tap (T (187)) - dl2[0].tap (T (1066));
            float oR = dl1[0].tap (T (353)) + dl1[0].tap (T (3627)) - ap2[0].d.tap (T (1228)) + dl2[0].tap (T (2111)) - dl1[1].tap (T (335)) - ap2[1].d.tap (T (2132)) - dl2[1].tap (T (121));
            oL *= 0.55f; oR *= 0.55f;
            const float m = 0.5f * (oL + oR), s = 0.5f * (oL - oR) * p.width;
            oL = outShelf[0].process (outHp[0].process (m + s));
            oR = outShelf[1].process (outHp[1].process (m - s));
            outL[i] = oL; outR[i] = oR;
        }
    }

private:
    void retune (const Params& p)
    {
        cachedTone = p.tone; cachedLow = p.lowCut;
        const double hz = 20.0 * std::pow (30.0, (double) juce::jlimit (0.0f, 1.0f, p.lowCut));
        for (auto& f : outHp) f.highpass (sr, hz, 0.707);
        for (auto& f : outShelf) f.highShelf (sr, 5000.0, 0.7, p.tone == 0 ? 4.0 : (p.tone == 2 ? -5.0 : 0.0));
    }
    double sr = 48000.0; float k = 1.6f, lfo = 0.0f, bw = 0.0f, fbL = 0.0f, fbR = 0.0f; float lpState[2] { 0, 0 };
    std::array<Allpass, 4> inAp; std::array<Allpass, 2> apMod, ap2; std::array<DLine, 2> dl1, dl2;
    std::array<Biquad, 2> outHp, outShelf; int cachedTone = -1; float cachedLow = -1.0f;
};

//==============================================================================
//  REVERB STAGE: pre-delay, then the Spring or Plate engine, then the mix.
//==============================================================================
class ReverbStage
{
public:
    struct Params
    {
        int type = 0; float preMs = 20.0f, decay = 0.4f, mix = 0.0f;
        SpringEngine::Params spring; PlateEngine::Params plate;
    };

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        preSize = (int) (sampleRate * 0.13) + 8;
        for (auto& b : pre) b.assign ((size_t) preSize, 0.0f);
        spring.prepare (sr); plate.prepare (sr);
        reset();
    }
    void reset()
    {
        for (auto& b : pre) std::fill (b.begin(), b.end(), 0.0f);
        preWrite = 0; spring.reset(); plate.reset();
        mixS.setCurrentAndTargetValue (0.0f); first = true;
    }

    void process (float* l, float* r, int n, const Params& p)
    {
        if (first) { mixS.reset (sr, 0.03); mixS.setCurrentAndTargetValue (p.mix); first = false; }
        mixS.setTargetValue (p.mix);
        if ((int) wl.size() < n) { for (auto* v : { &wl, &wr, &xl, &xr }) v->assign ((size_t) n, 0.0f); }
        const int preSamples = juce::jlimit (0, preSize - 2, (int) (p.preMs * 0.001f * (float) sr));
        float* io[2] { l, r };
        float* x[2] { xl.data(), xr.data() };
        for (int i = 0; i < n; ++i)
        {
            for (int c = 0; c < 2; ++c)
            {
                auto& b = pre[(size_t) c];
                b[(size_t) preWrite] = io[c][i];
                int rd = preWrite - preSamples; if (rd < 0) rd += preSize;
                x[c][i] = b[(size_t) rd];
            }
            if (++preWrite >= preSize) preWrite = 0;
        }
        auto sp = p.spring; sp.decay = p.decay;
        auto pl = p.plate;  pl.decay = p.decay;
        if (p.type == 0) spring.process (xl.data(), xr.data(), wl.data(), wr.data(), n, sp);
        else             plate.process (xl.data(), xr.data(), wl.data(), wr.data(), n, pl);
        for (int i = 0; i < n; ++i)
        {
            const float m = mixS.getNextValue();
            io[0][i] = io[0][i] * (1.0f - m * m) + wl[(size_t) i] * m * wetGain;
            io[1][i] = io[1][i] * (1.0f - m * m) + wr[(size_t) i] * m * wetGain;
        }
    }

private:
    static constexpr float wetGain = 1.0f;
    double sr = 48000.0; int preSize = 0, preWrite = 0; bool first = true;
    SpringEngine spring; PlateEngine plate;
    std::array<std::vector<float>, 2> pre;
    std::vector<float> wl, wr, xl, xr;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixS;
};

//==============================================================================
//  MODULATION: chorus / phaser / flanger
//==============================================================================
class ModStage
{
public:
    struct Params { int type = 0; float rate = 0.8f, depth = 0.5f, mix = 0.0f; };

    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate;
        spec = { sampleRate, (juce::uint32) maxBlock, 2 };
        chorus.prepare (spec); phaser.prepare (spec); flanger.prepare (spec);
        wet.setSize (2, maxBlock); reset();
    }
    void reset() { chorus.reset(); phaser.reset(); flanger.reset(); }

    void process (float* l, float* r, int n, const Params& p)
    {
        if (p.mix < 0.001f) return;
        wet.setSize (2, n, false, false, true);
        wet.copyFrom (0, 0, l, n); wet.copyFrom (1, 0, r, n);
        float* ch[2] { wet.getWritePointer (0), wet.getWritePointer (1) };
        juce::dsp::AudioBlock<float> block (ch, 2, (size_t) n);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        if (p.type == 0)
        {
            chorus.setRate (p.rate); chorus.setDepth (0.25f + 0.5f * p.depth); chorus.setCentreDelay (12.0f);
            chorus.setFeedback (0.0f); chorus.setMix (1.0f); chorus.process (ctx);
        }
        else if (p.type == 1)
        {
            phaser.setRate (p.rate); phaser.setDepth (0.3f + 0.7f * p.depth); phaser.setCentreFrequency (900.0f);
            phaser.setFeedback (0.45f); phaser.setMix (1.0f); phaser.process (ctx);
        }
        else
        {
            flanger.setRate (p.rate * 0.6f); flanger.setDepth (0.35f + 0.6f * p.depth); flanger.setCentreDelay (2.5f);
            flanger.setFeedback (0.55f); flanger.setMix (1.0f); flanger.process (ctx);
        }
        const float m = p.mix;
        for (int i = 0; i < n; ++i)
        {
            l[i] = l[i] * (1.0f - 0.5f * m) + ch[0][i] * m * 0.5f * 1.4f;
            r[i] = r[i] * (1.0f - 0.5f * m) + ch[1][i] * m * 0.5f * 1.4f;
        }
    }
private:
    double sr = 48000.0; juce::dsp::ProcessSpec spec {};
    juce::dsp::Chorus<float> chorus, flanger; juce::dsp::Phaser<float> phaser;
    juce::AudioBuffer<float> wet;
};

//==============================================================================
//  LIMITER (peak, instant attack, smooth release, hard ceiling, optional soft clip)
//==============================================================================
class Limiter
{
public:
    struct Params { float threshDb = 0.0f, releaseMs = 120.0f, ceilingDb = -0.3f; bool soft = false; };
    void prepare (double sampleRate) { sr = sampleRate; env = 1.0f; lastGr = 0.0f; }
    void reset() { env = 1.0f; }
    float process (float* l, float* r, int n, const Params& p)
    {
        const float thr = dbToGain (p.threshDb), ceil = dbToGain (p.ceilingDb);
        const float rel = onePoleCoef (p.releaseMs * 0.001f, sr);
        const float make = ceil / thr;
        float maxGr = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            const float pk = juce::jmax (std::abs (l[i]), std::abs (r[i]));
            const float target = pk > thr ? thr / pk : 1.0f;
            env = target < env ? target : env + (target - env) * rel;
            float a = l[i] * env * make, b = r[i] * env * make;
            if (p.soft) { a = ceil * std::tanh (a / ceil); b = ceil * std::tanh (b / ceil); }
            l[i] = juce::jlimit (-ceil, ceil, a); r[i] = juce::jlimit (-ceil, ceil, b);
            maxGr = juce::jmax (maxGr, -gainToDb (env));
        }
        lastGr = maxGr; return maxGr;
    }
    float lastGainReduction() const noexcept { return lastGr; }
private:
    double sr = 48000.0; float env = 1.0f, lastGr = 0.0f;
};

//==============================================================================
//  STEADYBASS - weight-preserving bass leveller
//   * detector: DC-blocked RMS (22 ms), so a 40 Hz wave does not ripple the gain
//   * threshold follows the player's own average level, so it works at any input level
//   * slow attack ("punch") lets each note's front edge through
//   * low band (< 220 Hz, one-pole split, no latency) gets less gain reduction than the
//     mids/highs (WEIGHT), so the fundamental keeps its body
//   * automatic make-up matches the long-term loudness of input and output
//==============================================================================
// Sub-band control (30-50 Hz) with optional harmonics that carry the note on small speakers.
class SubStage
{
public:
    struct Params { float levelDb = 0.0f, harm = 0.0f; };
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        for (int c = 0; c < 2; ++c)
        {
            hp1[c].highpass (sr, 30.0, 0.707); lp1[c].lowpass (sr, 50.0, 0.707);
            hp2[c].highpass (sr, 30.0, 0.707); lp2[c].lowpass (sr, 50.0, 0.707);
            hh[c].highpass (sr, 90.0, 0.707); hl[c].lowpass (sr, 600.0, 0.707);
        }
        reset();
    }
    void reset() { for (int c = 0; c < 2; ++c) { hp1[c].reset(); lp1[c].reset(); hp2[c].reset(); lp2[c].reset(); hh[c].reset(); hl[c].reset(); } level = 0.0f; }
    // returns nothing; the sub-band peak is available from getLevel()
    void process (float* l, float* r, int n, const Params& p)
    {
        const float g = std::pow (10.0f, p.levelDb / 20.0f) - 1.0f;
        const float cd = std::exp (-1.0f / (0.060f * (float) sr));
        float* ch[2] { l, r };
        for (int i = 0; i < n; ++i)
            for (int c = 0; c < 2; ++c)
            {
                const float x = ch[c][i];
                const float b = lp2[c].process (lp1[c].process (hp2[c].process (hp1[c].process (x))));
                level = std::max (std::abs (b), level * cd);
                float y = x + g * 1.85f * b;      // 1.85 makes the band path unity gain at 40 Hz
                if (p.harm > 0.001f)
                {
                    const float h = hl[c].process (hh[c].process (std::tanh (8.0f * b) * 0.25f));
                    y += p.harm * h;
                }
                ch[c][i] = y;
            }
    }
    float getLevel() const noexcept { return level; }
private:
    double sr = 44100.0; float level = 0.0f;
    Biquad hp1[2], lp1[2], hp2[2], lp2[2], hh[2], hl[2];
};

// Monitor-only small-speaker simulation (phone, laptop, MacBook, studio monitor). Never used when exporting.
class SpeakerCheck
{
public:
    void prepare (double sampleRate) { sr = sampleRate; mode = -1; }
    void reset() { for (auto& b : f) for (auto& q : b) q.reset(); }
    void process (float* l, float* r, int n, int m, float levelDb)
    {
        if (m != mode)
        {
            mode = m; reset();
            for (int c = 0; c < 2; ++c)
            {
                for (auto& q : f[c]) q.setIdentity();
                switch (m)
                {
                    case 1: f[c][0].highpass (sr, 420, 0.8); f[c][1].highpass (sr, 420, 0.8); f[c][2].lowpass (sr, 7500, 0.7); f[c][3].peak (sr, 1800, 1.0, 4.0); break;
                    case 2: f[c][0].highpass (sr, 200, 0.75); f[c][1].highpass (sr, 200, 0.75); f[c][2].lowpass (sr, 14000, 0.7); f[c][3].peak (sr, 2400, 0.9, 2.5); break;
                    case 3: f[c][0].highpass (sr, 90, 0.7); f[c][1].highpass (sr, 90, 0.7); f[c][2].lowpass (sr, 18000, 0.7); f[c][3].peak (sr, 250, 0.8, 1.5); break;
                    case 4: f[c][0].highpass (sr, 38, 0.7); break;
                    default: break;
                }
            }
        }
        if (m <= 0) return;
        const float g = std::pow (10.0f, levelDb / 20.0f);
        for (int i = 0; i < n; ++i)
        {
            float a = l[i], b = r[i];
            if (m <= 2) a = b = 0.5f * (a + b);          // phone / laptop: one small speaker
            for (auto& q : f[0]) a = q.process (a);
            for (auto& q : f[1]) b = q.process (b);
            l[i] = a * g; r[i] = b * g;
        }
    }
private:
    double sr = 44100.0; int mode = -1;
    Biquad f[2][4];
};

// Per-drum voicing (Kick, Snare, Rim, Hat, Toms; Loop leaves the sound alone) and the kick/bass EQ lock.
class DrumVoice
{
public:
    void prepare (double sampleRate) { sr = sampleRate; vType = -2; lockKey = -1.0f; reset(); }
    void reset() { for (auto& c : v) for (auto& q : c) q.reset(); for (auto& c : k) for (auto& q : c) q.reset(); }
    void voice (float* l, float* r, int n, int type)
    {
        type = juce::jlimit (0, 5, type);
        if (type != vType)
        {
            vType = type;
            for (int c = 0; c < 2; ++c)
            {
                for (auto& q : v[c]) q.setIdentity();
                auto& f = v[c];
                switch (type)
                {
                    case 0: f[0].highpass (sr, 35, 0.7); f[1].lowShelf (sr, 70, 0.7, 2.5); f[2].peak (sr, 350, 1.0, -2.5); f[3].peak (sr, 3500, 1.0, 1.5); break;
                    case 1: f[0].highpass (sr, 70, 0.7); f[1].peak (sr, 200, 1.0, 2.0); f[2].peak (sr, 4500, 0.9, 3.0); f[3].peak (sr, 500, 1.0, -1.5); break;
                    case 2: f[0].highpass (sr, 200, 0.7); f[1].peak (sr, 1800, 1.0, 3.0); f[2].lowpass (sr, 9000, 0.7); break;
                    case 3: f[0].highpass (sr, 400, 0.7); f[1].peak (sr, 3500, 1.0, -2.5); f[2].highShelf (sr, 10000, 0.7, 1.5); break;
                    case 4: f[0].highpass (sr, 50, 0.7); f[1].peak (sr, 120, 1.0, 3.0); f[2].peak (sr, 400, 1.0, -2.0); break;
                    default: break;
                }
            }
        }
        if (type == 5) return;
        for (int i = 0; i < n; ++i)
        {
            float a = l[i], b = r[i];
            for (auto& q : v[0]) a = q.process (a);
            for (auto& q : v[1]) b = q.process (b);
            l[i] = a; r[i] = b;
        }
    }
    // Makes room for the bass: dips the drums under about 100 Hz and keeps the kick's knock just above it.
    void lock (float* l, float* r, int n, float amount)
    {
        if (std::abs (amount - lockKey) > 0.002f)
        {
            lockKey = amount;
            for (int c = 0; c < 2; ++c) { k[c][0].lowShelf (sr, 100, 0.7, -7.0 * amount); k[c][1].peak (sr, 125, 1.2, 2.0 * amount); }
        }
        for (int i = 0; i < n; ++i)
        {
            float a = l[i], b = r[i];
            for (auto& q : k[0]) a = q.process (a);
            for (auto& q : k[1]) b = q.process (b);
            l[i] = a; r[i] = b;
        }
    }
private:
    double sr = 44100.0; int vType = -2; float lockKey = -1.0f;
    Biquad v[2][4], k[2][2];
};

// Control-room listening: monitor voicing, room, speaker position, colour, mono, headphone crossfeed.
// A mixing aid only: it approximates the character of listening setups, it does not model a particular speaker or room.
class ControlRoom
{
public:
    struct Params { int mon = 0, room = 1; float angle = 60.0f, dist = 2.4f, toe = 0.5f, colour = 0.35f, levelDb = 0.0f; bool mono = false, phones = false; };
    void prepare (double sampleRate) { sr = sampleRate; plate.prepare (sr); bufL.assign (512, 0.0f); bufR.assign (512, 0.0f); wetL.assign (512, 0.0f); wetR.assign (512, 0.0f); preL.assign ((size_t) (sr * 0.05) + 8, 0.0f); preR = preL; wp = 0; key = -1; reset(); }
    void reset()
    {
        plate.reset();
        for (auto& c : mf) for (auto& q : c) q.reset();
        for (auto& c : col) for (auto& q : c) q.reset();
        for (auto& q : air) q.reset(); for (auto& q : toeF) q.reset();
        xl = xr = 0.0f; std::fill (preL.begin(), preL.end(), 0.0f); std::fill (preR.begin(), preR.end(), 0.0f); wp = 0;
        for (auto& q : xf) q.reset(); xd[0] = xd[1] = 0; xHist[0].fill (0.0f); xHist[1].fill (0.0f);
    }
    void process (float* l, float* r, int n, const Params& p)
    {
        if (p.mon <= 0) return;
        const int mk = p.mon * 1000 + (int) std::lround (p.colour * 100) * 7 + (int) std::lround (p.dist * 10) * 131 + (int) std::lround (p.toe * 100) * 17;
        if (mk != key) { key = mk; configure (p); }
        const float sideG = juce::jlimit (0.3f, 1.6f, p.angle / 60.0f);
        const float lvl = std::pow (10.0f, p.levelDb / 20.0f);
        static const float roomAmt[3] { 0.05f, 0.11f, 0.20f };
        static const float roomDec[3] { 0.20f, 0.38f, 0.66f };
        const float wet = roomAmt[juce::jlimit (0, 2, p.room)] * juce::jlimit (0.4f, 1.6f, 0.45f + 0.23f * p.dist);
        PlateEngine::Params pp; pp.decay = roomDec[juce::jlimit (0, 2, p.room)]; pp.damp = 0.65f; pp.lowCut = 0.30f; pp.width = 1.0f; pp.tone = 1;
        const int pre = juce::jlimit (1, (int) preL.size() - 1, (int) (p.dist / 343.0f * (float) sr * 0.6f));
        for (int base = 0; base < n; base += 512)
        {
            const int m = juce::jmin (512, n - base);
            for (int i = 0; i < m; ++i)
            {
                float a = l[base + i], b = r[base + i];
                const float mid = 0.5f * (a + b), side = 0.5f * (a - b) * sideG;
                a = mid + side; b = mid - side;
                for (auto& q : toeF) { a = q.process (a); }
                b = toeB.process (b);
                for (auto& q : mf[0]) a = q.process (a);
                for (auto& q : mf[1]) b = q.process (b);
                a = air[0].process (a); b = air[1].process (b);
                if (p.colour > 0.001f)
                {
                    for (auto& q : col[0]) a = q.process (a);
                    for (auto& q : col[1]) b = q.process (b);
                    const float s = 0.15f + 0.5f * p.colour;
                    a = a + p.colour * (softSat (a, s) - a); b = b + p.colour * (softSat (b, s) - b);
                }
                bufL[(size_t) i] = a; bufR[(size_t) i] = b;
                // pre-delayed feed for the room
                preL[(size_t) wp] = a; preR[(size_t) wp] = b;
                const size_t rp_ = (size_t) ((wp + (int) preL.size() - pre) % (int) preL.size());
                dL[(size_t) i] = preL[rp_]; dR[(size_t) i] = preR[rp_];
                wp = (wp + 1) % (int) preL.size();
            }
            plate.process (dL, dR, wetL.data(), wetR.data(), m, pp);
            for (int i = 0; i < m; ++i)
            {
                float a = bufL[(size_t) i] + wet * wetL[(size_t) i], b = bufR[(size_t) i] + wet * wetR[(size_t) i];
                if (p.phones)
                {
                    // crossfeed: a little low-passed, slightly delayed signal from the other ear
                    const float ca = xf[0].process (xHist[1][(size_t) xd[1]]), cb = xf[1].process (xHist[0][(size_t) xd[0]]);
                    xHist[0][(size_t) xd[0]] = a; xHist[1][(size_t) xd[1]] = b; xd[0] = (xd[0] + 1) % 12; xd[1] = (xd[1] + 1) % 12;
                    a = (a + 0.38f * ca) / 1.15f; b = (b + 0.38f * cb) / 1.15f;
                }
                if (p.mono) a = b = 0.5f * (a + b);
                l[base + i] = a * lvl; r[base + i] = b * lvl;
            }
        }
    }
private:
    void configure (const Params& p)
    {
        for (auto& c : mf) for (auto& q : c) q.setIdentity();
        for (int c = 0; c < 2; ++c)
        {
            auto& f = mf[c];
            switch (p.mon)
            {
                case 1: f[0].highpass (sr, 55, 0.75); f[1].highpass (sr, 55, 0.75); f[2].peak (sr, 3000, 0.9, 1.5); f[3].lowpass (sr, 19000, 0.7); break;
                case 2: f[0].highpass (sr, 40, 0.7); f[1].highpass (sr, 40, 0.7); f[2].peak (sr, 220, 0.8, 1.0); f[3].highShelf (sr, 9000, 0.7, -0.8); break;
                default: f[0].highpass (sr, 28, 0.7); f[1].lowShelf (sr, 80, 0.7, 2.0); f[2].highShelf (sr, 10000, 0.7, -1.5); break;
            }
            col[c][0].lowShelf (sr, 100, 0.7, 2.0 * p.colour); col[c][1].highShelf (sr, 9000, 0.7, -2.5 * p.colour);
            air[c].lowpass (sr, juce::jlimit (4000.0, 20000.0, 22000.0 / (1.0 + 0.30 * p.dist)), 0.6);
            xf[c].lowpass (sr, 700, 0.7);
        }
        const double toeDb = (p.toe - 0.5) * 6.0;
        for (auto& q : toeF) q.highShelf (sr, 5000, 0.7, toeDb);
        toeB.highShelf (sr, 5000, 0.7, toeDb);
    }
    double sr = 44100.0; int key = -1, wp = 0; int xd[2] { 0, 0 };
    PlateEngine plate;
    Biquad mf[2][4], col[2][2], air[2], toeF[1], toeB, xf[2];
    std::array<float, 12> xHist[2];
    float xl = 0, xr = 0;
    std::vector<float> bufL, bufR, wetL, wetR, preL, preR;
    float dL[512] {}, dR[512] {};
};

// Shared, process-wide note of how much low end the Bass tab is playing right now. A Drums tab running in
// the same host can listen to it when no sidechain signal is routed in. Blocks are published whole, so it
// can lag the bass by up to one block.
struct BassBus
{
    static std::atomic<float>& level() { static std::atomic<float> v { 0.0f }; return v; }
    static std::atomic<juce::uint32>& stamp() { static std::atomic<juce::uint32> v { 0 }; return v; }
    static bool fresh() { return juce::Time::getMillisecondCounter() - stamp().load() < 400u; }
};

// Sidechain version of the kick/bass lock: a low shelf (under ~100 Hz) on the drums that is pulled down while the bass plays.
class SidechainLock
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        det[0].lowpass (sr, 120, 0.707); det[1].lowpass (sr, 120, 0.707);
        aC = std::exp (-1.0f / (0.005f * (float) sr)); rC = std::exp (-1.0f / (0.150f * (float) sr)); gC = std::exp (-16.0f / (0.012f * (float) sr));
        reset();
    }
    void reset() { for (auto& q : shelf) q.setIdentity(), q.reset(); for (auto& q : det) q.reset(); env = 0.0f; redDb = 0.0f; cnt = 0; }
    // scL/scR: sample-accurate sidechain (may be null). Otherwise fixedLevel (linear, held for the block) drives the detector.
    // Returns the largest reduction in dB applied in this block.
    float process (float* l, float* r, int n, float amount, const float* scL, const float* scR, float fixedLevel)
    {
        const float depth = 14.0f * juce::jlimit (0.0f, 1.0f, amount);
        float worst = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            float x;
            if (scL != nullptr) { const float m = 0.5f * (scL[i] + (scR != nullptr ? scR[i] : scL[i])); x = std::abs (det[1].process (det[0].process (m))); }
            else x = fixedLevel;
            env = x > env ? x + (env - x) * aC : x + (env - x) * rC;
            if (++cnt >= 16)
            {
                cnt = 0;
                const float envDb = 20.0f * std::log10 (env + 1.0e-9f);
                const float target = depth * juce::jlimit (0.0f, 1.0f, (envDb + 48.0f) / 26.0f);
                redDb = target + (redDb - target) * gC;
                Biquad t;
                if (redDb > 0.05f) t.lowShelf (sr, 100.0, 0.7, -redDb); else t.setIdentity();
                for (auto& q : shelf) { q.b0 = t.b0; q.b1 = t.b1; q.b2 = t.b2; q.a1 = t.a1; q.a2 = t.a2; }
            }
            l[i] = shelf[0].process (l[i]);
            r[i] = shelf[1].process (r[i]);
            worst = std::max (worst, redDb);
        }
        return worst;
    }
private:
    double sr = 44100.0; float aC = 0, rC = 0, gC = 0, env = 0, redDb = 0; int cnt = 0;
    Biquad shelf[2], det[2];
};

class SteadyBass
{
public:
    struct Params { float amount = 0.5f, punch = 0.5f, weight = 0.5f; };

    void prepare (double sampleRate) { sr = sampleRate; reset(); }
    void reset()
    {
        dcL = dcR = lpL = lpR = 0.0f; env = 0.0f; refDb = -30.0f; primed = false; grDb = 0.0f; grLow = 0.0f; pin = pout = 0.0f; makeup = 1.0f; lastGr = 0.0f; warm = 0.0f; nAct = 0.0f;
    }

    // Level-follower design: the detector tracks each note's envelope, a slow "typical loud note" reference is learned
    // from the playing, and the gain moves notes toward it (quiet notes lifted, hard notes pulled down).
    // The gain is smoothed and frequency-weighted: the fundamental region (< ~220 Hz) moves less than the upper partials,
    // so evening out the level never thins the low end. No look-ahead, so there is no latency and no phase change.
    // returns the largest gain reduction (positive dB) in this block
    float process (float* l, float* r, int n, const Params& p)
    {
        const float cDc = onePoleCoef (1.0f / (6.2832f * 30.0f), sr);
        const float cEa = onePoleCoef (0.004f, sr), cEr = onePoleCoef (0.06f, sr);
        const float cRu = onePoleCoef (1.5f, sr), cRd = 0.0f;
        const float cLp = 1.0f - std::exp (-6.2832f * 220.0f / (float) sr);
        const float att = onePoleCoef ((6.0f + 40.0f * p.punch) * 0.001f, sr), rel = onePoleCoef (0.10f, sr);
        const float cLoud = onePoleCoef (3.0f, sr), cMk = onePoleCoef (1.2f, sr);
        const float ratio = 1.0f + p.amount * 8.0f, slope = 1.0f / ratio - 1.0f;
        const float maxLift = 3.0f + 9.0f * p.amount, knee = 4.0f;
        const float lowShare = 1.0f - 0.3f * p.weight;                       // the fundamental region moves a little less than the upper partials
        const float cSlow = onePoleCoef ((4.0f + 70.0f * p.weight) * 0.001f, sr);   // ...and arrives a little later, so the note's thump survives
        float maxGr = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            const float x0 = l[i], x1 = r[i];
            dcL += cDc * (x0 - dcL); dcR += cDc * (x1 - dcR);
            const float d = juce::jmax (std::abs (x0 - dcL), std::abs (x1 - dcR));
            const float pwr = d * d;
            env += (pwr > env ? cEa : cEr) * (pwr - env);                 // fast attack, note-length release
            const float levelDb = 10.0f * std::log10 (env + 1.0e-12f) + 3.01f;
            const bool active = levelDb > -55.0f;
            if (active)
            {
                if (! primed) { refDb = levelDb; primed = true; }
                else { nAct += 1.0f; refDb += juce::jmax (cRu, 1.0f / nAct) * (levelDb - refDb); }   // the player's typical level (plain average at first)
            }
            const float dev = levelDb - refDb;
            float target = 0.0f;
            if (dev > knee * 0.5f)        target = slope * (dev - knee * 0.5f) + slope * knee * 0.25f;
            else if (dev > -knee * 0.5f)  { const float t = dev + knee * 0.5f; target = slope * t * t / (2.0f * knee); }
            else                          target = juce::jmin (maxLift, (-dev - knee * 0.5f) * (1.0f - 1.0f / ratio) + (1.0f - 1.0f / ratio) * knee * 0.25f);
            if (! active) target = 0.0f;
            else if (levelDb < -48.0f) target *= (levelDb + 55.0f) / 7.0f; // fade the lift out into the noise floor
            grDb += (target < grDb ? att : rel) * (target - grDb);
            grLow += cSlow * (grDb - grLow);
            const float gHigh = dbToGain (grDb), gLow = dbToGain (grLow * lowShare);
            lpL += cLp * (x0 - lpL); lpR += cLp * (x1 - lpR);
            const float y0 = lpL * gLow + (x0 - lpL) * gHigh, y1 = lpR * gLow + (x1 - lpR) * gHigh;
            // long-term loudness match (in either direction), so "active" never just means "louder"
            warm += 1.0f;                                                      // start-up: plain averages until the filters have settled
            const float cl = juce::jmax (cLoud, 1.0f / warm), cm = cMk;
            pin += cl * (0.5f * (x0 * x0 + x1 * x1) - pin); pout += cl * (0.5f * (y0 * y0 + y1 * y1) - pout);
            if (pin > 1.0e-9f) { const float want = juce::jlimit (0.6f, 1.8f, std::sqrt (pin / (pout + 1.0e-12f))); makeup += cm * (want - makeup); }
            l[i] = y0 * makeup; r[i] = y1 * makeup;
            maxGr = juce::jmax (maxGr, -grDb);
        }
        lastGr = maxGr; return maxGr;
    }
    float lastGainReduction() const noexcept { return lastGr; }
private:
    double sr = 48000.0; bool primed = false;
    float dcL = 0, dcR = 0, lpL = 0, lpR = 0, env = 0, refDb = -30.0f, grDb = 0, grLow = 0, pin = 0, pout = 0, makeup = 1.0f, lastGr = 0, warm = 0, nAct = 0;
};

} // namespace riddim
