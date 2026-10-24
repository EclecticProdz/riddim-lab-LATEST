#pragma once
#include <JuceHeader.h>

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
        findLights (img);
        buildFigures();
        place();
        repaint();
    }

    void resized() override { place(); }

    bool hitTest (int x, int y) override
    {
        for (auto& f : figs)
            if (f.click && f.box.reduced (f.box.getWidth() / 5, f.box.getHeight() / 8).contains (x, y)) return true;
        return false;
    }
    void mouseUp (const juce::MouseEvent&) override { toggleLighter(); }

    bool isLit() const noexcept { return lighter > 0.0f; }
    void toggleLighter()
    {
        if (page != 0) return;
        if (lighter > 0.0f) { lighter = 0.0f; repaint(); return; }
        lighter = 14.0f; spawnClock = 0.0f;
    }

    // called by the owner at about 30 Hz; level is the recent output peak (0..1)
    void tick (float level)
    {
        const float dt = 1.0f / 30.0f;
        time += dt; ++frame;
        beat += (level - beat) * (level > beat ? 0.5f : 0.12f);
        if (lighter > 0.0f)
        {
            lighter -= dt; spawnClock -= dt;
            if (spawnClock <= 0.0f && lighter > 0.0f) { spawnClock = 0.07f; spawn(); }
        }
        if ((frame & 1) == 0) for (auto& l : lights) repaint (l.box);
        // the musicians move every frame while there is signal, a little slower when idle
        if (beat > 0.01f || (frame % 3) == 0) for (auto& f : figs) repaint (f.box);
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
        if (! dirty.isEmpty()) repaint (dirty.expanded (4));
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
    struct Fig { int img; float cx, footY, h, sway, bob, phase, speed; bool click; juce::Rectangle<int> box; };
    std::vector<Light> lights; std::vector<Puff> puffs; std::vector<Fig> figs;
    juce::Image fig[8];
    int gridW = 96, gridH = 28, frame = 0, page = -1;
    float time = 0.0f, beat = 0.0f, lighter = 0.0f, spawnClock = 0.0f; bool lighterWasOn = false;
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
            case 1: figs.push_back ({ 5, 598.0f, 292.0f, 150.0f, 0.8f, 1.5f, 0.0f, 0.6f, false, {} }); break;   // engineer at the desk
            case 2: figs.push_back ({ 0, 636.0f, 300.0f, 215.0f, 0.9f, 5.0f, 0.0f, 1.0f, false, {} }); break;   // drummer
            case 3: figs.push_back ({ 2, 650.0f, 348.0f, 290.0f, 1.4f, 3.0f, 1.2f, 0.8f, false, {} }); break;   // guitarist
            case 4: figs.push_back ({ 1, 335.0f, 348.0f, 290.0f, 1.4f, 3.5f, 0.5f, 0.7f, false, {} }); break;   // bassist
            case 5: figs.push_back ({ 3, 628.0f, 305.0f, 205.0f, 0.7f, 2.5f, 0.3f, 0.7f, false, {} }); break;   // pianist
            case 6: figs.push_back ({ 4, 740.0f, 348.0f, 300.0f, 1.8f, 3.0f, 0.9f, 0.6f, false, {} }); break;   // sax
            case 7: figs.push_back ({ 5, 626.0f, 352.0f, 200.0f, 0.6f, 1.5f, 0.2f, 0.5f, false, {} }); break;   // engineer, back to us
            default: break;
        }
    }

    void paintFigure (juce::Graphics& g, const Fig& f)
    {
        using namespace juce;
        int idx = f.img;
        if (idx == 6 && lighter > 0.0f) idx = 7;
        const auto& im = fig[idx];
        if (! im.isValid()) return;
        const float s = f.h / (float) im.getHeight();
        const float ang = f.sway * 0.0174533f * std::sin (time * 6.2832f * 0.55f * f.speed + f.phase) + 0.012f * beat * std::sin (time * 9.0f + f.phase);
        const float bob = f.bob * beat * (0.6f + 0.4f * std::sin (time * 11.0f * f.speed));
        const float sy = 1.0f + 0.010f * beat + 0.004f * std::sin (time * 1.9f + f.phase);
        // soft shadow on the floor
        g.setGradientFill (ColourGradient (Colours::black.withAlpha (0.38f), f.cx, f.footY, Colours::black.withAlpha (0.0f), f.cx + f.h * 0.34f, f.footY, true));
        g.fillEllipse (f.cx - f.h * 0.34f, f.footY - f.h * 0.05f, f.h * 0.68f, f.h * 0.10f);
        const float ax = (float) im.getWidth() * 0.5f, ay = (float) im.getHeight();
        auto tf = AffineTransform::translation (-ax, -ay).scaled (1.0f, sy).rotated (ang).scaled (s, s).translated (f.cx, f.footY - bob);
        g.setOpacity (1.0f);
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
            const float w = f.h * 0.9f;
            f.box = juce::Rectangle<float> (f.cx - w * 0.7f, f.footY - f.h * 1.08f, w * 1.4f, f.h * 1.2f).toNearestInt();
        }
    }
};
