#include "PluginEditor.h"
#include "BinaryData.h"
#include <array>

//==============================================================================
namespace
{
    const juce::Colour kBrassHi (0xffe9c36b);
    const juce::Colour kBrass   (0xffb98538);
    const juce::Colour kBrassLo (0xff6d4b1d);
    const juce::Colour kGold    (0xffffd071);
    const juce::Colour kCream   (0xfff3e5c3);
    const juce::Colour kMuted   (0xffa5b0a2);
    const juce::Colour kName    (0xffd9c28a);
    const juce::Colour kInk     (0xff060a07);
    const juce::Colour kAmber   (0xffffa726);

    const char* const kSectionIds[7]    { "master", "drums", "guitar", "bass", "keys", "horns", "vocal" };
    const char* const kSectionNames[7]  { "MASTER", "DRUMS", "GUITAR", "BASS", "KEYS", "HORNS", "VOCAL" };
    const char* const kToneRole[7]      { "MASTER SHEEN", "ATTACK / BODY", "SKANK / PICK", "LOW-PASS WEIGHT",
                                          "KEYS SHEEN", "HORN PRESENCE", "VOCAL TONE" };

    struct KnobDef { const char* paramId; const char* name; int fmt; };   // fmt: 0 percent, 1 Hz, 2 ms
    const KnobDef kKnobDefs[19] =
    {
        { "tapeSat",   "SATURATION",     0 }, { "tapeTone", "TAPE TONE", 0 }, { "wow", "WOW", 0 },
        { "flutter",   "FLUTTER",        0 }, { "wear",     "TAPE WEAR", 0 },
        { "cutoff",    "CUTOFF",         1 }, { "resonance", "RESONANCE", 0 },
        { "time",      "FREE TIME",      2 }, { "feedback", "FEEDBACK",  0 }, { "mix", "WET / DRY", 0 },
        { "drive",     "TAPE HEAT",      0 }, { "width",    "STEREO OFFSET", 0 }, { "damping", "DAMPING", 0 },
        { nullptr,     "DRIVE",          0 }, { nullptr,    "TONE",      0 }, { nullptr, "COMP", 0 },
        { "guitarChopTightness",  "CHOP TIGHTNESS",  0 },
        { "guitarPickBrightness", "PICK BRIGHTNESS", 0 },
        { "guitarRhythmBody",     "RHYTHM BODY",     0 }
    };

    // 13 values per preset, in the order of kPresetIds. A value below zero means
    // "leave this parameter as the user has it".
    const char* const kPresetIds[13] { "feedback", "mix", "cutoff", "resonance", "drive", "time",
                                       "tapeSat", "tapeTone", "wow", "flutter", "wear", "width", "damping" };
    struct PresetDef { const char* name; const char* sub; float v[13]; };
    const PresetDef kPresets[8] =
    {
        { "KINGSTON SESSION",   "Tape warmth, medium echo",
          { 0.42f, 0.26f, 2800.f, 0.35f, 0.28f, 360.f, 0.18f, 0.72f, 0.05f, 0.04f, 0.10f, -1.f, -1.f } },
        { "FOUNDATION RIDDIMS", "Filtered skank echo",
          { 0.52f, 0.31f, 2100.f, 0.48f, 0.36f, 290.f, 0.32f, 0.60f, 0.08f, 0.06f, 0.20f, -1.f, -1.f } },
        { "ROOTS & CULTURE",    "Dark tape, long repeats",
          { 0.38f, 0.22f, 3200.f, 0.30f, 0.22f, 420.f, 0.30f, 0.55f, 0.10f, 0.06f, 0.25f, 0.18f, 0.50f } },
        { "DUB SESSIONS",       "Runaway feedback dub",
          { 0.74f, 0.36f, 3600.f, 0.62f, 0.45f, 480.f, 0.42f, 0.68f, 0.12f, 0.09f, 0.30f, -1.f, -1.f } },
        { "LOVERS ROCK",        "Soft, bright repeats",
          { 0.34f, 0.24f, 4500.f, 0.25f, 0.18f, 520.f, 0.15f, 0.80f, 0.06f, 0.03f, 0.05f, 0.22f, 0.35f } },
        { "ROCKSTEADY",         "Short, clean slap",
          { 0.30f, 0.20f, 5200.f, 0.20f, 0.20f, 300.f, 0.22f, 0.70f, 0.04f, 0.03f, 0.08f, 0.10f, 0.40f } },
        { "DANCEHALL VIBES",    "Bright, quick echoes",
          { 0.48f, 0.30f, 6500.f, 0.40f, 0.30f, 250.f, 0.20f, 0.85f, 0.03f, 0.02f, 0.00f, 0.25f, 0.30f } },
        { "VOCAL TOOLS",        "Subtle vocal echo",
          { 0.32f, 0.22f, 2400.f, 0.22f, 0.12f, 340.f, 0.10f, 0.75f, 0.02f, 0.02f, 0.03f, 0.20f, 0.55f } }
    };

