#pragma once
#include "PluginProcessor.h"
class RiddimLabAudioProcessorEditor : public juce::AudioProcessorEditor {
public: explicit RiddimLabAudioProcessorEditor (RiddimLabAudioProcessor& p) : AudioProcessorEditor (p) {}
};
