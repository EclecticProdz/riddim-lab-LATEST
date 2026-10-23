#pragma once
#include <JuceHeader.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class RiddimLabAudioProcessor : public juce::AudioProcessor
{
public:
    RiddimLabAudioProcessor();
    ~RiddimLabAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
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
    float getOutputPeak (int channel) const noexcept { return outputPeak[juce::jlimit (0, 1, channel)].load(); }

private:
    static APVTS::ParameterLayout createParameterLayout();

    juce::AudioBuffer<float> delayBuffer;
    int writePosition = 0;
    double currentSampleRate = 44100.0;
    juce::dsp::StateVariableTPTFilter<float> filters[2];
    juce::dsp::StateVariableTPTFilter<float> tapeToneFilters[2];
    juce::dsp::Reverb springReverb, plateReverb;
    // Dedicated per-section tone filters and envelope followers. Each section
    // has its own parameter group and a distinct processing path.
    juce::dsp::StateVariableTPTFilter<float> instrumentFilters[7][2];
    float sectionEnvelope[7][2] {};
    std::atomic<float> outputPeak[2] { 0.0f, 0.0f };
    double wowPhase = 0.0;
    double flutterPhase = 0.0;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed, feedbackSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> cutoffSmoothed, driveSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> dampingSmoothed;
    int delaySamples[2] { 18000, 20000 };
    float panicGain = 1.0f;
    bool wasPanic = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RiddimLabAudioProcessor)
};