    void setSliderFormat (juce::Slider& s, int fmt)
    {
        if (fmt == 0)
        {
            s.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + "%"; };
            s.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue() / 100.0; };
        }
        else if (fmt == 1)
        {
            s.textFromValueFunction = [] (double v)
            {
                return v >= 1000.0 ? juce::String (v / 1000.0, 1) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz";
            };
            s.valueFromTextFunction = [] (const juce::String& t)
            {
                const double x = t.getDoubleValue();
                return t.containsIgnoreCase ("k") ? x * 1000.0 : x;
            };
        }
        else
        {
            s.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v)) + " ms"; };
            s.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue(); };
        }
        s.updateText();
    }

    juce::Point<float> polar (juce::Point<float> c, float angle, float radius)
    {
        return { c.x + std::sin (angle) * radius, c.y - std::cos (angle) * radius };
    }

    //------------------------------------------------------------------ panels
    enum PanelStyle { Green, Brown, Dark };

    void drawWoodFrame (juce::Graphics& g, juce::Rectangle<int> r)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5a361b), 0.0f, (float) r.getY(),
                                                 juce::Colour (0xff2a170a), 0.0f, (float) r.getBottom(), false));
        g.fillRect (r);
        juce::Random rng (1234);
        for (int i = 0; i < 180; ++i)
        {
            const float y = rng.nextFloat() * (float) r.getHeight();
            const float x = rng.nextFloat() * (float) r.getWidth() - 60.0f;
            const float len = 80.0f + rng.nextFloat() * 520.0f;
            g.setColour ((i % 3 == 0 ? juce::Colours::black : juce::Colour (0xff8a5a2c))
                             .withAlpha (0.05f + rng.nextFloat() * 0.10f));
            g.drawLine (x, y, x + len, y + (rng.nextFloat() - 0.5f) * 3.0f, 0.6f + rng.nextFloat() * 1.2f);
        }
    }

    void drawScrew (juce::Graphics& g, juce::Point<float> c, float seed)
    {
        const float r = 4.2f;
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillEllipse (c.x - r, c.y - r + 1.2f, r * 2.0f, r * 2.0f);
        g.setGradientFill (juce::ColourGradient (kBrassHi, c.x - r, c.y - r, kBrassLo, c.x + r, c.y + r, false));
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        g.setColour (kInk.withAlpha (0.85f));
        g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 0.8f);
        const float a = seed * 2.4f;
        g.drawLine (c.x - std::cos (a) * r * 0.7f, c.y - std::sin (a) * r * 0.7f,
                    c.x + std::cos (a) * r * 0.7f, c.y + std::sin (a) * r * 0.7f, 1.2f);
    }

    void drawPanel (juce::Graphics& g, juce::Rectangle<int> ri, const juce::String& title,
                    PanelStyle style, const juce::Image& grain)
    {
        const auto r = ri.toFloat();
        juce::Colour top, bot;
        if (style == Green)      { top = juce::Colour (0xff23432f); bot = juce::Colour (0xff0e2017); }
        else if (style == Brown) { top = juce::Colour (0xff4a3320); bot = juce::Colour (0xff22160d); }
        else                     { top = juce::Colour (0xff14211a); bot = juce::Colour (0xff070d09); }

        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (r.translated (0.0f, 3.0f), 7.0f);
        g.setGradientFill (juce::ColourGradient (top, 0.0f, r.getY(), bot, 0.0f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 7.0f);

        if (grain.isValid())
        {
            g.setTiledImageFill (grain, 0, 0, 1.0f);
            g.fillRoundedRectangle (r, 7.0f);
        }

        // soft vignette so panels read as slightly dished
        g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, r.getCentreX(), r.getCentreY(),
                                                 juce::Colours::black.withAlpha (0.38f), r.getX(), r.getY(), true));
        g.fillRoundedRectangle (r, 7.0f);

        // inset shadow + brass trim
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawRoundedRectangle (r.reduced (3.0f), 5.0f, 2.0f);
        g.setGradientFill (juce::ColourGradient (kBrassHi, r.getX(), r.getY(), kBrassLo, r.getRight(), r.getBottom(), false));
        g.drawRoundedRectangle (r.reduced (1.0f), 7.0f, 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawRoundedRectangle (r.reduced (3.5f), 5.0f, 0.8f);

        drawScrew (g, { r.getX() + 9.0f,     r.getY() + 9.0f },      1.0f);
        drawScrew (g, { r.getRight() - 9.0f, r.getY() + 9.0f },      2.0f);
        drawScrew (g, { r.getX() + 9.0f,     r.getBottom() - 9.0f }, 3.0f);
        drawScrew (g, { r.getRight() - 9.0f, r.getBottom() - 9.0f }, 4.0f);

        if (title.isNotEmpty())
        {
            const auto textArea = juce::Rectangle<int> (ri.getX() + 18, ri.getY() + 9, ri.getWidth() - 36, 22);
            g.setFont (RiddimLookAndFeel::font (17.0f));
            g.setColour (juce::Colours::black.withAlpha (0.8f));
            g.drawText (title, textArea.translated (1, 1), juce::Justification::centredLeft, true);
            g.setColour (kGold);
            g.drawText (title, textArea, juce::Justification::centredLeft, true);
            g.setColour (kBrass.withAlpha (0.35f));
            g.fillRect (ri.getX() + 18, ri.getY() + 33, ri.getWidth() - 36, 1);
        }
    }

    //------------------------------------------------------------------ animated parts
    void drawVU (juce::Graphics& g, juce::Rectangle<float> r, float needle)
    {
        const auto bz = r.expanded (5.0f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (bz.translated (0.0f, 2.0f), 8.0f);
        g.setGradientFill (juce::ColourGradient (kBrassHi, bz.getX(), bz.getY(), kBrassLo, bz.getRight(), bz.getBottom(), false));
        g.fillRoundedRectangle (bz, 8.0f);
        g.setColour (juce::Colour (0xff120c06));
        g.fillRoundedRectangle (bz.reduced (2.5f), 6.5f);

        juce::Graphics::ScopedSaveState saved (g);
        juce::Path clip;
        clip.addRoundedRectangle (r, 5.0f);
        g.reduceClipRegion (clip);

        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffbe7ae), r.getCentreX(), r.getBottom(),
                                                 juce::Colour (0xffc58d36), r.getX(), r.getY(), true));
        g.fillRect (r);

        const float px = r.getCentreX();
        const float py = r.getBottom() + 16.0f;
        const float R  = r.getHeight();
        const juce::Point<float> pivot (px, py);
        auto angleFor = [] (float f) { return juce::jmap (f, 0.0f, 1.0f, -0.92f, 0.92f); };
        auto fracForDb = [] (float db) { return (db + 40.0f) / 43.0f; };

        // scale arc and red zone
        juce::Path arc;
        arc.addCentredArc (px, py, R * 0.86f, R * 0.86f, 0.0f, angleFor (0.0f), angleFor (1.0f), true);
        g.setColour (juce::Colour (0xff2a1a0c));
        g.strokePath (arc, juce::PathStrokeType (1.3f));
        juce::Path red;
        red.addCentredArc (px, py, R * 0.86f, R * 0.86f, 0.0f, angleFor (fracForDb (-3.0f)), angleFor (1.0f), true);
        g.setColour (juce::Colour (0xffc2261a));
        g.strokePath (red, juce::PathStrokeType (3.0f));

        struct Tick { float db; const char* label; };
        const Tick ticks[] = { { -40.0f, "-40" }, { -35.0f, nullptr }, { -30.0f, "-30" }, { -25.0f, nullptr },
                               { -20.0f, "-20" }, { -15.0f, nullptr }, { -10.0f, "-10" }, { -6.0f, nullptr },
                               { -3.0f, nullptr }, { 0.0f, "0" }, { 3.0f, nullptr } };
        g.setFont (RiddimLookAndFeel::font (9.0f));
        for (auto& t : ticks)
        {
            const float a = angleFor (fracForDb (t.db));
            const bool major = t.label != nullptr;
            const auto p1 = polar (pivot, a, R * (major ? 0.78f : 0.82f));
            const auto p2 = polar (pivot, a, R * 0.86f);
            g.setColour (t.db > -3.5f ? juce::Colour (0xffa32016) : juce::Colour (0xff2a1a0c));
            g.drawLine (p1.x, p1.y, p2.x, p2.y, major ? 1.5f : 1.0f);
            if (major)
            {
                const auto lp = polar (pivot, a, R * 0.68f);
                g.drawText (juce::String (t.label), juce::Rectangle<float> (lp.x - 12.0f, lp.y - 6.0f, 24.0f, 12.0f),
                            juce::Justification::centred, false);
            }
        }
        g.setColour (juce::Colour (0xff2a1a0c));
        g.setFont (RiddimLookAndFeel::font (10.0f));
        g.drawText ("PEAK dB", juce::Rectangle<float> (px - 36.0f, r.getBottom() - 28.0f, 72.0f, 12.0f),
                    juce::Justification::centred, false);

        // needle
        const float a = angleFor (juce::jlimit (0.0f, 1.0f, needle));
        const auto tip = polar (pivot, a, R * 0.90f);
        g.setColour (juce::Colours::black.withAlpha (0.30f));
        g.drawLine (px + 2.5f, py + 2.0f, tip.x + 2.5f, tip.y + 2.0f, 1.8f);
        g.setColour (juce::Colour (0xff7a1a10));
        g.drawLine (px, py, tip.x, tip.y, 1.7f);
        g.setColour (juce::Colour (0xff2a1a0c));
        g.fillEllipse (px - 11.0f, py - 11.0f, 22.0f, 22.0f);
        g.setColour (kBrass);
        g.drawEllipse (px - 11.0f, py - 11.0f, 22.0f, 22.0f, 1.5f);

        // glass
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.22f), r.getX(), r.getY(),
                                                 juce::Colours::transparentWhite, r.getX(), r.getCentreY(), false));
        g.fillRect (r);
        g.setColour (juce::Colours::black.withAlpha (0.40f));
        g.drawRoundedRectangle (r, 5.0f, 3.0f);
    }

    void drawReel (juce::Graphics& g, juce::Point<float> c, float radius, float angle)
    {
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillEllipse (c.x - radius, c.y - radius + 3.0f, radius * 2.0f, radius * 2.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe0bb70), c.x - radius, c.y - radius,
                                                 juce::Colour (0xff5c4018), c.x + radius, c.y + radius, false));
        g.fillEllipse (c.x - radius, c.y - radius, radius * 2.0f, radius * 2.0f);

        const float ir = radius * 0.80f;
        g.setColour (juce::Colour (0xff0d0a07));
        g.fillEllipse (c.x - ir, c.y - ir, ir * 2.0f, ir * 2.0f);

        juce::Path wedge;
        wedge.startNewSubPath (0.0f, 0.0f);
        wedge.addCentredArc (0.0f, 0.0f, ir * 0.96f, ir * 0.96f, 0.0f, -0.62f, 0.62f, false);
        wedge.closeSubPath();
        for (int k = 0; k < 3; ++k)
        {
            const auto t = juce::AffineTransform::rotation (angle + (float) k * juce::MathConstants<float>::twoPi / 3.0f)
                               .translated (c.x, c.y);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffc9a05a), c.x - ir, c.y - ir,
                                                     juce::Colour (0xff6a4c20), c.x + ir, c.y + ir, false));
            g.fillPath (wedge, t);
        }
        g.setColour (juce::Colour (0xff1a120a));
        g.fillEllipse (c.x - radius * 0.24f, c.y - radius * 0.24f, radius * 0.48f, radius * 0.48f);
        g.setColour (kBrass);
        g.drawEllipse (c.x - radius * 0.24f, c.y - radius * 0.24f, radius * 0.48f, radius * 0.48f, 1.4f);
        g.setColour (kInk.withAlpha (0.8f));
        g.drawEllipse (c.x - radius, c.y - radius, radius * 2.0f, radius * 2.0f, 1.2f);
    }

    void drawLamp (juce::Graphics& g, juce::Point<float> c, float r, float brightness, juce::Colour col)
    {
        if (brightness > 0.05f)
        {
            g.setGradientFill (juce::ColourGradient (col.withAlpha (0.45f * brightness), c.x, c.y,
                                                     juce::Colours::transparentBlack, c.x + r * 2.6f, c.y, true));
            g.fillEllipse (c.x - r * 2.6f, c.y - r * 2.6f, r * 5.2f, r * 5.2f);
        }
        const auto lit = col.interpolatedWith (juce::Colour (0xfffff0c0), 0.35f * brightness);
        const auto dark = juce::Colour (0xff1a1208);
        g.setGradientFill (juce::ColourGradient (dark.interpolatedWith (lit, brightness), c.x - r, c.y - r,
                                                 dark.interpolatedWith (col.darker (0.5f), brightness), c.x + r, c.y + r, false));
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        g.setColour (kInk);
        g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 1.2f);
    }
}

