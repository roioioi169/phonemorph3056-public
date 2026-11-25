#include "PluginEditor.h"

PhonemorphEditor::PhonemorphEditor (PhonemorphProcessor& p)
    : AudioProcessorEditor (&p), proc (p), scope (p)
{
    addAndMakeVisible (scope);

    struct Spec { const char* id; const char* name; double def; };
    const Spec specs[] = {
        { "morph", "MORPH", 45 }, { "transientGuard", "GUARD", 75 },
        { "vacuum", "VACUUM", 50 }, { "throat", "THROAT", 55 },
        { "breath", "BREATH", 25 }, { "phantomLow", "PHANTOM", 60 },
        { "tinySpeaker", "TINY", 50 }, { "outputGain", "OUTPUT", 0 }
    };

    for (const auto& s : specs)
    {
        auto k = std::make_unique<Knob>();
        k->slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        k->slider.setLookAndFeel (&laf);
        k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
        k->slider.setDoubleClickReturnValue (true, s.def);

        k->label.setText (s.name, juce::dontSendNotification);
        k->label.setJustificationType (juce::Justification::centred);
        k->label.setColour (juce::Label::textColourId, juce::Colour (0xff9ff7ff));

        addAndMakeVisible (k->slider);
        addAndMakeVisible (k->label);

        k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            proc.apvts, s.id, k->slider);

        knobs.push_back (std::move (k));
    }

    setSize (560, 420);
}

PhonemorphEditor::~PhonemorphEditor()
{
    for (auto& k : knobs)
        k->slider.setLookAndFeel (nullptr);
}

void PhonemorphEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff070D12));
    g.setColour (juce::Colour (0xff00F0FF));
    g.setFont (juce::FontOptions (18.f, juce::Font::bold));
    g.drawText ("PHONEMORPH 3056", getLocalBounds().removeFromBottom (24).reduced (12, 0),
                juce::Justification::centredLeft);
}

void PhonemorphEditor::resized()
{
    auto area = getLocalBounds().reduced (12);
    area.removeFromBottom (24);
    scope.setBounds (area.removeFromTop (150));
    area.removeFromTop (8);

    constexpr int cols = 4;
    const int cellW = area.getWidth() / cols;
    const int cellH = area.getHeight() / 2;

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        juce::Rectangle<int> cell ((int) (area.getX() + (int) (i % cols) * cellW),
                                   area.getY() + (int) (i / cols) * cellH,
                                   cellW, cellH);
        cell = cell.reduced (4);
        knobs[i]->label.setBounds (cell.removeFromTop (18));
        knobs[i]->slider.setBounds (cell);
    }
}
