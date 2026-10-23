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
    struct Params { float sat = 0, tone = 1, wow = 0, flutter = 0, wear = 0; int type = 0; bool on = true; };

    static constexpr float kBaseDelaySeconds = 0.0020f;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        baseDelay = kBaseDelaySeconds * (float) sr;
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
        wowPh = flutPh = flut2Ph = 0.0f; cachedType = -1; cachedWear = -1; cachedTone = -1;
    }

    void process (float* l, float* r, int n, const Params& p)
    {
        const float typeWow = p.type == 2 ? 1.6f : (p.type == 1 ? 1.15f : 1.0f);
        const float wowDepth = p.on ? p.wow * typeWow * 0.0009f * (float) sr : 0.0f;          // samples (about +-0.4% pitch at max)
        const float fluDepth = p.on ? p.flutter * typeWow * 0.00006f * (float) sr : 0.0f;
        const float wowInc = 2.0f * kPi * 0.65f / (float) sr, fluInc = 2.0f * kPi * 9.3f / (float) sr, flu2Inc = 2.0f * kPi * 13.7f / (float) sr;

        if (p.type != cachedType || p.wear != cachedWear || p.tone != cachedTone) retune (p);
        const float drive = p.sat * (p.type == 0 ? 0.85f : 1.0f);
        const float bias = p.type == 0 ? 0.0f : 0.10f;                     // asymmetry = even harmonics
        const float blend = p.on ? juce::jmin (1.0f, p.sat * 1.6f) : 0.0f;
        const float hiss = p.on ? p.wear * (p.type == 2 ? 0.0030f : 0.0016f) : 0.0f;

        for (int i = 0; i < n; ++i)
        {
            wowPh += wowInc; if (wowPh > 2.0f * kPi) wowPh -= 2.0f * kPi;
            flutPh += fluInc; if (flutPh > 2.0f * kPi) flutPh -= 2.0f * kPi;
            flut2Ph += flu2Inc; if (flut2Ph > 2.0f * kPi) flut2Ph -= 2.0f * kPi;
            const float mod = wowDepth * std::sin (wowPh) + fluDepth * (0.7f * std::sin (flutPh) + 0.3f * std::sin (flut2Ph));
            float* io[2] { l + i, r + i };
            for (int c = 0; c < 2; ++c)
            {
                float x = *io[c];
                // wow / flutter: read the delay line at a moving position
                auto& buf = line[(size_t) c];
                buf[(size_t) pos] = x;
                float rd = (float) pos - (baseDelay + mod);
                while (rd < 0) rd += (float) size;
                while (rd >= (float) size) rd -= (float) size;
                const int i0 = juce::jlimit (0, size - 1, (int) rd); const float fr = juce::jlimit (0.0f, 1.0f, rd - (float) i0);
                const int i1 = (i0 + 1) % size;
                x = buf[(size_t) i0] * (1.0f - fr) + buf[(size_t) i1] * fr;

                if (p.on)
                {
                    if (p.type != 0) x = bump[(size_t) c].process (x);                  // head bump
                    const float sat = (std::tanh ((x + bias) * (1.0f + drive * 7.0f)) - std::tanh (bias * (1.0f + drive * 7.0f)))
                                      / ((1.0f + drive * 7.0f) * (1.0f - std::pow (std::tanh (bias * (1.0f + drive * 7.0f)), 2.0f)));
                    x = x + (sat - x) * blend;
                    x = toneLp[(size_t) c].process (x);
                    if (p.wear > 0.001f) x = wearLp[(size_t) c].process (x);
                    if (hiss > 0.0f) x += hiss * (rng.nextFloat() * 2.0f - 1.0f);
                }
                *io[c] = x;
            }
            if (++pos >= size) pos = 0;
        }
    }

