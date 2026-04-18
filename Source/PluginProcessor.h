#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PadEngine.h"

class OctoPadAudioProcessor : public juce::AudioProcessor
{
public:
    OctoPadAudioProcessor();
    ~OctoPadAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "OctoPad"; }
    bool acceptsMidi()  const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void pushParamsToEngine();

    octopad::PadEngine engine;
    int currentProgram = 0;

    // Sustain pedal (CC64) handling — notes released while pedal is down
    // are held, then released together when the pedal goes up.
    bool sustainPedalDown = false;
    std::array<bool, 128> heldByPedal { {} };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OctoPadAudioProcessor)
};