//==============================================================================
RiddimLookAndFeel::RiddimLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff0f1a14));
    setColour (juce::PopupMenu::textColourId, kCream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, kBrass);
    setColour (juce::PopupMenu::highlightedTextColourId, kInk);
    setColour (juce::ComboBox::textColourId, kCream);
    setColour (juce::Slider::textBoxTextColourId, kCream);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff0b130e));
    setColour (juce::Slider::textBoxOutlineColourId, kBrassLo);
}

juce::Font RiddimLookAndFeel::font (float size, bool bold)
{
    return juce::Font (juce::FontOptions ("Avenir Next Condensed", size, bold ? juce::Font::bold : juce::Font::plain));
}

juce::Font RiddimLookAndFeel::getComboBoxFont (juce::ComboBox&)  { return font (13.5f); }
juce::Font RiddimLookAndFeel::getPopupMenuFont()                 { return font (14.0f); }

juce::Font RiddimLookAndFeel::getLabelFont (juce::Label& l)
{
    if (dynamic_cast<juce::Slider*> (l.getParentComponent()) != nullptr)
        return font (12.5f);
    return l.getFont();
}

void RiddimLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                          float pos, float startAngle, float endAngle, juce::Slider& slider)
{
    const bool dim = ! slider.isEnabled();
    if (dim)
        g.beginTransparencyLayer (0.45f);

    const juce::Rectangle<float> bounds ((float) x, (float) y, (float) width, (float) height);
    const float d = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto area = bounds.withSizeKeepingCentre (d, d).reduced (d * 0.03f);
    const auto c = area.getCentre();
    const float outerR = area.getWidth() * 0.5f;
    const float knobR = outerR * 0.66f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    // scale ticks
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
        const auto p1 = polar (c, a, outerR * (i % 5 == 0 ? 0.82f : 0.88f));
        const auto p2 = polar (c, a, outerR * 0.98f);
        g.setColour (kBrass.withAlpha (0.9f));
        g.drawLine (p1.x, p1.y, p2.x, p2.y, i % 5 == 0 ? 1.8f : 1.1f);
    }

    // value arc
    juce::Path track;
    track.addCentredArc (c.x, c.y, outerR * 0.74f, outerR * 0.74f, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff050806).withAlpha (0.85f));
    g.strokePath (track, juce::PathStrokeType (3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    juce::Path val;
    val.addCentredArc (c.x, c.y, outerR * 0.74f, outerR * 0.74f, 0.0f, startAngle, angle, true);
    g.setColour (kAmber);
    g.strokePath (val, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // drop shadow and skirt
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillEllipse (c.x - knobR, c.y - knobR + knobR * 0.14f, knobR * 2.0f, knobR * 2.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a2a14), c.x - knobR, c.y - knobR,
                                             juce::Colour (0xff0a0704), c.x + knobR, c.y + knobR, false));
    g.fillEllipse (c.x - knobR, c.y - knobR, knobR * 2.0f, knobR * 2.0f);

    // brass cap
    const float capR = knobR * 0.88f;
    juce::ColourGradient cap (kBrassHi.brighter (0.25f), c.x - capR * 0.6f, c.y - capR * 0.7f,
                              kBrassLo, c.x + capR * 0.7f, c.y + capR * 0.8f, false);
    cap.addColour (0.45, kBrass);
    g.setGradientFill (cap);
    g.fillEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f);
    for (int k = 1; k <= 4; ++k)
    {
        const float rr = capR * (float) k / 5.0f;
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.drawEllipse (c.x - rr, c.y - rr, rr * 2.0f, rr * 2.0f, 0.6f);
    }
    juce::Path highlight;
    highlight.addCentredArc (c.x, c.y, capR * 0.86f, capR * 0.86f, 0.0f, -2.6f, -0.9f, true);
    g.setColour (juce::Colours::white.withAlpha (0.38f));
    g.strokePath (highlight, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (kInk.withAlpha (0.75f));
    g.drawEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f, 1.2f);

    // pointer
    juce::Path pointer;
    pointer.addRoundedRectangle (-1.9f, -capR * 0.86f, 3.8f, capR * 0.46f, 1.5f);
    const auto xf = juce::AffineTransform::rotation (angle).translated (c.x, c.y);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillPath (pointer, xf.translated (0.8f, 1.2f));
    g.setColour (juce::Colour (0xff1a0f05));
    g.fillPath (pointer, xf);

    if (dim)
        g.endTransparencyLayer();
}

void RiddimLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                          bool highlighted, bool)
{
    const auto r = b.getLocalBounds().toFloat();
    const auto plate = r.reduced (1.0f);
    const bool on = b.getToggleState();

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff27392e), 0.0f, plate.getY(),
                                             juce::Colour (0xff101a14), 0.0f, plate.getBottom(), false));
    g.fillRoundedRectangle (plate, 5.0f);
    g.setColour (kBrassLo.brighter (highlighted ? 0.45f : 0.1f));
    g.drawRoundedRectangle (plate, 5.0f, 1.3f);

    const float ld = juce::jmin (r.getHeight() * 0.5f, 18.0f);
    const juce::Point<float> lc (plate.getX() + 14.0f + ld * 0.5f, r.getCentreY());
    drawLamp (g, lc, ld * 0.5f, on ? 1.0f : 0.0f, kAmber);

    g.setColour (on ? kCream : kMuted);
    g.setFont (font (13.0f));
    g.drawText (b.getButtonText(), plate.withTrimmedLeft (ld + 28.0f).toNearestInt(),
                juce::Justification::centredLeft, true);
}

void RiddimLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                              bool highlighted, bool down)
{
    const auto role = b.getProperties()["role"].toString();
    const auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    const float corner = 4.5f;

    juce::Colour top = on ? juce::Colour (0xfff6cf6e) : juce::Colour (0xff273a2f);
    juce::Colour bot = on ? juce::Colour (0xffb27a28) : juce::Colour (0xff101a14);
    if (highlighted && ! on) { top = top.brighter (0.18f); bot = bot.brighter (0.12f); }
    if (down) { top = top.darker (0.25f); bot = bot.darker (0.25f); }

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r.translated (0.0f, 1.5f), corner);
    g.setGradientFill (juce::ColourGradient (top, 0.0f, r.getY(), bot, 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, corner);
    g.setColour (juce::Colours::white.withAlpha (on ? 0.35f : 0.08f));
    g.drawRoundedRectangle (r.reduced (1.2f), corner, 1.0f);
    g.setColour (on ? kBrassLo.darker() : kBrassLo.brighter (0.15f));
    g.drawRoundedRectangle (r, corner, 1.3f);
    if (on && role != "lib")
    {
        g.setColour (kGold.withAlpha (0.20f));
        g.drawRoundedRectangle (r.expanded (1.5f), corner + 1.0f, 2.0f);
    }
}

void RiddimLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    auto r = b.getLocalBounds();
    const bool on = b.getToggleState();
    const auto role = b.getProperties()["role"].toString();

    if (role == "lib")
    {
        const auto num = b.getProperties()["num"].toString();
        const auto sub = b.getProperties()["sub"].toString();
        auto left = r.removeFromLeft (46);
        g.setColour (on ? kInk : kGold);
        g.setFont (font (26.0f));
        g.drawText (num, left, juce::Justification::centred, false);
        auto text = r.reduced (2, 12);
        auto nameArea = text.removeFromTop (text.getHeight() / 2);
        g.setColour (on ? kInk : kCream);
        g.setFont (font (13.5f));
        g.drawText (b.getButtonText(), nameArea, juce::Justification::bottomLeft, true);
        g.setColour (on ? kInk.withAlpha (0.75f) : kMuted);
        g.setFont (font (11.0f, false));
        g.drawText (sub, text.withTrimmedTop (2), juce::Justification::topLeft, true);
        return;
    }

    float size = 12.5f;
    if (role == "nav") size = 17.0f;
    else if (role == "sub") size = 10.5f;
    else if (role == "arrow") size = 16.0f;

    g.setFont (font (size));
    if (! on)
    {
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawText (b.getButtonText(), r.translated (0, 1), juce::Justification::centred, true);
    }
    g.setColour (on ? kInk : kCream);
    g.drawText (b.getButtonText(), r, juce::Justification::centred, true);
}

void RiddimLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    const juce::Rectangle<float> r (0.0f, 0.0f, (float) w, (float) h);
    const auto rr = r.reduced (0.5f);
    const float alpha = box.isEnabled() ? 1.0f : 0.45f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff09100c), 0.0f, 0.0f,
                                             juce::Colour (0xff1b2c22), 0.0f, (float) h, false));
    g.fillRoundedRectangle (rr, 4.0f);
    g.setColour (kBrass.withAlpha (alpha));
    g.drawRoundedRectangle (rr, 4.0f, 1.4f);
    juce::Path arrow;
    const float cx = (float) w - 15.0f, cy = (float) h * 0.5f;
    arrow.addTriangle (cx - 5.0f, cy - 2.5f, cx + 5.0f, cy - 2.5f, cx, cy + 3.5f);
    g.setColour (kGold.withAlpha (alpha));
    g.fillPath (arrow);
}

