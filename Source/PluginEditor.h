#pragma once
#include "PluginProcessor.h"

//==============================================================================
// Glossy obsidian/chrome knob look. Drawn per frame here; caching is a TODO.
class AeroGlassLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                           float pos, float startA, float endA, juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.f);
        const float d = juce::jmin (bounds.getWidth(), bounds.getHeight());
        const auto c  = bounds.getCentre();
        const float r = d * 0.5f;

        juce::ColourGradient chrome (juce::Colour (0xffdfe7ea), c.x - r, c.y - r,
                                     juce::Colour (0xff4a5a62), c.x + r, c.y + r, false);
        g.setGradientFill (chrome);
        g.fillEllipse (c.x - r, c.y - r, d, d);

        const float cr = r * 0.72f;
        juce::ColourGradient glass (juce::Colour (0xff183038), c.x, c.y - cr,
                                    juce::Colour (0xff050a0d), c.x, c.y + cr, false);
        g.setGradientFill (glass);
        g.fillEllipse (c.x - cr, c.y - cr, cr * 2.f, cr * 2.f);

        const float ang = startA + pos * (endA - startA);
        juce::Path track, value;
        track.addCentredArc (c.x, c.y, r * 0.9f, r * 0.9f, 0.f, startA, endA, true);
        value.addCentredArc (c.x, c.y, r * 0.9f, r * 0.9f, 0.f, startA, ang, true);
        g.setColour (juce::Colour (0xff1d2a31));
        g.strokePath (track, juce::PathStrokeType (3.f));
        g.setColour (juce::Colour (0xff00F0FF));
        g.strokePath (value, juce::PathStrokeType (3.f));

        juce::Path notch;
        notch.addRectangle (-1.5f, -cr * 0.95f, 3.f, cr * 0.45f);
        g.setColour (juce::Colour (0xff39FF88));
        g.fillPath (notch, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
    }
};

//==============================================================================
// Lightweight scope: pauses its timer when hidden.
class AudioVisualScope : public juce::Component, private juce::Timer
{
public:
    explicit AudioVisualScope (PhonemorphProcessor& p) : proc (p) {}
    ~AudioVisualScope() override { stopTimer(); }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff070D12));
        g.fillRoundedRectangle (b, 12.f);

        const auto c = b.getCentre();
        const float R = juce::jmin (b.getWidth(), b.getHeight()) * 0.35f;
        const float rad = R * (1.f - 0.45f * proc.visVacuum.load());
        g.setColour (juce::Colour (0xff00F0FF));
        g.drawEllipse (c.x - rad, c.y - rad, rad * 2.f, rad * 2.f, 2.f);

        auto xFor = [&] (float hz)
        {
            const float t = std::log2 (hz / 100.f) / std::log2 (100.f);
            return b.getX() + b.getWidth() * juce::jlimit (0.f, 1.f, t);
        };

        g.setColour (juce::Colour (0xff39FF88));
        g.fillEllipse (xFor (proc.visF1.load()) - 4.f, b.getBottom() - 14.f, 8.f, 8.f);
        g.setColour (juce::Colour (0xff00F0FF));
        g.fillEllipse (xFor (proc.visF2.load()) - 4.f, b.getBottom() - 14.f, 8.f, 8.f);
    }

    void visibilityChanged() override
    {
        if (isShowing()) startTimerHz (30);
        else             stopTimer();
    }

private:
    void timerCallback() override { repaint(); }
    PhonemorphProcessor& proc;
};

//==============================================================================
class PhonemorphEditor : public juce::AudioProcessorEditor
{
public:
    explicit PhonemorphEditor (PhonemorphProcessor&);
    ~PhonemorphEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att; // last: destroyed first
    };

    PhonemorphProcessor& proc;
    AeroGlassLookAndFeel laf;
    AudioVisualScope scope;
    std::vector<std::unique_ptr<Knob>> knobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhonemorphEditor)
};
