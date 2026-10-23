#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float maxDelaySeconds = 4.0f;

    float tapeSaturate (float x, float drive)
    {
        const float amount = 1.0f + drive * 7.0f;
        return std::tanh (x * amount) / std::tanh (amount);
    }
}

RiddimLabAudioProcessor::RiddimLabAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

RiddimLabAudioProcessor::APVTS::ParameterLayout RiddimLabAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout p;
    using Float = juce::AudioParameterFloat;
    using Choice = juce::AudioParameterChoice;

    p.add (std::make_unique<Choice> ("division", "Division",
        juce::StringArray { "1/4", "1/8", "1/8D", "1/8T", "1/16", "1/4D" }, 2));
    p.add (std::make_unique<Float> ("time", "Free Time (ms)",
        juce::NormalisableRange<float> (30.0f, 1200.0f, 1.0f), 360.0f));
    p.add (std::make_unique<Float> ("feedback", "Feedback",
        juce::NormalisableRange<float> (0.0f, 0.94f, 0.001f), 0.46f));
    p.add (std::make_unique<Float> ("mix", "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.26f));
    p.add (std::make_unique<Float> ("cutoff", "Filter Cutoff",
        juce::NormalisableRange<float> (180.0f, 12000.0f, 1.0f, 0.35f), 2800.0f));
    p.add (std::make_unique<Float> ("resonance", "Resonance",
        juce::NormalisableRange<float> (0.1f, 0.92f, 0.001f), 0.35f));
    p.add (std::make_unique<Float> ("drive", "Tape Heat",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.28f));
    p.add (std::make_unique<Float> ("width", "Stereo Offset",
        juce::NormalisableRange<float> (0.0f, 0.5f, 0.001f), 0.14f));
    p.add (std::make_unique<Float> ("damping", "Repeat Damping",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.42f));
    p.add (std::make_unique<Float> ("tapeSat", "Tape Saturation", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.18f));
    p.add (std::make_unique<Float> ("tapeTone", "Tape Tone", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.72f));
    p.add (std::make_unique<Float> ("wow", "Wow", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.05f));
    p.add (std::make_unique<Float> ("flutter", "Flutter", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.04f));
    p.add (std::make_unique<Float> ("wear", "Tape Wear", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    p.add (std::make_unique<Choice> ("filterMode", "Filter Mode", juce::StringArray { "Low-pass", "High-pass" }, 0));
    p.add (std::make_unique<juce::AudioParameterBool> ("sync", "Tempo Sync", true));
    p.add (std::make_unique<juce::AudioParameterBool> ("throw", "Feedback Throw", false));
    p.add (std::make_unique<juce::AudioParameterBool> ("panic", "Panic / Clear", false));
    p.add (std::make_unique<juce::AudioParameterBool> ("bypass", "Effect Bypass", false));

    // UI-selected processing context and Guitar mode.
    p.add (std::make_unique<juce::AudioParameterInt> ("activeSection", "Active Section", 0, 6, 0));
    p.add (std::make_unique<juce::AudioParameterInt> ("guitarMode", "Guitar Mode", 0, 2, 0));
    p.add (std::make_unique<Float> ("guitarChopTightness", "Guitar Chop Tightness", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    p.add (std::make_unique<Float> ("guitarPickBrightness", "Guitar Pick Brightness", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    p.add (std::make_unique<Float> ("guitarRhythmBody", "Guitar Rhythm Body", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

    // Independent control groups used by the instrument-specific processing path.
    const juce::StringArray sectionNames { "master", "drums", "guitar", "bass", "keys", "horns", "vocal" };
    for (const auto& section : sectionNames)
    {
        p.add (std::make_unique<Float> (section + "Drive", section + " Drive",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.15f));
        p.add (std::make_unique<Float> (section + "Tone", section + " Tone",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.65f));
        p.add (std::make_unique<Float> (section + "Comp", section + " Compression",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.2f));
    }
    return p;
}

void RiddimLabAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    const int length = juce::roundToInt (sampleRate * maxDelaySeconds) + samplesPerBlock + 8;
    delayBuffer.setSize (2, length);
    delayBuffer.clear();
    writePosition = 0;

    for (int ch = 0; ch < 2; ++ch)
    {
        filters[ch].reset();
        filters[ch].setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        filters[ch].prepare ({ sampleRate, (juce::uint32) samplesPerBlock, 1 });
        tapeToneFilters[ch].reset();
        tapeToneFilters[ch].setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        tapeToneFilters[ch].prepare ({ sampleRate, (juce::uint32) samplesPerBlock, 1 });
    }
    for (int section = 0; section < 7; ++section)
        for (int ch = 0; ch < 2; ++ch)
        {
            instrumentFilters[section][ch].reset();
            instrumentFilters[section][ch].setType (juce::dsp::StateVariableTPTFilterType::lowpass);
            instrumentFilters[section][ch].prepare ({ sampleRate, (juce::uint32) samplesPerBlock, 1 });
        }

    juce::dsp::Reverb::Parameters rv;
    rv.roomSize = 0.72f; rv.damping = 0.42f; rv.wetLevel = 1.0f; rv.dryLevel = 0.0f; rv.width = 1.0f;
    springReverb.setParameters (rv);
    rv.roomSize = 0.88f; rv.damping = 0.28f;
    plateReverb.setParameters (rv);
    wowPhase = flutterPhase = 0.0;

    mixSmoothed.reset (sampleRate, 0.025);
    feedbackSmoothed.reset (sampleRate, 0.035);
    cutoffSmoothed.reset (sampleRate, 0.03);
    driveSmoothed.reset (sampleRate, 0.03);
    dampingSmoothed.reset (sampleRate, 0.03);

    mixSmoothed.setCurrentAndTargetValue (parameters.getRawParameterValue ("mix")->load());
    feedbackSmoothed.setCurrentAndTargetValue (parameters.getRawParameterValue ("feedback")->load());
    cutoffSmoothed.setCurrentAndTargetValue (parameters.getRawParameterValue ("cutoff")->load());
    driveSmoothed.setCurrentAndTargetValue (parameters.getRawParameterValue ("drive")->load());
    dampingSmoothed.setCurrentAndTargetValue (parameters.getRawParameterValue ("damping")->load());
    panicGain = 1.0f;
    wasPanic = false;
}

void RiddimLabAudioProcessor::releaseResources()
{
    delayBuffer.setSize (0, 0);
}

bool RiddimLabAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return in == out && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void RiddimLabAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = juce::jmin (2, buffer.getNumChannels());
    float blockPeak[2] { 0.0f, 0.0f };
    const int totalSamples = delayBuffer.getNumSamples();
    if (channels == 0 || totalSamples < 4) return;

    const auto get = [this] (const juce::String& id) { return parameters.getRawParameterValue (id)->load(); };
    if (get ("bypass") > 0.5f) return;

    double bpm = 120.0;
    if (auto* playHead = getPlayHead())
        if (auto pos = playHead->getPosition())
            if (auto hostBpm = pos->getBpm())
                bpm = juce::jlimit (30.0, 300.0, *hostBpm);

    const int activeSection = juce::jlimit (0, 6, juce::roundToInt (get ("activeSection")));
    const int guitarModeIndex = juce::jlimit (0, 2, juce::roundToInt (get ("guitarMode")));
    const float guitarChopTightness = get ("guitarChopTightness");
    const float guitarPickBrightness = get ("guitarPickBrightness");
    const float guitarRhythmBody = get ("guitarRhythmBody");
    const juce::StringArray sectionIds { "master", "drums", "guitar", "bass", "keys", "horns", "vocal" };
    const auto sectionId = sectionIds[activeSection];
    const float sectionDrive = get (sectionId + "Drive");
    const float sectionTone = get (sectionId + "Tone");
    const float sectionComp = get (sectionId + "Comp");
    const float tapeSat = get ("tapeSat");
    const float tapeTone = get ("tapeTone");
    const float wow = get ("wow");
    const float flutter = get ("flutter");
    const float wear = get ("wear");
    const bool highPass = get ("filterMode") > 0.5f;
    const bool sync = get ("sync") > 0.5f;
    const int division = juce::roundToInt (get ("division"));
    const double beatMs = 60000.0 / bpm;
    const double multipliers[] = { 1.0, 0.5, 0.75, 1.0 / 3.0, 0.25, 1.5 };
    const double delayMs = sync ? beatMs * multipliers[juce::jlimit (0, 5, division)]
                                : (double) get ("time");
    const int baseDelay = juce::jlimit (1, totalSamples - 2,
        juce::roundToInt ((delayMs / 1000.0) * currentSampleRate));
    const float offset = get ("width") * 0.5f;
    delaySamples[0] = baseDelay;
    delaySamples[1] = juce::jlimit (1, totalSamples - 2,
        juce::roundToInt (baseDelay * (1.0f + offset)));

    const bool throwOn = get ("throw") > 0.5f;
    const bool panicOn = get ("panic") > 0.5f;
    if (panicOn && ! wasPanic)
    {
        delayBuffer.clear();
        for (auto& f : filters) f.reset();
        panicGain = 0.0f;
    }
    wasPanic = panicOn;

    const float targetFeedback = throwOn ? juce::jmin (0.94f, get ("feedback") + 0.22f) : get ("feedback");
    mixSmoothed.setTargetValue (get ("mix"));
    feedbackSmoothed.setTargetValue (targetFeedback);
    cutoffSmoothed.setTargetValue (get ("cutoff"));
    driveSmoothed.setTargetValue (get ("drive"));
    dampingSmoothed.setTargetValue (get ("damping"));

    for (int ch = 0; ch < channels; ++ch)
        filters[ch].setResonance (get ("resonance"));

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const float mix = mixSmoothed.getNextValue();
        const float feedback = juce::jlimit (0.0f, 0.94f, feedbackSmoothed.getNextValue());
        const float cutoff = cutoffSmoothed.getNextValue();
        const float drive = driveSmoothed.getNextValue();
        const float damping = dampingSmoothed.getNextValue();

        for (int ch = 0; ch < channels; ++ch)
        {
            filters[ch].setCutoffFrequency (cutoff);
            filters[ch].setType (highPass ? juce::dsp::StateVariableTPTFilterType::highpass
                                          : juce::dsp::StateVariableTPTFilterType::lowpass);
            float dry = buffer.getSample (ch, i);
            // Tape stage: smooth saturation, tone loss, and subtle deterministic wow/flutter.
            tapeToneFilters[ch].setCutoffFrequency (juce::jmap (tapeTone, 0.0f, 1.0f, 2200.0f, 20000.0f));
            const float softened = tapeToneFilters[ch].processSample (0, dry);
            const float tapeBlend = tapeSaturate (dry, tapeSat * 0.85f);
            const float lofi = dry * (1.0f - wear * 0.28f) + softened * wear * 0.28f;
            const float modulation = 1.0f + std::sin ((float) wowPhase) * wow * 0.0035f
                                        + std::sin ((float) flutterPhase) * flutter * 0.0012f;
            dry = ((lofi * (1.0f - tapeSat * 0.45f)) + (tapeBlend * tapeSat * 0.45f)) * modulation;
            int readPos = writePosition - delaySamples[ch];
            if (readPos < 0) readPos += totalSamples;
            const float delayed = delayBuffer.getSample (ch, readPos);
            const float filtered = filters[ch].processSample (0, delayed);
            const float damped = filtered * (1.0f - damping * 0.58f);
            const float tape = tapeSaturate (damped, drive);
            const float write = juce::jlimit (-1.25f, 1.25f, dry + tape * feedback);
            delayBuffer.setSample (ch, writePosition, write);

            if (ch == 0 && panicGain < 1.0f)
                panicGain = juce::jmin (1.0f, panicGain + 0.0008f);

            float processed = dry * (1.0f - mix) + delayed * mix * panicGain;

            // Distinct audio-processing path for the currently selected section.
            // All sections have independent parameter groups and filter state.
            const float cutoffHz = [&]()
            {
                switch (activeSection)
                {
                    case 1: return juce::jmap (sectionTone, 0.0f, 1.0f, 1800.0f, 16000.0f); // drums: attack/body
                    case 2: return juce::jmap (sectionTone, 0.0f, 1.0f, 1200.0f, 15000.0f); // guitar: skank/pick
                    case 3: return juce::jmap (sectionTone, 0.0f, 1.0f, 90.0f, 1200.0f);    // bass: low-pass
                    case 4: return juce::jmap (sectionTone, 0.0f, 1.0f, 2500.0f, 18000.0f); // keys: sheen
                    case 5: return juce::jmap (sectionTone, 0.0f, 1.0f, 2200.0f, 17000.0f); // horns: presence
                    case 6: return juce::jmap (sectionTone, 0.0f, 1.0f, 3500.0f, 14000.0f); // vocal: de-ess-ish tone
                    default: return juce::jmap (sectionTone, 0.0f, 1.0f, 1000.0f, 19000.0f);
                }
            }();
            instrumentFilters[activeSection][ch].setCutoffFrequency (cutoffHz);
            instrumentFilters[activeSection][ch].setType (juce::dsp::StateVariableTPTFilterType::lowpass);
            float toneProcessed = instrumentFilters[activeSection][ch].processSample (0, processed);
            const float toneMix = activeSection == 3 ? 0.85f : 0.22f;
            processed += (toneProcessed - processed) * toneMix;

            // Section-specific dynamics/saturation character.
            const float sat = tapeSaturate (processed, sectionDrive * (activeSection == 1 ? 1.25f : 0.9f));
            processed = processed * (1.0f - sectionDrive * 0.55f) + sat * sectionDrive * 0.55f;
            const float threshold = activeSection == 1 || activeSection == 6 ? 0.58f : 0.78f;
            const float compAmount = sectionComp * (activeSection == 1 ? 0.62f : activeSection == 6 ? 0.68f : 0.42f);
            const float magnitude = std::abs (processed);
            if (magnitude > threshold)
                processed = std::copysign (threshold + (magnitude - threshold) * (1.0f - compAmount), processed);

            if (activeSection == 2)
            {
                if (guitarModeIndex == 0) // CHOP: tighter transient, reduced low-mid bloom
                {
                    processed *= (1.0f - 0.12f * guitarChopTightness);
                    processed = std::tanh (processed * (1.0f + guitarChopTightness * 0.35f));
                }
                else if (guitarModeIndex == 1) // PICK: brighter, more defined attack
                {
                    const float bright = juce::jlimit (-1.0f, 1.0f, processed - sectionEnvelope[2][ch]);
                    processed += bright * (0.08f + 0.22f * guitarPickBrightness);
                }
                else // RHYTHM: fuller body, restrained peaks
                {
                    processed = processed * (0.85f + 0.15f * guitarRhythmBody);
                    processed = std::tanh (processed * 1.08f);
                }
                sectionEnvelope[2][ch] = 0.995f * sectionEnvelope[2][ch] + 0.005f * processed;
            }
            if (activeSection == 2 || activeSection == 4)
                processed = 0.985f * processed + 0.015f * std::sin (processed * 1.7f); // subtle harmonic body
            if (activeSection == 5)
                processed = 0.94f * processed + 0.06f * std::tanh (processed * 1.8f); // brass presence
            if (activeSection == 6)
                processed = std::tanh (processed * (1.0f + sectionComp * 1.5f)); // vocal density

            buffer.setSample (ch, i, processed);
            blockPeak[ch] = juce::jmax (blockPeak[ch], std::abs (processed));
        }
        writePosition = (writePosition + 1) % totalSamples;
    }
    for (int ch = 0; ch < 2; ++ch)
        outputPeak[ch].store (juce::jmax (blockPeak[ch], outputPeak[ch].load() * 0.82f), std::memory_order_relaxed);
}

juce::AudioProcessorEditor* RiddimLabAudioProcessor::createEditor()
{
    return new RiddimLabAudioProcessorEditor (*this);
}

void RiddimLabAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void RiddimLabAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RiddimLabAudioProcessor();
}
