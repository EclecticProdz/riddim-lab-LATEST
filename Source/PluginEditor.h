#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "RiddimUI.h"

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

private:
    struct KnobDef { std::unique_ptr<ui::Knob> k; rp::P p; int fmt; bool echoTime = false; };
    struct LibItem;

    ui::Knob* addKnob (rp::P p, const juce::String& caption, int diameter, int fmt, bool echoTime = false);
    ui::Lamp* addLamp (std::unique_ptr<ui::Lamp> l);
    ui::ChoiceLamps* addChoice (std::unique_ptr<ui::ChoiceLamps> c);
    void place (juce::Component& c, float cx, int top, int w, int h);

    void selectTab (int section, int guitarMode, bool writeParams);
    void layoutControls();
    void applyPianoVoicing (int which);
    bool pianoVoicingMatches (int which) const;
    static juce::String tabTitle (int section);
    void rebindAll();
    void updateEchoTimeBinding();
    void setInfo (const juce::String&);

    // presets
    void applyFactoryPreset (int index);
    void applyUserPreset (const juce::File&);
    void setParam (rp::Sec, rp::P, float);
    void saveUserPreset();
    void deleteCurrentPreset();
    void showLoadMenu();
    void refreshPresetBox();
    void stepPreset (int delta);
    void setCurrentPreset (const juce::String& name, int factoryIndex, const juce::File& file);
    static juce::File presetFolder();

    void timerCallback() override;

    RiddimLabAudioProcessor& proc;
    ui::LookAndFeel look;
    int sec = 0, gMode = 0;
    juce::String infoText;

    // layout rectangles
    enum class BKind { Src, Steady, Filter, Comp, Eq, Limiter };
    struct BPanel { BKind kind; juce::Rectangle<int> r; };
    struct FlowItem { juce::String label; rp::P param; bool master; };
    std::vector<BPanel> rowB;
    std::vector<FlowItem> flowItems;
    juce::Rectangle<int> rHeaderArt, rVuPanel, rPreset, rLibrary, rInfo,
        rInput, rMeters, rOutput, rFlow, rTape, rEcho, rReverb;

    // widgets
    juce::TextButton nav[7], sub[3], oldSchoolB, modernB;
    std::vector<std::unique_ptr<KnobDef>> knobs;
    std::vector<std::unique_ptr<ui::Lamp>> lamps;
    std::vector<std::unique_ptr<ui::ChoiceLamps>> choices;
    ui::Lamp *lSat = nullptr, *lAnalog = nullptr, *lSync = nullptr, *lSoft = nullptr, *lBypass = nullptr, *lThrow = nullptr, *lPanic = nullptr,
             *pFilt = nullptr, *pComp = nullptr, *pEq = nullptr, *pTape = nullptr, *pEcho = nullptr, *pRev = nullptr, *pLim = nullptr, *pSrc = nullptr, *pSteady = nullptr;
    ui::Lamp* flow[9] {};
    ui::ChoiceLamps *cFilterMode = nullptr, *cFilterRoute = nullptr, *cCompMode = nullptr, *cTapeType = nullptr, *cEchoType = nullptr, *cRevType = nullptr;
    std::vector<ui::Knob*> limKnobs, srcKnobs;
    ui::Knob* echoTimeKnob = nullptr;

    ui::VuMeter meterL { true, false }, meterR { true, false }, meterIn { false, false }, meterGr { false, true };
    ui::LedLadder ladder;
    ui::TapeDeck deck;
    float needle[4] { 0, 0, 0, 0 };

    juce::ComboBox presetBox;
    juce::TextButton prevB, nextB, loadB, saveB, delB, browseB;
    juce::Label presetInfo;
    std::vector<std::unique_ptr<LibItem>> lib;
    std::vector<juce::File> userPresets;
    int currentFactory = -1; juce::File currentUserFile; juce::String currentName;

    std::unique_ptr<juce::ParameterAttachment> secAtt, modeAtt, syncAtt;
    bool syncOn = true;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainUI)
};

class RiddimLabAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit RiddimLabAudioProcessorEditor (RiddimLabAudioProcessor&);
    void resized() override;
private:
    MainUI main;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RiddimLabAudioProcessorEditor)
};
