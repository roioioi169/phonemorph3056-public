#include "PluginProcessor.h"
#include "PluginEditor.h"

PhonemorphProcessor::PhonemorphProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout PhonemorphProcessor::createLayout()
{
    using NR = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto pct = [&] (const char* id, const char* name, float def)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, NR (0.f, 100.f, 0.01f), def,
            juce::AudioParameterFloatAttributes().withLabel ("%")));
    };

    pct ("morph",        "Morph",         45.f);
    pct ("transientGuard","Transient Guard", 75.f);
    pct ("vacuum",       "Vacuum",        50.f);
    pct ("throat",       "Throat",        55.f);
    pct ("breath",       "Breath",        25.f);
    pct ("phantomLow",   "Phantom Low",   60.f);
    pct ("tinySpeaker",  "Tiny Speaker",  50.f);

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "outputGain", 1 }, "Output Gain",
        NR (-18.f, 12.f, 0.01f), 0.f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    return { params.begin(), params.end() };
}

void PhonemorphProcessor::prepareToPlay (double sampleRate, int)
{
    chain.prepare (sampleRate);
    setLatencySamples (chain.latencySamples());
}

bool PhonemorphProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    return in == out;
}

void PhonemorphProcessor::pushParamsToChain()
{
    Phonemorph::MorphChain::Params p;
    p.morph         = apvts.getRawParameterValue ("morph")->load()         * 0.01f;
    p.transientGuard= apvts.getRawParameterValue ("transientGuard")->load() * 0.01f;
    p.vacuum        = apvts.getRawParameterValue ("vacuum")->load()        * 0.01f;
    p.throat        = apvts.getRawParameterValue ("throat")->load()        * 0.01f;
    p.breath        = apvts.getRawParameterValue ("breath")->load()        * 0.01f;
    p.phantomLow    = apvts.getRawParameterValue ("phantomLow")->load()    * 0.01f;
    p.tinySpeaker   = apvts.getRawParameterValue ("tinySpeaker")->load()   * 0.01f;
    p.outGain       = juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("outputGain")->load());
    chain.setTargets (p);
}

void PhonemorphProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    if (buffer.getNumChannels() == 0)
        return;

    pushParamsToChain();

    const int n = buffer.getNumSamples();
    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < n; ++i)
    {
        float l = L[i];
        float r = R ? R[i] : l;
        chain.processSample (l, r);
        L[i] = l;
        if (R) R[i] = r;
    }

    visVacuum.store (chain.visVacuum());
    visF1.store     (chain.visF1Hz());
    visF2.store     (chain.visF2Hz());
}

juce::AudioProcessorEditor* PhonemorphProcessor::createEditor()
{
    return new PhonemorphEditor (*this);
}

void PhonemorphProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void PhonemorphProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PhonemorphProcessor();
}