//==============================================================================
void RiddimLiveDisplay::paint (juce::Graphics& g)
{
    drawVU (g, state.meterL.toFloat(), state.needle[0]);
    drawVU (g, state.meterR.toFloat(), state.needle[1]);

    // peak LED stack
    {
        const float peak = juce::jmax (state.peak[0], state.peak[1]);
        const float db = 20.0f * std::log10 (juce::jmax (peak, 1.0e-5f));
        const float thresholds[8] { -40.0f, -30.0f, -24.0f, -18.0f, -12.0f, -6.0f, -3.0f, 0.0f };
        const std::array<juce::Colour, 8> cols { juce::Colour (0xff3ddc57), juce::Colour (0xff3ddc57),
                                                 juce::Colour (0xff3ddc57), juce::Colour (0xff3ddc57),
                                                 juce::Colour (0xffffb020), juce::Colour (0xffffb020),
                                                 juce::Colour (0xffff3b30), juce::Colour (0xffff3b30) };
        const auto s = state.ledStack;
        const float segH = ((float) s.getHeight() - 7.0f * 3.0f) / 8.0f;
        for (int i = 0; i < 8; ++i)
        {
            const float y = (float) s.getBottom() - (float) (i + 1) * segH - (float) i * 3.0f;
            const bool lit = db >= thresholds[i];
            g.setColour (lit ? cols[(size_t) i] : cols[(size_t) i].withAlpha (0.18f));
            g.fillRoundedRectangle ((float) s.getX(), y, (float) s.getWidth(), segH, 1.5f);
            g.setColour (kInk.withAlpha (0.8f));
            g.drawRoundedRectangle ((float) s.getX(), y, (float) s.getWidth(), segH, 1.5f, 0.8f);
        }
    }

    // tape reels, tape path and head block
    {
        const auto a = state.reelArea;
        const juce::Point<float> c1 ((float) a.getX() + (float) a.getWidth() * 0.26f, (float) a.getY() + 52.0f);
        const juce::Point<float> c2 ((float) a.getX() + (float) a.getWidth() * 0.74f, (float) a.getY() + 52.0f);
        const float radius = 42.0f;
        const juce::Rectangle<float> head ((float) a.getCentreX() - 40.0f, (float) a.getBottom() - 30.0f, 80.0f, 20.0f);

        juce::Path tape;
        tape.startNewSubPath (c1.x - radius * 0.7f, c1.y + radius * 0.7f);
        tape.lineTo (head.getX() + 8.0f, head.getY());
        tape.lineTo (head.getRight() - 8.0f, head.getY());
        tape.lineTo (c2.x + radius * 0.7f, c2.y + radius * 0.7f);
        g.setColour (juce::Colour (0xff2a1a0e));
        g.strokePath (tape, juce::PathStrokeType (2.2f));

        drawReel (g, c1, radius, state.reelAngle);
        drawReel (g, c2, radius, -state.reelAngle);

        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (head.translated (0.0f, 2.0f), 4.0f);
        g.setGradientFill (juce::ColourGradient (kBrassHi, head.getX(), head.getY(), kBrassLo, head.getX(), head.getBottom(), false));
        g.fillRoundedRectangle (head, 4.0f);
        g.setColour (kInk.withAlpha (0.8f));
        for (int i = 1; i <= 3; ++i)
            g.fillRect (head.getX() + head.getWidth() * (float) i / 4.0f - 5.0f, head.getY() + 5.0f, 10.0f, 10.0f);
        g.drawRoundedRectangle (head, 4.0f, 1.2f);
    }

    // signal path lamps
    {
        const char* names[6] { "INPUT", "TAPE", "ECHO", "FILTER", "SECTION", "OUTPUT" };
        const auto a = state.pathArea;
        const float cw = (float) a.getWidth() / 3.0f;
        const float ch = (float) a.getHeight() / 2.0f;
        for (int i = 0; i < 6; ++i)
        {
            const int col = i % 3, row = i / 3;
            const juce::Point<float> c ((float) a.getX() + cw * ((float) col + 0.5f),
                                        (float) a.getY() + ch * (float) row + ch * 0.38f);
            float b = state.bypassed ? 0.0f : 1.0f;
            if (i == 5)
                b = state.bypassed ? 0.0f : juce::jlimit (0.25f, 1.0f, 0.25f + juce::jmax (state.peak[0], state.peak[1]) * 3.0f);
            drawLamp (g, c, 7.0f, b, kAmber);
            g.setColour (kMuted);
            g.setFont (RiddimLookAndFeel::font (10.5f));
            g.drawText (juce::String (names[i]), juce::Rectangle<float> (c.x - cw * 0.5f, c.y + 12.0f, cw, 14.0f),
                        juce::Justification::centred, false);
        }
    }
}

