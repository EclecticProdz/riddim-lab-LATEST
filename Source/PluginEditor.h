#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "RiddimUI.h"
#include "RoomFx.h"

// The whole interface lives in MainUI at a fixed 1280 x 900 layout; the editor
// wraps it and scales it to whatever window size the host gives us.
class MainUI : public juce::Component, private juce::Timer
{
public:
    explicit MainUI (RiddimLabAudioProcessor&);
    ~MainUI() override;

    static constexpr int kW = 1280, kH = 900;
    void paint (juce::Graphics&) override;
    void resized() override;

    // pages: instrument tabs map onto the engine's profile sections, the rest are views
    void lightLighter() { roomFx.toggleLighter(); }
    void showPage (int p) { selectPage ((Page) juce::jlimit (0, (int) NumPages - 1, p), gMode, false); }
    enum Page { PHome, PMaster, PDrums, PGuitar, PBass, PKeys, PHorns, PControl, PFx, PTape, PSpring, PPlate, PEcho, NumPages };

private:
    struct KnobDef { std::unique_ptr<ui::Knob> k; rp::P p; int fmt; bool echoTime = false; };

    ui::Knob* addKnob (rp::P p, const juce::String& caption, int diameter, int fmt, bool echoTime = false);
    ui::Lamp* addLamp (std::unique_ptr<ui::Lamp> l);
    ui::ChoiceLamps* addChoice (std::unique_ptr<ui::ChoiceLamps> c);
    ui::Knob* knobFor (rp::P p);

    // navigation
    void selectPage (Page page, int guitarMode, bool writeParams);
    static int sectionOfPage (Page p);                  // -1: page keeps the current section
    static Page pageOfSection (int s);
    static bool isEffectPage (Page p) { return p == PTape || p == PSpring || p == PPlate || p == PEcho; }
    static bool isInstrumentPage (Page p) { return p == PMaster || p == PDrums || p == PGuitar || p == PBass || p == PKeys || p == PHorns; }
    void showGuitarMenu();
    void showEffectsMenu();
    void showSettingsMenu();
    void buildHotspots();

    // layout
    enum class MK { Src, Steady, Filter, Comp, Eq, Limiter, TapeW, Reverb,
                    HomeIn, HomeMeters, HomeOut, HomeLim, HomeFlow,
                    TapeMain, TapeTrans, TapeOut, RevMain, RevOut, EchoMain, EchoHeat, EchoFilter,
                    FxCard, Soon, TapeMore, SpChar, SpKnobs, SpTone, SpOut, PlMain, PlTone, PlOut, Sub, Spk, DrumSrc, Lock, CtlMon, CtlRoom, CtlPos, CtlColour, CtlListen };
    struct Mod { MK k; juce::Rectangle<int> r; int arg = 0; };
    int naturalWidth (MK k, int dia) const;
    void buildModules();
    void layoutControls();
    void hideAllControls();
    void knobRow (std::initializer_list<rp::P> ps, juce::Rectangle<int> area, int dia, ui::Mat m, int top = 0);
    void styleKnob (ui::Knob*, ui::Mat, int dia);
    void styleWidget (ui::ParamWidget*, ui::Mat);
    void power (ui::Lamp*, const juce::Rectangle<int>& panel);
    void applyPianoVoicing (int which);
    bool pianoVoicingMatches (int which) const;
    static juce::String pageTitle (Page);
    static juce::String pageSubtitle (Page);
    void rebindAll();
    void updateEchoTimeBinding();
    void setInfo (const juce::String&);
    void paintModuleExtras (juce::Graphics&, const Mod&);

    // presets (one strip per page; instrument tabs save the whole tab, effect pages save only their effect)
    struct Bank;
    const Bank& bankFor (Page) const;
    void applyFactoryPreset (int index);
    void applyUserPreset (const juce::File&);
    void setParam (rp::Sec, rp::P, float);
    void saveUserPreset();
    void deleteCurrentPreset();
    void showLoadMenu();
    void refreshUserPresets();
    void stepPreset (int delta);
    void setCurrentPreset (const juce::String& name, int factoryIndex, const juce::File& file);
    void saveWholePlugin();
    void loadWholePlugin();
    static juce::File presetFolder();

    void timerCallback() override;

    RiddimLabAudioProcessor& proc;
    ui::LookAndFeel look;
    Page page = PHome;
    int sec = 0, gMode = 0;
    juce::String infoText;

    std::vector<Mod> mods;
    int moduleDia = 58;
    int bandTop = 566;
    juce::Rectangle<int> rHeader, rMeterPanel, rRail, rRoom, rInfo;

    // widgets
    std::unique_ptr<ui::NavTab> tab[9];
    ui::CogButton cog;
    ui::PresetStrip strip;
    juce::TextButton backB, styleB[3], oldSchoolB, modernB, openB[4], spkB[4], drumB[6], monB[3], roomB[3];
    std::vector<std::unique_ptr<ui::Hotspot>> hotspots;
    RoomFx roomFx;
    void updateRoomFx();
    int lastScKey = -1;
    std::vector<std::unique_ptr<KnobDef>> knobs;
    std::vector<std::unique_ptr<ui::Lamp>> lamps;
    std::vector<std::unique_ptr<ui::ChoiceLamps>> choices;
    ui::Lamp *lMono = nullptr, *lPhones = nullptr, *lLock = nullptr, *lSat = nullptr, *lAnalog = nullptr, *lSync = nullptr, *lSoft = nullptr, *lBypass = nullptr, *lThrow = nullptr, *lPanic = nullptr,
             *pFilt = nullptr, *pComp = nullptr, *pEq = nullptr, *pTape = nullptr, *pEcho = nullptr, *pRev = nullptr, *pLim = nullptr, *pSrc = nullptr, *pSteady = nullptr;
    ui::Lamp* flow[9] {};
    ui::Lamp* cardLamp[4] {};
    std::unique_ptr<ui::Knob> cardKnob[4];
    ui::ChoiceLamps *cFilterMode = nullptr, *cFilterRoute = nullptr, *cCompMode = nullptr, *cTapeType = nullptr, *cEchoType = nullptr, *cRevType = nullptr,
                    *cTapeSpeed = nullptr, *cSpChar = nullptr, *cSpLen = nullptr, *cSpStereo = nullptr, *cPlTone = nullptr, *cLock = nullptr;
    ui::Lamp *head[3] {};
    std::vector<ui::Knob*> limKnobs, srcKnobs;
    ui::Knob* echoTimeKnob = nullptr;
    ui::Fader fIn { "GAIN" }, fOut { "LEVEL" }, fCeil { "CEILING" };
    ui::LedColumn ledIn, ledOut, ledGr { true, 14 }, ledSub { false, 14 };

    struct FlowItem { juce::String label; rp::P param; bool master; };
    std::vector<FlowItem> flowItems;

    ui::VuMeter meterL { true, false }, meterR { true, false }, meterIn { false, false }, meterGr { false, true };
    ui::LedLadder ladder;
    float needle[4] { 0, 0, 0, 0 };

    std::vector<juce::File> userPresets;
    int currentFactory = -1; juce::File currentUserFile; juce::String currentName;
    std::shared_ptr<juce::FileChooser> chooser;

    std::unique_ptr<juce::ParameterAttachment> secAtt, modeAtt, syncAtt;
    bool syncOn = true;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainUI)
};

class RiddimLabAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit RiddimLabAudioProcessorEditor (RiddimLabAudioProcessor&);
    void resized() override;
    void showPage (int p) { main.showPage (p); }
    void lightLighter() { main.lightLighter(); }
private:
    MainUI main;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RiddimLabAudioProcessorEditor)
};
