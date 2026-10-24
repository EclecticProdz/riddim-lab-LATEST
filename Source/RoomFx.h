#pragma once
#include <JuceHeader.h>
#include <array>

// Live lighting, musicians and smoke laid over the room picture. Everything here is drawn on top of the
// still photograph:
//  - the lamps and bright spots already in the picture are found automatically and glow and flicker,
//    pumping gently with the output level;
//  - each room has its musician, who sways and bobs with the output level;
//  - on the Home page, click the man sitting on the right: he lights a lighter and the smoke drifts up.
class RoomFx : public juce::Component
{
public:
    RoomFx() { setInterceptsMouseClicks (true, false); setOpaque (false); }

    // figure pictures in this order: drummer, bassist, guitarist, pianist, sax, engineer, seated, seated with lighter lit
    void setFigures (const juce::Image* imgs) { for (int i = 0; i < 8; ++i) fig[i] = imgs[i]; }

    // picture shown in this room (stretched over the component) and the page number (0 home, 1 master, 2 drums,
    // 3 guitar, 4 bass, 5 keys, 6 horns, 7 control); lights are re-detected and the room's musician is placed
    void setRoom (const juce::Image& img, int pageId)
    {
        lights.clear(); figs.clear(); puffs.clear();
        page = pageId; lighter = 0.0f; lighterWasOn = false;
        reels.clear(); cones.clear(); lamps.clear(); spin = 0.0f; fast = 0.0f;
        findLights (img);
        buildFigures();
        buildControlFx (img);
        place();
        repaint();
    }

    void resized() override { place(); }

    // false = animation switched off: the picture stays still and the lighter cannot be lit
    void setActive (bool on) { if (on != active) { active = on; if (! on) { lighter = 0.0f; puffs.clear(); } repaint(); } }
    bool isActive() const noexcept { return active; }

    bool hitTest (int x, int y) override
    {
        if (! active) return false;
        for (auto& f : figs)
            if (f.click && f.box.reduced (f.box.getWidth() / 5, f.box.getHeight() / 8).contains (x, y)) return true;
        return false;
    }
    void mouseUp (const juce::MouseEvent&) override { toggleLighter(); }

    bool isLit() const noexcept { return lighter > 0.0f; }
    void toggleLighter()
    {
        if (page != 0 || ! active) return;
        if (lighter > 0.0f) { lighter = 0.0f; repaint(); return; }
        lighter = 14.0f; spawnClock = 0.0f;
    }