//==============================================================================
RiddimLabAudioProcessorEditor::RiddimLabAudioProcessorEditor (RiddimLabAudioProcessor& p)
    : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&look);

    headerArt = juce::ImageCache::getFromMemory (BinaryData::header_art_png, BinaryData::header_art_pngSize);

    // Fine speckle texture used to give panels an aged surface.
    grain = juce::Image (juce::Image::ARGB, 128, 128, true);
    {
        juce::Graphics ig (grain);
        juce::Random rng (42);
        for (int i = 0; i < 1500; ++i)
        {
            ig.setColour (juce::Colours::black.withAlpha (rng.nextFloat() * 0.22f));
            ig.fillRect (rng.nextInt (128), rng.nextInt (128), 1 + rng.nextInt (2), 1 + rng.nextInt (2));
        }
        for (int i = 0; i < 500; ++i)
        {
            ig.setColour (juce::Colours::white.withAlpha (rng.nextFloat() * 0.07f));
            ig.fillRect (rng.nextInt (128), rng.nextInt (128), 1, 1);
        }
    }

    addAndMakeVisible (liveDisplay);

    // Navigation (exact order requested: Master, Drums, Guitar, Bass, Keys, Horns, Vocal).
    for (int i = 0; i < 7; ++i)
    {
        navButtons[i].setButtonText (kSectionNames[i]);
        navButtons[i].setClickingTogglesState (true);
        navButtons[i].setRadioGroupId (7421);
        navButtons[i].getProperties().set ("role", "nav");
        addAndMakeVisible (navButtons[i]);
        navButtons[i].onClick = [this, i] { selectSection (i, true); };
    }
    const char* const modeNames[3] { "CHOP", "PICK", "RHYTHM" };
    for (int i = 0; i < 3; ++i)
    {
        guitarModeButtons[i].setButtonText (modeNames[i]);
        guitarModeButtons[i].setClickingTogglesState (true);
        guitarModeButtons[i].setRadioGroupId (7422);
        guitarModeButtons[i].getProperties().set ("role", "sub");
        addAndMakeVisible (guitarModeButtons[i]);
        guitarModeButtons[i].onClick = [this, i]
        {
            selectSection (2, true);
            selectGuitarMode (i, true);
        };
    }

    // Library of starting points and the header preset selector.
    for (int i = 0; i < 8; ++i)
    {
        libButtons[i].setButtonText (kPresets[i].name);
        libButtons[i].setClickingTogglesState (true);
        libButtons[i].setRadioGroupId (7423);
        libButtons[i].getProperties().set ("role", "lib");
        libButtons[i].getProperties().set ("num", juce::String (i + 1).paddedLeft ('0', 2));
        libButtons[i].getProperties().set ("sub", kPresets[i].sub);
        addAndMakeVisible (libButtons[i]);
        libButtons[i].onClick = [this, i] { applyPreset (i); };
        presetBox.addItem (juce::String (i + 1).paddedLeft ('0', 2) + "  " + kPresets[i].name, i + 1);
    }
    presetBox.setTextWhenNothingSelected ("SELECT A PRESET");
    presetBox.onChange = [this] { const int id = presetBox.getSelectedId(); if (id > 0) applyPreset (id - 1); };
    addAndMakeVisible (presetBox);

    prevPreset.setButtonText ("<");
    nextPreset.setButtonText (">");
    for (auto* b : { &prevPreset, &nextPreset })
    {
        b->getProperties().set ("role", "arrow");
        addAndMakeVisible (*b);
    }
    prevPreset.onClick = [this] { applyPreset ((juce::jmax (currentPreset, 0) + 7) % 8); };
    nextPreset.onClick = [this] { applyPreset ((currentPreset + 1) % 8); };

    presetInfo.setFont (RiddimLookAndFeel::font (12.0f, false));
    presetInfo.setColour (juce::Label::textColourId, kMuted);
    presetInfo.setJustificationType (juce::Justification::centredLeft);
    presetInfo.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (presetInfo);

    // Rotary controls, all bound to the existing parameter IDs.
    for (int k = 0; k < NumKnobs; ++k)
        initKnob (k);

    // Echo and filter selectors
    divisionBox.addItemList (juce::StringArray { "1/4", "1/8", "1/8D", "1/8T", "1/16", "1/4D" }, 1);
    addAndMakeVisible (divisionBox);
    divisionA = std::make_unique<CA> (proc.parameters, "division", divisionBox);
    divisionLabel.setText ("DIVISION", juce::dontSendNotification);

    filterModeBox.addItemList (juce::StringArray { "LOW PASS", "HIGH PASS" }, 1);
    addAndMakeVisible (filterModeBox);
    filterModeA = std::make_unique<CA> (proc.parameters, "filterMode", filterModeBox);
    filterTypeLabel.setText ("FILTER TYPE", juce::dontSendNotification);

    for (auto* l : { &divisionLabel, &filterTypeLabel })
    {
        l->setFont (RiddimLookAndFeel::font (11.0f));
        l->setColour (juce::Label::textColourId, kName);
        l->setJustificationType (juce::Justification::centredLeft);
        l->setInterceptsMouseClicks (false, false);
        addAndMakeVisible (*l);
    }

    syncToggle.setButtonText ("TEMPO SYNC");
    bypassToggle.setButtonText ("BYPASS");
    panicToggle.setButtonText ("PANIC / CLEAR");
    for (auto* t : { &syncToggle, &bypassToggle, &panicToggle })
        addAndMakeVisible (*t);
    syncA   = std::make_unique<BA> (proc.parameters, "sync", syncToggle);
    bypassA = std::make_unique<BA> (proc.parameters, "bypass", bypassToggle);
    panicA  = std::make_unique<BA> (proc.parameters, "panic", panicToggle);

    throwButton.setButtonText ("HOLD TO THROW");
    throwButton.getProperties().set ("role", "action");
    addAndMakeVisible (throwButton);
    throwButton.onStateChange = [this]
    {
        const bool down = throwButton.isDown();
        if (down != throwHeld)
        {
            throwHeld = down;
            setBoolParam ("throw", down);
        }
    };

    // Restore which tab / guitar mode the saved state is in.
    if (auto* sec = proc.parameters.getRawParameterValue ("activeSection"))
        activeSection = juce::jlimit (0, 6, juce::roundToInt (sec->load()));
    if (auto* gm = proc.parameters.getRawParameterValue ("guitarMode"))
        activeGuitarMode = juce::jlimit (0, 2, juce::roundToInt (gm->load()));
    guitarModeButtons[activeGuitarMode].setToggleState (true, juce::dontSendNotification);
    selectSection (activeSection, false);

    setResizable (false, false);
    setSize (1280, 900);
    startTimerHz (30);
}

RiddimLabAudioProcessorEditor::~RiddimLabAudioProcessorEditor()
{
    stopTimer();
    if (throwHeld)
        setBoolParam ("throw", false);
    setLookAndFeel (nullptr);
}

//==============================================================================
void RiddimLabAudioProcessorEditor::initKnob (int k)
{
    auto& s = knobs[k];
    const auto& def = kKnobDefs[k];
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 74, 18);
    addAndMakeVisible (s);

    auto& l = knobLabels[k];
    l.setText (def.name, juce::dontSendNotification);
    l.setFont (RiddimLookAndFeel::font (12.0f));
    l.setColour (juce::Label::textColourId, kName);
    l.setJustificationType (juce::Justification::centred);
    l.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (l);

    if (def.paramId != nullptr)
    {
        // The attachment installs its own text functions, so the display
        // format is applied afterwards.
        knobA[k] = std::make_unique<SA> (proc.parameters, def.paramId, s);
        setSliderFormat (s, def.fmt);
    }
}

void RiddimLabAudioProcessorEditor::setKnobVisible (int k, bool visible)
{
    knobs[k].setVisible (visible);
    knobLabels[k].setVisible (visible);
}

void RiddimLabAudioProcessorEditor::placeKnob (int k, juce::Rectangle<int> cell)
{
    auto c = cell.reduced (3, 0);
    auto lab = c.removeFromBottom (18);
    knobs[k].setBounds (c);
    knobLabels[k].setBounds (lab);
}

//==============================================================================
void RiddimLabAudioProcessorEditor::setFloatParam (const char* id, float plainValue)
{
    if (auto* prm = proc.parameters.getParameter (id))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 (plainValue));
        prm->endChangeGesture();
    }
}

void RiddimLabAudioProcessorEditor::setBoolParam (const char* id, bool value)
{
    if (auto* prm = proc.parameters.getParameter (id))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (value ? 1.0f : 0.0f);
        prm->endChangeGesture();
    }
}

void RiddimLabAudioProcessorEditor::setIntParam (const char* id, int value)
{
    setFloatParam (id, (float) value);
}

//==============================================================================
void RiddimLabAudioProcessorEditor::selectSection (int section, bool notifyHost)
{
    activeSection = juce::jlimit (0, 6, section);
    navButtons[activeSection].setToggleState (true, juce::dontSendNotification);
    if (notifyHost)
        setIntParam ("activeSection", activeSection);
    rebuildChain();
}

void RiddimLabAudioProcessorEditor::selectGuitarMode (int mode, bool notifyHost)
{
    activeGuitarMode = juce::jlimit (0, 2, mode);
    guitarModeButtons[activeGuitarMode].setToggleState (true, juce::dontSendNotification);
    if (notifyHost)
        setIntParam ("guitarMode", activeGuitarMode);
    layoutChain();
    repaint (rChain);
}