private:
    void retune (const Params& p)
    {
        cachedType = p.type; cachedWear = p.wear; cachedTone = p.tone;
        const double toneHz = 2200.0 * std::pow (20000.0 / 2200.0, (double) p.tone);
        const double maxHz = sr * 0.45;
        for (auto& f : toneLp) f.lowpass (sr, juce::jmin (toneHz, maxHz), 0.707);
        const double wearHz = p.wear > 0.001f ? 16000.0 * std::pow (4500.0 / 16000.0, (double) p.wear) * (p.type == 2 ? 0.8 : 1.0) : maxHz;
        for (auto& f : wearLp) f.lowpass (sr, juce::jmin (wearHz, maxHz), 0.707);
        for (auto& f : bump) f.lowShelf (sr, 90.0, 0.8, p.type == 2 ? 2.0 : 2.5);
    }

    double sr = 48000.0; float baseDelay = 72.0f; int size = 300, pos = 0;
    std::array<std::vector<float>, 2> line;
    std::array<Biquad, 2> toneLp, wearLp, bump;
    float wowPh = 0, flutPh = 0, flut2Ph = 0;
    int cachedType = -1; float cachedWear = -1, cachedTone = -1;
    juce::Random rng { 1234 };
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
    }
    void reset()
    {
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
                const float delayed = buf[(size_t) c][(size_t) i0] * (1.0f - fr) + buf[(size_t) c][(size_t) i1] * fr;
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
//  REVERB: Spring / Plate / Hall / Room / Dub built on the JUCE reverb core,
//  with pre-delay, per-type tone shaping and a dispersive "drip" for the spring.
//==============================================================================
class ReverbStage
{
public:
    struct Params { int type = 0; float preMs = 20.0f, decay = 0.4f, mix = 0.0f; };

    void prepare (double sampleRate)
    {
        sr = sampleRate; rev.setSampleRate (sampleRate);
        preSize = (int) (sampleRate * 0.13) + 8;
        for (auto& b : pre) b.assign ((size_t) preSize, 0.0f);
        reset();
    }
    void reset()
    {
        rev.reset();
        for (auto& b : pre) std::fill (b.begin(), b.end(), 0.0f);
        preWrite = 0; cachedType = -1;
        for (auto& a : ap) for (auto& s : a) s = 0.0f;
        for (auto& f : inHp) f.reset(); for (auto& f : inLp) f.reset();
        for (auto& f : outLp) f.reset(); for (auto& f : outShelf) f.reset(); for (auto& f : outHp) f.reset();
        mixS.setCurrentAndTargetValue (0.0f); first = true;
    }

    void process (float* l, float* r, int n, const Params& p)
    {
        if (p.type != cachedType) retune (p.type);
        if (first) { mixS.reset (sr, 0.03); mixS.setCurrentAndTargetValue (p.mix); first = false; }
        mixS.setTargetValue (p.mix);
        if ((int) wl.size() < n) { wl.assign ((size_t) n, 0.0f); wr.assign ((size_t) n, 0.0f); }

        // decay knob -> room size / damping per type
        juce::Reverb::Parameters rp;
        const float d = juce::jlimit (0.0f, 1.0f, p.decay);
        switch (p.type)
        {
            case 0: rp.roomSize = 0.30f + 0.40f * d; rp.damping = 0.55f; break;           // spring
            case 1: rp.roomSize = 0.55f + 0.38f * d; rp.damping = 0.18f; break;           // plate
            case 2: rp.roomSize = 0.70f + 0.29f * d; rp.damping = 0.38f; break;           // hall
            case 3: rp.roomSize = 0.15f + 0.40f * d; rp.damping = 0.50f; break;           // room
            default: rp.roomSize = 0.72f + 0.27f * d; rp.damping = 0.82f; break;          // dub
        }
        rp.wetLevel = 0.33f; rp.dryLevel = 0.0f; rp.width = 1.0f; rp.freezeMode = 0.0f;
        rev.setParameters (rp);

        const int preSamples = juce::jlimit (0, preSize - 2, (int) (p.preMs * 0.001f * (float) sr));
        float* io[2] { l, r };
        float* w[2] { wl.data(), wr.data() };
        for (int i = 0; i < n; ++i)
        {
            for (int c = 0; c < 2; ++c)
            {
                auto& b = pre[(size_t) c];
                b[(size_t) preWrite] = io[c][i];
                int rd = preWrite - preSamples; if (rd < 0) rd += preSize;
                float x = b[(size_t) rd];
                x = inLp[(size_t) c].process (inHp[(size_t) c].process (x));
                if (p.type == 0)                                  // spring: dispersive chain gives the "boing"
                    for (int s = 0; s < kApStages; ++s)
                    {
                        const float g = 0.62f, y = -g * x + ap[(size_t) c][(size_t) s];
                        ap[(size_t) c][(size_t) s] = x + g * y;
                        x = y;
                    }
                w[c][i] = x;
            }
            if (++preWrite >= preSize) preWrite = 0;
        }
        rev.processStereo (w[0], w[1], n);
        for (int i = 0; i < n; ++i)
        {
            const float m = mixS.getNextValue();
            for (int c = 0; c < 2; ++c)
            {
                const float x = outShelf[(size_t) c].process (outLp[(size_t) c].process (outHp[(size_t) c].process (w[c][i])));
                io[c][i] = io[c][i] * (1.0f - m * m) + x * m * 1.4f;
            }
        }
    }

private:
    static constexpr int kApStages = 16;
    void retune (int type)
    {
        cachedType = type;
        for (int c = 0; c < 2; ++c)
        {
            inHp[(size_t) c].highpass (sr, type == 1 ? 180.0 : (type == 0 ? 220.0 : 90.0), 0.707);
            inLp[(size_t) c].lowpass (sr, type == 4 ? 4200.0 : (type == 0 ? 4800.0 : 12000.0), 0.707);
            outHp[(size_t) c].highpass (sr, type == 4 ? 120.0 : 60.0, 0.707);
            outLp[(size_t) c].lowpass (sr, type == 4 ? 3200.0 : (type == 0 ? 5200.0 : 14000.0), 0.707);
            outShelf[(size_t) c].highShelf (sr, 5000.0, 0.707, type == 1 ? 3.0 : (type == 4 ? -3.0 : 0.0));
        }
    }
    double sr = 48000.0; int preSize = 0, preWrite = 0, cachedType = -1; bool first = true;
    juce::Reverb rev;
    std::array<std::vector<float>, 2> pre;
    std::vector<float> wl, wr;
    std::array<std::array<float, kApStages>, 2> ap {};
    std::array<Biquad, 2> inHp, inLp, outHp, outLp, outShelf;
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

} // namespace riddim