    // Called by the owner on every display frame. `dt` is the real time since the last call (seconds) and
    // level is the recent output peak (0..1). `draw` says whether this frame should also redraw the picture:
    // the owner passes false on some frames while a knob is being dragged or the setting is "Reduced", so the
    // movement keeps its timing but the screen is touched less often.
    void tick (float level, float dt, bool draw)
    {
        dt = juce::jlimit (0.001f, 0.1f, dt);
        time += dt;
        const float k = 1.0f - std::pow (level > beat ? 0.5f : 0.88f, dt * 30.0f);      // same feel at any frame rate
        beat += (level - beat) * k;
        if (lighter > 0.0f)
        {
            lighter -= dt; spawnClock -= dt;
            if (spawnClock <= 0.0f && lighter > 0.0f) { spawnClock = 0.07f; spawn(); }
        }
        // smoke always moves; it is only redrawn when asked to
        juce::Rectangle<int> dirty;
        for (auto it = puffs.begin(); it != puffs.end();)
        {
            dirty = dirty.getUnion (puffBox (*it));
            it->age += dt;
            if (it->age >= it->life) { it = puffs.erase (it); continue; }
            const float wob = 1.0f + 2.0f * beat;
            it->x += (it->vx + std::sin (it->age * 1.3f + it->seed) * 9.0f * wob) * dt;
            it->y -= (it->vy + it->age * 5.0f) * dt;
            dirty = dirty.getUnion (puffBox (*it));
            ++it;
        }
        pendingDirty = pendingDirty.getUnion (dirty);
        if (! draw) { lighterWasOn = lighterWasOn || lighter > 0.0f; return; }

        if (page == 7)
        {
            const float target = beat > 0.02f ? 4.5f : 0.0f;
            spin += (target - spin) * (1.0f - std::exp (-dt * 3.0f));
            for (auto& r : reels) r.ang += spin * dt;
            fast = juce::jmax (level, fast * std::exp (-dt * 12.0f));
        }

        ++frame;
        if (page == 7)
        {
            const bool spinning = spin > 0.02f, loud = fast > 0.01f;
            if (spinning || spinWas) for (auto& r : reels) repaint (r.box);
            if (loud || loudWas) for (auto& c : cones) repaint (c.box);
            if ((loud || loudWas) && (frame & 1) == 0) for (auto& l : lamps) repaint (l.box);
            spinWas = spinning; loudWas = loud;
        }
        if ((frame & 1) == 0) for (auto& l : lights) repaint (l.box);                   // lamps flicker slowly: half rate is enough
        // the musicians move every frame while there is signal, at half rate when it is quiet
        if (beat > 0.01f || (frame & 1) == 0) for (auto& f : figs) repaint (f.box);
        if (! pendingDirty.isEmpty()) { repaint (pendingDirty.expanded (4)); pendingDirty = {}; }
        if (lighter > 0.0f || lighterWasOn) repaint (flameBox());
        lighterWasOn = lighter > 0.0f;
    }

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        const float pump = 1.0f + 1.4f * beat;
        for (auto& l : lights)
        {
            const float f = 0.5f + 0.5f * (0.6f * std::sin (time * 3.1f + l.ph) + 0.4f * std::sin (time * 7.7f + l.ph * 2.3f));
            const float a = jlimit (0.0f, 0.5f, l.lum * 0.30f * (0.55f + 0.45f * f) * pump);
            g.setGradientFill (ColourGradient (Colour (0xffffd28a).withAlpha (a), l.x, l.y, Colour (0xffffb050).withAlpha (0.0f), l.x + l.r, l.y, true));
            g.fillEllipse (l.x - l.r, l.y - l.r, l.r * 2, l.r * 2);
        }
        for (auto& f : figs) paintFigure (g, f);
        if (page == 7) paintControlFx (g);
        for (auto& p : puffs)
        {
            const float t = p.age / p.life;
            const float fade = std::pow (1.0f - t, 1.6f) * jmin (1.0f, p.age * 2.5f);
            const float r = p.size * (0.35f + 0.65f * (0.25f + t));
            g.setGradientFill (ColourGradient (Colour (0xffe9e2d6).withAlpha (0.34f * fade), p.x, p.y, Colour (0xffe9e2d6).withAlpha (0.0f), p.x + r, p.y, true));
            g.fillEllipse (p.x - r, p.y - r, r * 2, r * 2);
        }
        if (lighter > 0.0f && page == 0)
        {
            const auto o = flameOrigin();
            const float fl = 0.85f + 0.15f * std::sin (time * 23.0f) + 0.08f * std::sin (time * 37.0f);
            const float on = jmin (1.0f, (14.0f - lighter) * 6.0f) * jmin (1.0f, lighter * 3.0f);
            g.setGradientFill (ColourGradient (Colour (0xffffc060).withAlpha (0.45f * on * fl), o.x, o.y, Colour (0xffff9020).withAlpha (0.0f), o.x + 60, o.y, true));
            g.fillEllipse (o.x - 60, o.y - 60, 120, 120);
        }
    }

