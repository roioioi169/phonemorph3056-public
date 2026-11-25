#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "DSP/MorphEngines.h"

class PhonemorphProcessor : public juce::AudioProcessor
{
public:
    PhonemorphProcessor();
    ~PhonemorphProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "PHONEMORPH 3056"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.05; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Visualizer feed (written in processBlock, read by editor timer)
    std::atomic<float> visVacuum { 0.f }, visF1 { 500.f }, visF2 { 1500.f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void pushParamsToChain();

    Phonemorph::MorphChain chain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhonemorphProcessor)
};
