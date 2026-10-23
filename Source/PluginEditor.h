#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Look and feel: brass knobs, lamp switches, metal plate buttons and inset combo
// boxes. Everything is drawn in code so it stays sharp at any display scale.
class RiddimLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RiddimLookAndFeel();

    static juce::Font font (float size, bool bold = true);

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&,
                         bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getLabelFont (juce::Label&) override;
};

//==============================================================================
// Geometry and values for the animated parts (VU meters, LEDs, reels, lamps).
struct RiddimLiveState
{
    juce::Rectangle<int> meterL, meterR, ledStack, reelArea, pathArea;
    float needle[2] { 0.0f, 0.0f };   // 0..1 smoothed needle position
    float peak[2]   { 0.0f, 0.0f };   // raw post-processing peak
    float reelAngle = 0.0f;
    bool bypassed = false;
    bool playing = false;
};

// Transparent overlay that draws only the animated parts. It never takes mouse
// clicks, so it cannot get in the way of any control.
class RiddimLiveDisplay : public juce::Component
{
public:
    explicit RiddimLiveDisplay (RiddimLiveState& s) : state (s)
    {
        setInterceptsMouseClicks (false, false);
        setOpaque (false);
    }
    void paint (juce::Graphics&) override;
private:
    RiddimLiveState& state;
};

//==============================================================================
class RiddimLabAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit RiddimLabAudioProcessorEditor (RiddimLabAudioProcessor&);
    ~RiddimLabAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    enum Knob
    {
        TapeSat = 0, TapeTone, Wow, Flutter, Wear,
        Cutoff, Resonance,
        Time, Feedback, Mix, Heat, Offset, Damping,
        ChainDrive, ChainTone, ChainComp,
        GuitarChop, GuitarPick, GuitarBody,
        NumKnobs
    };

    void initKnob (int k);
    void placeKnob (int k, juce::Rectangle<int> cell);
    void setKnobVisible (int k, bool visible);

    void selectSection (int section, bool notifyHost);
    void selectGuitarMode (int mode, bool notifyHost);
    void rebuildChain();
    void layoutChain();
    void applyPreset (int index);
    void updatePresetDisplay();

    void setFloatParam (const char* id, float plainValue);
    void setBoolParam (const char* id, bool value);
    void setIntParam (const char* id, int value);

    void timerCallback() override;

    RiddimLabAudioProcessor& proc;

    // Declared first so it is destroyed last.
    RiddimLookAndFeel look;

    juce::Image headerArt, grain;
    RiddimLiveState live;
    RiddimLiveDisplay liveDisplay { live };

    // Layout rectangles, computed once per resize in resized().
    juce::Rectangle<int> rHeaderArt, rMeters, rPreset, rLibrary,
                         rTape, rFilter, rEcho, rChain, rPath, rSafety;

    int activeSection = 0;
    int activeGuitarMode = 0;
    int currentPreset = -1;
    bool throwHeld = false;

    juce::TextButton navButtons[7];
    juce::TextButton guitarModeButtons[3];
    juce::TextButton libButtons[8];
    juce::TextButton prevPreset, nextPreset, throwButton;
    juce::ComboBox presetBox, divisionBox, filterModeBox;
    juce::ToggleButton syncToggle, bypassToggle, panicToggle;
    juce::Label presetInfo, divisionLabel, filterTypeLabel;

    juce::Slider knobs[NumKnobs];
    juce::Label knobLabels[NumKnobs];

    // Attachments are declared after the controls they reference so they are
    // destroyed first.
    std::unique_ptr<SA> knobA[NumKnobs];
    std::unique_ptr<CA> divisionA, filterModeA;
    std::unique_ptr<BA> syncA, bypassA, panicA;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RiddimLabAudioProcessorEditor)
};