// Re-binds the three section knobs (Drive / Tone / Comp) to the parameters of
// the active section. Only the attachments change; parameter IDs are untouched.
void RiddimLabAudioProcessorEditor::rebuildChain()
{
    const juce::String sec (kSectionIds[activeSection]);
    const char* const suffix[3] { "Drive", "Tone", "Comp" };
    for (int i = 0; i < 3; ++i)
    {
        const int k = ChainDrive + i;
        knobA[k].reset();
        knobA[k] = std::make_unique<SA> (proc.parameters, sec + suffix[i], knobs[k]);
        setSliderFormat (knobs[k], 0);
    }
    knobLabels[ChainTone].setText (activeSection == 3 ? "TONE (LOW-PASS)" : "TONE", juce::dontSendNotification);
    layoutChain();
    repaint (rChain);
}

void RiddimLabAudioProcessorEditor::layoutChain()
{
    const bool guitar = activeSection == 2;
    setKnobVisible (GuitarChop,  guitar && activeGuitarMode == 0);
    setKnobVisible (GuitarPick,  guitar && activeGuitarMode == 1);
    setKnobVisible (GuitarBody,  guitar && activeGuitarMode == 2);

    if (rChain.isEmpty())
        return;

    auto inner = rChain.reduced (14, 0).withTrimmedTop (58).withTrimmedBottom (12);
    const int cells = guitar ? 4 : 3;
    const int cw = inner.getWidth() / cells;
    for (int i = 0; i < 3; ++i)
        placeKnob (ChainDrive + i, inner.removeFromLeft (cw));
    if (guitar)
        placeKnob (GuitarChop + activeGuitarMode, inner.removeFromLeft (cw));
}

void RiddimLabAudioProcessorEditor::applyPreset (int index)
{
    index = juce::jlimit (0, 7, index);
    currentPreset = index;
    for (int i = 0; i < 13; ++i)
        if (kPresets[index].v[i] >= 0.0f)
            setFloatParam (kPresetIds[i], kPresets[index].v[i]);
    updatePresetDisplay();
}

void RiddimLabAudioProcessorEditor::updatePresetDisplay()
{
    if (currentPreset >= 0)
    {
        libButtons[currentPreset].setToggleState (true, juce::dontSendNotification);
        presetBox.setSelectedId (currentPreset + 1, juce::dontSendNotification);
        presetInfo.setText (juce::String (currentPreset + 1).paddedLeft ('0', 2) + " / 08   " + kPresets[currentPreset].sub,
                            juce::dontSendNotification);
    }
}

//==============================================================================
void RiddimLabAudioProcessorEditor::timerCallback()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        const float peak = proc.getOutputPeak (ch);
        const float db = 20.0f * std::log10 (juce::jmax (peak, 1.0e-5f));
        const float target = juce::jlimit (0.0f, 1.0f, (db + 40.0f) / 43.0f);
        const float coeff = target > live.needle[ch] ? 0.55f : 0.12f;
        live.needle[ch] += (target - live.needle[ch]) * coeff;
        live.peak[ch] = peak;
    }

    if (auto* byp = proc.parameters.getRawParameterValue ("bypass"))
        live.bypassed = byp->load() > 0.5f;
    live.playing = ! live.bypassed && (live.peak[0] + live.peak[1]) > 0.002f;
    if (live.playing)
    {
        live.reelAngle += 0.06f;
        if (live.reelAngle > juce::MathConstants<float>::twoPi)
            live.reelAngle -= juce::MathConstants<float>::twoPi;
    }

    // Follow host automation / state restore for the tab and guitar mode.
    if (auto* sec = proc.parameters.getRawParameterValue ("activeSection"))
    {
        const int s = juce::jlimit (0, 6, juce::roundToInt (sec->load()));
        if (s != activeSection)
            selectSection (s, false);
    }
    if (auto* gm = proc.parameters.getRawParameterValue ("guitarMode"))
    {
        const int m = juce::jlimit (0, 2, juce::roundToInt (gm->load()));
        if (m != activeGuitarMode)
            selectGuitarMode (m, false);
    }

    // Free time only matters with tempo sync off; division only with it on.
    if (auto* sy = proc.parameters.getRawParameterValue ("sync"))
    {
        const bool synced = sy->load() > 0.5f;
        knobs[Time].setEnabled (! synced);
        divisionBox.setEnabled (synced);
    }

    liveDisplay.repaint (live.meterL.expanded (8));
    liveDisplay.repaint (live.meterR.expanded (8));
    liveDisplay.repaint (live.ledStack.expanded (2));
    liveDisplay.repaint (live.reelArea);
    liveDisplay.repaint (live.pathArea.expanded (4));
}

//==============================================================================
void RiddimLabAudioProcessorEditor::paint (juce::Graphics& g)
{
    drawWoodFrame (g, getLocalBounds());
    const auto body = getLocalBounds().reduced (10).toFloat();
    g.setColour (juce::Colour (0xff070b08));
    g.fillRoundedRectangle (body, 6.0f);
    g.setColour (kBrassLo);
    g.drawRoundedRectangle (body, 6.0f, 2.0f);

    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    if (headerArt.isValid())
    {
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (rHeaderArt.toFloat().translated (0.0f, 3.0f), 7.0f);
        g.drawImage (headerArt, rHeaderArt.getX(), rHeaderArt.getY(), rHeaderArt.getWidth(), rHeaderArt.getHeight(),
                     0, 0, headerArt.getWidth(), headerArt.getHeight());
    }
    else
    {
        drawPanel (g, rHeaderArt, "RIDDIM LAB", Green, grain);
    }

    drawPanel (g, rMeters,  {}, Dark, grain);
    drawPanel (g, rPreset,  "PRESET", Dark, grain);
    drawPanel (g, rLibrary, "LIBRARY", Dark, grain);
    drawPanel (g, rTape,    "TAPE MACHINE  /  SATURATION", Brown, grain);
    drawPanel (g, rFilter,  "TUBBY FILTER", Green, grain);
    drawPanel (g, rEcho,    "DUB ECHO", Green, grain);
    drawPanel (g, rChain,   juce::String (kSectionNames[activeSection]) + " SECTION", Green, grain);
    drawPanel (g, rPath,    "SIGNAL PATH", Green, grain);
    drawPanel (g, rSafety,  "MASTER SAFETY", Brown, grain);

    // meter captions
    g.setColour (kName);
    g.setFont (RiddimLookAndFeel::font (12.0f));
    g.drawText ("LEFT",  live.meterL.getX(), live.meterL.getBottom() + 8, live.meterL.getWidth(), 14, juce::Justification::centred);
    g.drawText ("RIGHT", live.meterR.getX(), live.meterR.getBottom() + 8, live.meterR.getWidth(), 14, juce::Justification::centred);

    // section note
    g.setColour (kMuted);
    g.setFont (RiddimLookAndFeel::font (11.5f, false));
    g.drawText ("TONE: " + juce::String (kToneRole[activeSection]) + "  |  DRIVE: SATURATION  |  COMP: PEAKS",
                rChain.getX() + 18, rChain.getY() + 38, rChain.getWidth() - 36, 16,
                juce::Justification::centredLeft, true);

    // safety note
    g.drawText ("PANIC CLEARS ECHO TAILS.", rSafety.getX() + 18, rSafety.getBottom() - 70, rSafety.getWidth() - 36, 16,
                juce::Justification::centredLeft, true);
    g.drawText ("KEEP MONITORING LOW", rSafety.getX() + 18, rSafety.getBottom() - 52, rSafety.getWidth() - 36, 16,
                juce::Justification::centredLeft, true);
    g.drawText ("WHEN PUSHING FEEDBACK.", rSafety.getX() + 18, rSafety.getBottom() - 36, rSafety.getWidth() - 36, 16,
                juce::Justification::centredLeft, true);
}

