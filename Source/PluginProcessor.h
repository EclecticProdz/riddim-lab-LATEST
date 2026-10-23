#pragma once
#include <JuceHeader.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "RiddimParams.h"
#include "RiddimDSP.h"

class RiddimLabAudioProcessor : public juce::AudioProcessor
{
public:
    RiddimLabAudioProcessor();
    ~RiddimLabAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Riddim Lab"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 16.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS parameters;

    // Meters (read by the editor, written by the audio thread).
    float getInputPeak  (int ch) const noexcept { return inPeak[juce::jlimit (0, 1, ch)].load(); }
    float getOutputPeak (int ch) const noexcept { return outPeak[juce::jlimit (0, 1, ch)].load(); }
    float getCompGainReductionDb() const noexcept    { return compGr.load(); }
    float getLimiterGainReductionDb() const noexcept { return limGr.load(); }

    // The id of a parameter for a given profile (tab).
    static juce::String paramId (rp::Sec s, rp::P p) { return rp::id (s, p); }

private:
    static APVTS::ParameterLayout createParameterLayout();
    float raw (rp::Sec s, rp::P p) const noexcept;
    void runChunk (float* l, float* r, int n, int sec, bool bypass, bool throwOn, bool panic, double bpm, int guitarMode);

    static constexpr int kChunk = 512;
    double sr = 44100.0;

    std::atomic<float>* ptr[rp::NumSec][rp::NumP] {};
    std::atomic<float>* pBypass = nullptr; std::atomic<float>* pThrow = nullptr; std::atomic<float>* pPanic = nullptr;
    std::atomic<float>* pSection = nullptr; std::atomic<float>* pGuitarMode = nullptr;

    riddim::SourceStage source;
    riddim::Compressor comp;
    riddim::SteadyBass steady;
    riddim::ToneEQ eq;
    riddim::TapeStage tape;
    riddim::DubEcho echo;
    riddim::RhythmDelay delay;
    riddim::ModStage mod;
    riddim::ReverbStage reverb;
    riddim::Limiter limiter;
    riddim::Biquad inFilt[2], anaLow[2], anaHigh[2];

    std::vector<float> L, R;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inGainS, outGainS;
    bool echoWasOn = true, delayWasOn = true, modWasOn = true, revWasOn = true;
    bool primed = false;
    float ipk[2] { 0.0f, 0.0f };

    std::atomic<float> inPeak[2] { 0.0f, 0.0f }, outPeak[2] { 0.0f, 0.0f }, compGr { 0.0f }, limGr { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RiddimLabAudioProcessor)
};