private:
    struct Light { float gx, gy, x, y, r, lum, ph; juce::Rectangle<int> box; };
    struct Puff { float x, y, vx, vy, age, life, size, seed; };
    struct Fig { int img; float cx, footY, h, sway, bob, phase, speed; bool click; juce::Rectangle<int> box; bool floor = true; };
    std::vector<Light> lights; std::vector<Puff> puffs; std::vector<Fig> figs;
    juce::Image fig[8];
    int gridW = 96, gridH = 28, frame = 0, page = -1;
    float time = 0.0f, beat = 0.0f, lighter = 0.0f, spawnClock = 0.0f; bool lighterWasOn = false;
    bool active = true;
    // control room extras: reel-to-reel spinning, speaker cones pumping, console meter lamps pulsing
    struct Reel { float cx, cy, rx, ry, ang; juce::Image img; juce::Point<float> origin; juce::Rectangle<int> box; };
    struct Cone { float cx, cy, r; juce::Image img; juce::Rectangle<int> box; };
    struct Lamp { juce::Rectangle<float> r; float ph; juce::Rectangle<int> box; };
    std::vector<Reel> reels; std::vector<Cone> cones; std::vector<Lamp> lamps;
    float spin = 0.0f, fast = 0.0f; bool spinWas = false, loudWas = false;
    juce::Rectangle<int> pendingDirty;
    juce::Image scaledFig[8]; int scaledFigH[8] {};
    juce::Random rng;

    void findLights (const juce::Image& img)
    {
        if (! img.isValid()) return;
        const int gw = 96, gh = juce::jmax (8, (int) std::lround (gw * (double) img.getHeight() / (double) img.getWidth()));
        auto small = img.rescaled (gw, gh, juce::Graphics::mediumResamplingQuality);
        struct C { float l; int x, y; };
        std::vector<C> cand;
        for (int y = 1; y < gh - 1; ++y)
            for (int x = 1; x < gw - 1; ++x)
            {
                const auto c = small.getPixelAt (x, y);
                const float lum = 0.30f * c.getFloatRed() + 0.59f * c.getFloatGreen() + 0.11f * c.getFloatBlue();
                if (lum > 0.66f && c.getFloatRed() >= c.getFloatBlue() * 0.95f) cand.push_back ({ lum, x, y });
            }
        std::sort (cand.begin(), cand.end(), [] (const C& a, const C& b) { return a.l > b.l; });
        for (auto& c : cand)
        {
            bool ok = true;
            for (auto& l : lights) { const float dx = (c.x - l.gx), dy = (c.y - l.gy); if (dx * dx + dy * dy < 150.0f) { ok = false; break; } }
            if (! ok) continue;
            lights.push_back ({ (float) c.x, (float) c.y, 0.0f, 0.0f, 0.0f, c.l, (float) lights.size() * 1.7f, {} });
            if (lights.size() >= 8) break;
        }
        gridW = gw; gridH = gh;
    }

    // where each room's musician stands (room coordinates, 1252 x 358): picture, centre x, foot line, height, sway deg, bob px, phase, speed
    void buildFigures()
    {
        switch (page)
        {
            case 0: figs.push_back ({ 6, 1130.0f, 352.0f, 235.0f, 0.5f, 1.5f, 0.0f, 0.5f, true, {} }); break;   // sitting, click for the lighter
            case 1: figs.push_back ({ 5, 650.5f, 435.0f, 300.6f, 0.2f, 0.0f, 0.0f, 0.6f, false, {}, false }); break;   // engineer: scaled to sit exactly over the chair in the picture
            case 2: figs.push_back ({ 0, 636.0f, 300.0f, 215.0f, 0.9f, 5.0f, 0.0f, 1.0f, false, {} }); break;   // drummer
            case 3: figs.push_back ({ 2, 650.0f, 348.0f, 290.0f, 1.4f, 3.0f, 1.2f, 0.8f, false, {} }); break;   // guitarist
            case 4: figs.push_back ({ 1, 335.0f, 348.0f, 290.0f, 1.4f, 3.5f, 0.5f, 0.7f, false, {} }); break;   // bassist
            case 5: figs.push_back ({ 3, 628.0f, 305.0f, 205.0f, 0.7f, 2.5f, 0.3f, 0.7f, false, {} }); break;   // pianist
            case 6: figs.push_back ({ 4, 740.0f, 348.0f, 300.0f, 1.8f, 3.0f, 0.9f, 0.6f, false, {} }); break;   // sax
            case 7: break;   // Control room: no person, the equipment moves instead (see buildControlFx)
            default: break;
        }
    }

    // Positions are in room coordinates (1252 x 358) and were measured on the Control room picture.
    void buildControlFx (const juce::Image& img)
    {
        if (page != 7 || ! img.isValid()) return;
        const auto box = [] (float cx, float cy, float r) { return juce::Rectangle<float> (cx - r - 3, cy - r - 3, 2 * r + 6, 2 * r + 6).toNearestInt(); };
        const float k = (float) img.getWidth() / 1252.0f;
        // the two reels of the tape machine are seen at an angle, so they are ellipses (half width rx, half height ry)
        for (auto rl : { std::array<float, 4> { 33.5f, 158.0f, 35.0f, 50.0f }, std::array<float, 4> { 113.0f, 155.0f, 29.0f, 44.0f } })
        {
            const float ox = rl[0] - rl[2] - 3.0f, oy = rl[1] - rl[3] - 3.0f;
            reels.push_back ({ rl[0], rl[1], rl[2], rl[3], reels.empty() ? 0.0f : 0.6f,
                               img.getClippedImage (juce::Rectangle<float> (ox * k, oy * k, (rl[2] + 3.0f) * 2 * k, (rl[3] + 3.0f) * 2 * k).toNearestInt()), { ox, oy },
                               juce::Rectangle<float> (rl[0] - rl[2] - 3, rl[1] - rl[3] - 3, 2 * rl[2] + 6, 2 * rl[3] + 6).toNearestInt() });
        }
        for (auto c : { juce::Point<float> (263.3f, 212.7f), juce::Point<float> (978.0f, 216.0f) })
        {
            const float r = 36.0f, e = r + 6.0f;
            cones.push_back ({ c.x, c.y, r, img.getClippedImage (juce::Rectangle<float> ((c.x - e) * k, (c.y - e) * k, 2 * e * k, 2 * e * k).toNearestInt()), box (c.x, c.y, r + 2.0f) });
        }
        // the console's meter windows (x, y, w, h)
        static const float m[][4] { { 242, 325.5f, 14.5f, 8.5f }, { 290, 323, 17.5f, 11.5f }, { 346, 316.5f, 20, 18.5f }, { 397.5f, 320, 19.5f, 15 }, { 447.5f, 316.5f, 22.5f, 19 },
                                    { 498, 317.5f, 23.5f, 18.5f }, { 551, 317.5f, 23, 18.5f }, { 601, 316, 28, 20 }, { 695.5f, 317.5f, 28, 18.5f }, { 751, 317, 25, 19 },
                                    { 806, 317.5f, 23.5f, 18.5f }, { 859.5f, 317, 19.5f, 19 }, { 911.5f, 316.5f, 21, 19 }, { 965, 320, 19.5f, 15.5f }, { 1018, 317, 16.5f, 18 }, { 1071, 322.5f, 14.5f, 12 } };
        int i = 0;
        for (auto& q : m)
        {
            juce::Rectangle<float> rc (q[0], q[1], q[2], q[3]);
            lamps.push_back ({ rc, (float) i * 0.83f, rc.expanded (3).toNearestInt() }); ++i;
        }
    }

    void paintControlFx (juce::Graphics& g)
    {
        using namespace juce;
        // reels: the reel part of the picture itself is turned about its hub, so it looks exactly like the picture
        if (spin > 0.02f)
        {
            for (auto& r : reels)
            {
                Graphics::ScopedSaveState ss (g);
                Path clip; clip.addEllipse (r.cx - r.rx * 0.97f, r.cy - r.ry * 0.97f, r.rx * 1.94f, r.ry * 1.94f);
                g.reduceClipRegion (clip);
                const float k = (float) r.img.getWidth() / ((r.rx + 3.0f) * 2.0f);          // picture pixels per room unit
                const auto tf = AffineTransform::scale (1.0f / k).translated (r.origin.x - r.cx, r.origin.y - r.cy)
                                    .scaled (r.ry / r.rx, 1.0f).rotated (r.ang).scaled (r.rx / r.ry, 1.0f).translated (r.cx, r.cy);
                g.setOpacity (1.0f);
                g.setImageResamplingQuality (Graphics::mediumResamplingQuality);
                g.drawImageTransformed (r.img, tf, false);
            }
        }
        const float pump = jlimit (0.0f, 1.0f, std::pow (fast, 0.6f) * 1.15f);
        if (pump > 0.005f)
        {
            // speaker cones: the cone area of the picture grows a little with the signal
            for (auto& c : cones)
            {
                Graphics::ScopedSaveState ss (g);
                Path clip; clip.addEllipse (c.cx - c.r, c.cy - c.r, c.r * 2, c.r * 2);
                g.reduceClipRegion (clip);
                const float e = (c.r + 6.0f) * (1.0f + 0.09f * pump);
                g.setOpacity (1.0f);
                g.setImageResamplingQuality (Graphics::mediumResamplingQuality);
                g.drawImage (c.img, Rectangle<float> (c.cx - e, c.cy - e, e * 2, e * 2));
                g.setColour (Colour (0xfffff0d0).withAlpha (0.10f * pump));
                g.fillEllipse (c.cx - c.r, c.cy - c.r, c.r * 2, c.r * 2);
            }
            // console meters brighten and bounce
            for (auto& l : lamps)
            {
                const float w = 0.55f + 0.45f * std::sin (time * 6.5f + l.ph) * std::sin (time * 2.3f + l.ph * 1.7f);
                const float a = jlimit (0.0f, 0.7f, 0.62f * pump * w);
                g.setColour (Colour (0xffffb84a).withAlpha (a * 0.22f));
                g.fillRoundedRectangle (l.r.expanded (2.5f), 4.0f);                    // soft glow around the window
                g.setColour (Colour (0xffffd070).withAlpha (a));
                g.fillRoundedRectangle (l.r.reduced (1.2f), 2.0f);
            }
        }
    }

    void paintFigure (juce::Graphics& g, const Fig& f)
    {
        using namespace juce;
        int idx = f.img;
        if (idx == 6 && lighter > 0.0f) idx = 7;
        const auto& src = fig[idx];
        if (! src.isValid()) return;
        // the picture is resized once to the exact pixel height it is drawn at (screen scale included)
        const float ps = jmax (0.25f, g.getInternalContext().getPhysicalPixelScaleFactor());
        const int wantH = jmax (8, roundToInt (f.h * ps));
        if (scaledFigH[idx] != wantH || ! scaledFig[idx].isValid())
        {
            scaledFig[idx] = src.rescaled (jmax (4, roundToInt (src.getWidth() * (double) wantH / src.getHeight())), wantH, Graphics::highResamplingQuality);
            scaledFigH[idx] = wantH;
        }
        const auto& im = scaledFig[idx];
        const float s = f.h / (float) im.getHeight();
        const float ang = f.sway * 0.0174533f * std::sin (time * 6.2832f * 0.55f * f.speed + f.phase) + 0.012f * beat * std::sin (time * 9.0f + f.phase);
        const float bob = f.bob * beat * (0.6f + 0.4f * std::sin (time * 11.0f * f.speed));
        const float sy = 1.0f + 0.010f * beat + 0.004f * std::sin (time * 1.9f + f.phase);
        // soft shadow on the floor
        if (f.floor)
        {
            g.setGradientFill (ColourGradient (Colours::black.withAlpha (0.38f), f.cx, f.footY, Colours::black.withAlpha (0.0f), f.cx + f.h * 0.34f, f.footY, true));
            g.fillEllipse (f.cx - f.h * 0.34f, f.footY - f.h * 0.05f, f.h * 0.68f, f.h * 0.10f);
        }
        const float ax = (float) im.getWidth() * 0.5f, ay = (float) im.getHeight();
        auto tf = AffineTransform::translation (-ax, -ay).scaled (1.0f, sy).rotated (ang).scaled (s, s).translated (f.cx, f.footY - bob);
        g.setOpacity (1.0f);
        g.setImageResamplingQuality (Graphics::mediumResamplingQuality);
        g.drawImageTransformed (im, tf, false);
    }

    juce::Point<float> flameOrigin() const
    {
        for (auto& f : figs) if (f.img == 6)
        {
            const float s = f.h / 432.0f;
            return { f.cx + (141.0f - 161.0f) * s, f.footY - f.h + 44.0f * s };
        }
        return { 0, 0 };
    }
    juce::Rectangle<int> flameBox() const { auto o = flameOrigin(); return juce::Rectangle<float> (o.x - 70, o.y - 70, 140, 140).toNearestInt(); }
    juce::Rectangle<int> puffBox (const Puff& p) const { const float r = p.size * 1.1f; return juce::Rectangle<float> (p.x - r, p.y - r, r * 2, r * 2).toNearestInt(); }
    void spawn()
    {
        const auto o = flameOrigin();
        puffs.push_back ({ o.x + rng.nextFloat() * 4.0f - 2.0f, o.y - 6.0f, 4.0f + rng.nextFloat() * 8.0f, 20.0f + rng.nextFloat() * 14.0f, 0.0f,
                           7.0f + rng.nextFloat() * 4.0f, 34.0f + rng.nextFloat() * 36.0f, rng.nextFloat() * 6.28f });
        if (puffs.size() > 160) puffs.erase (puffs.begin());
    }
    void place()
    {
        const float sx = (float) getWidth() / (float) gridW, sy = (float) getHeight() / (float) gridH;
        for (auto& l : lights)
        {
            l.x = (l.gx + 0.5f) * sx; l.y = (l.gy + 0.5f) * sy; l.r = 58.0f + 40.0f * l.lum;
            l.box = juce::Rectangle<float> (l.x - l.r, l.y - l.r, l.r * 2, l.r * 2).toNearestInt();
        }
        for (auto& f : figs)
        {
            const float w = f.h * 1.1f;
            f.box = juce::Rectangle<float> (f.cx - w * 0.75f, f.footY - f.h * 1.08f, w * 1.5f, f.h * 1.2f).toNearestInt();
        }
    }
};