//==============================================================================
void RiddimLabAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (14);       // 1252 x 872 at the standard 1280 x 900

    auto header = area.removeFromTop (130);
    area.removeFromTop (8);
    rHeaderArt = header.removeFromLeft (556);
    header.removeFromLeft (8);
    rMeters = header.removeFromLeft (392);
    header.removeFromLeft (8);
    rPreset = header;

    rLibrary = area.removeFromLeft (206);
    area.removeFromLeft (8);

    auto nav = area.removeFromTop (54);
    area.removeFromTop (8);
    auto rowA = area.removeFromTop (196);
    area.removeFromTop (8);
    auto rowB = area.removeFromTop (224);
    area.removeFromTop (8);
    auto rowC = area;

    rTape   = rowA.removeFromLeft (640);
    rowA.removeFromLeft (8);
    rFilter = rowA;
    rEcho   = rowB;
    rChain  = rowC.removeFromLeft (560);
    rowC.removeFromLeft (8);
    rPath   = rowC.removeFromLeft (250);
    rowC.removeFromLeft (8);
    rSafety = rowC;

    liveDisplay.setBounds (getLocalBounds());

    // ---- navigation tabs; the Guitar tab carries its Chop / Pick / Rhythm subtabs
    {
        const float weights[7] { 1.0f, 1.0f, 1.3f, 1.0f, 1.0f, 1.0f, 1.0f };
        float total = 0.0f;
        for (float w : weights) total += w;
        const int gap = 4;
        const float unit = (float) (nav.getWidth() - 6 * gap) / total;
        int x = nav.getX();
        for (int i = 0; i < 7; ++i)
        {
            const int w = (i == 6) ? nav.getRight() - x : juce::roundToInt (unit * weights[i]);
            const juce::Rectangle<int> cell (x, nav.getY(), w, nav.getHeight());
            if (i == 2)
            {
                navButtons[i].setBounds (cell.withHeight (30));
                auto subs = cell.withTrimmedTop (32);
                const int sw = subs.getWidth() / 3;
                for (int s = 0; s < 3; ++s)
                    guitarModeButtons[s].setBounds ((s == 2 ? subs : subs.removeFromLeft (sw)).reduced (1, 0));
            }
            else
            {
                navButtons[i].setBounds (cell);
            }
            x += w + gap;
        }
    }

    // ---- library list
    {
        auto inner = rLibrary.reduced (10, 0).withTrimmedTop (40).withTrimmedBottom (12);
        const int itemH = (inner.getHeight() - 7 * 4) / 8;
        for (int i = 0; i < 8; ++i)
        {
            libButtons[i].setBounds (inner.removeFromTop (itemH));
            inner.removeFromTop (4);
        }
    }

    // ---- header preset block
    {
        auto pin = rPreset.reduced (14, 0).withTrimmedTop (38).withTrimmedBottom (12);
        presetBox.setBounds (pin.removeFromTop (34));
        pin.removeFromTop (8);
        auto row = pin.removeFromTop (32);
        prevPreset.setBounds (row.removeFromLeft (44));
        row.removeFromLeft (6);
        nextPreset.setBounds (row.removeFromLeft (44));
        row.removeFromLeft (10);
        presetInfo.setBounds (row);
    }

    // ---- header meters (the moving parts are drawn by the live overlay)
    {
        auto inner = rMeters.reduced (14, 10);
        live.meterL = juce::Rectangle<int> (inner.getX() + 4, inner.getY() + 4, 160, 86);
        live.meterR = juce::Rectangle<int> (inner.getRight() - 164, inner.getY() + 4, 160, 86);
        live.ledStack = juce::Rectangle<int> (rMeters.getCentreX() - 8, inner.getY() + 8, 16, 82);
    }

    // ---- tape machine: reels on the left, five knobs on the right
    {
        auto inner = rTape.reduced (14, 0).withTrimmedTop (38).withTrimmedBottom (10);
        live.reelArea = inner.removeFromLeft (210);
        const int cw = inner.getWidth() / 5;
        const int order[5] { TapeSat, TapeTone, Wow, Flutter, Wear };
        for (int i = 0; i < 5; ++i)
            placeKnob (order[i], inner.removeFromLeft (cw));
    }

    // ---- tubby filter: two knobs and the mode selector
    {
        auto inner = rFilter.reduced (14, 0).withTrimmedTop (38).withTrimmedBottom (10);
        auto modeCol = inner.removeFromRight (140);
        const int cw = inner.getWidth() / 2;
        placeKnob (Cutoff, inner.removeFromLeft (cw));
        placeKnob (Resonance, inner.removeFromLeft (cw));
        auto m = modeCol.withSizeKeepingCentre (modeCol.getWidth() - 6, 50);
        filterTypeLabel.setBounds (m.removeFromTop (18));
        filterModeBox.setBounds (m.removeFromTop (30));
    }

    // ---- dub echo: six knobs and the timing column
    {
        auto inner = rEcho.reduced (14, 0).withTrimmedTop (38).withTrimmedBottom (10);
        auto col = inner.removeFromRight (240).withSizeKeepingCentre (230, 140);
        const int cw = inner.getWidth() / 6;
        const int order[6] { Time, Feedback, Mix, Heat, Offset, Damping };
        for (int i = 0; i < 6; ++i)
            placeKnob (order[i], inner.removeFromLeft (cw));
        divisionLabel.setBounds (col.removeFromTop (16));
        divisionBox.setBounds (col.removeFromTop (30));
        col.removeFromTop (10);
        syncToggle.setBounds (col.removeFromTop (32));
        col.removeFromTop (10);
        throwButton.setBounds (col.removeFromTop (40));
    }

    layoutChain();

    // ---- signal path lamps
    live.pathArea = rPath.reduced (16, 0).withTrimmedTop (46).withTrimmedBottom (16);

    // ---- safety switches
    {
        auto inner = rSafety.reduced (14, 0).withTrimmedTop (44);
        bypassToggle.setBounds (inner.removeFromTop (38));
        inner.removeFromTop (10);
        panicToggle.setBounds (inner.removeFromTop (38));
    }
}
