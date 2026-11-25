#pragma once
#include <juce_core/juce_core.h>
#include <cmath>
#include <cstdint>
#include <vector>

namespace Phonemorph
{
//==============================================================================
// Zavalishin TPT state-variable filter. Coefficients set at control rate only.
struct SVF
{
    float g = 0.f, k = 1.f, a1 = 0.f, a2 = 0.f, a3 = 0.f, ic1 = 0.f, ic2 = 0.f;

    void setup (float fc, float Q, float fs) noexcept
    {
        fc = juce::jlimit (20.f, fs * 0.45f, fc);
        g  = std::tan (juce::MathConstants<float>::pi * fc / fs);
        k  = 1.f / Q;
        a1 = 1.f / (1.f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    void reset() noexcept { ic1 = ic2 = 0.f; }

    void process (float x, float& lp, float& bp, float& hp) noexcept
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.f * v1 - ic1 + 1.0e-20f;   // denormal guard
        ic2 = 2.f * v2 - ic2 + 1.0e-20f;
        lp = v2;
        bp = v1;
        hp = x - k * v1 - v2;
    }
};

//==============================================================================
// One-pole asymmetric envelope follower.
struct Env
{
    float att = 0.f, rel = 0.f, y = 0.f;

    void setup (float attMs, float relMs, float fs) noexcept
    {
        att = std::exp (-1.f / (attMs * 0.001f * fs));
        rel = std::exp (-1.f / (relMs * 0.001f * fs));
    }

    float process (float x) noexcept
    {
        const float c = x > y ? att : rel;
        y = c * y + (1.f - c) * x;
        return y;
    }
};

//==============================================================================
// Master chain: transient splitter, formant oral resonator, vacuum (M/S),
// breath, phantom-fundamental bass, midrange knock, soft-clip guard.
class MorphChain
{
public:
    struct Params
    {
        float morph = 0.45f, transientGuard = 0.75f, vacuum = 0.5f, throat = 0.55f,
              breath = 0.25f, phantomLow = 0.6f, tinySpeaker = 0.5f, outGain = 1.f;
    };

    void prepare (double sampleRate)
    {
        fs      = (float) sampleRate;
        lookLen = juce::jmax (1, (int) std::ceil (0.012f * fs));   // 12 ms lookahead
        sideLen = juce::jmax (1, (int) std::round (0.0004f * fs)); // 0.4 ms vacuum micro-delay
        bufLen  = lookLen + 4;

        dL.assign ((size_t) bufLen, 0.f);
        dR.assign ((size_t) bufLen, 0.f);
        sideBuf.assign ((size_t) bufLen, 0.f);
        w = 0;

        fastEnv.setup (0.5f, 40.f, fs);
        slowEnv.setup (10.f, 150.f, fs);
        rmsEnv.setup (5.f, 200.f, fs);

        bp1.reset(); bp2.reset(); subLP.reset();
        phBP.reset(); knockBP.reset(); breathBP.reset();
        updateControl();
    }

    int latencySamples() const noexcept { return lookLen; }
    void setTargets (const Params& p) noexcept { target = p; }

    float visF1Hz()  const noexcept { return visF1; }
    float visF2Hz()  const noexcept { return visF2; }
    float visVacuum() const noexcept { return visDuck; }

    void processSample (float& L, float& R) noexcept
    {
        if (++ctr >= 32) { ctr = 0; updateControl(); }

        // Write undelayed input; detection runs on it (this is the lookahead).
        dL[(size_t) w] = L;
        dR[(size_t) w] = R;

        const float inMid = 0.5f * (L + R);
        const float lvl   = std::abs (inMid);

        // Transient splitter
        const float fE    = fastEnv.process (lvl);
        const float sE    = slowEnv.process (lvl);
        const float trans = juce::jlimit (0.f, 1.f, (fE - sE) / (fE + 1.0e-4f) * 2.f);
        const float env   = rmsEnv.process (lvl);

        // Zero-crossing frequency estimator (heuristic formant tracker)
        if ((inMid < 0.f) != (prevMid < 0.f)) ++zc;
        prevMid = inMid;
        if (++zcN >= 512)
        {
            const float est = (float) zc * fs / 1024.f;
            trackedHz += (juce::jlimit (40.f, 8000.f, est) - trackedHz) * 0.3f;
            zc = 0; zcN = 0;
        }

        // Read lookahead-delayed audio
        const int r  = (w - lookLen + bufLen) % bufLen;
        const float dl = dL[(size_t) r], dr = dR[(size_t) r];
        const float dMid  = 0.5f * (dl + dr);
        const float dSide = 0.5f * (dl - dr);

        sideBuf[(size_t) w] = dSide;
        const int sr = (w - sideLen + bufLen) % bufLen;
        const float sideMicro = sideBuf[(size_t) sr];

        float lp, bp, hp;

        // Oral resonator on sustain layer (transients bypass via susGain)
        bp1.process (dMid, lp, bp, hp); const float f1 = bp;
        bp2.process (dMid, lp, bp, hp); const float f2 = bp;
        const float susGain = 1.f - cur.transientGuard * trans;
        float mid = dMid + cur.throat * cur.morph * susGain * (f1 * 0.9f + f2 * 0.6f) * 1.5f;

        // Breath: noise through formant-coupled bandpass, gated by micro-gaps
        const float gap = 1.f - juce::jlimit (0.f, 1.f, env * 8.f);
        breathBP.process (nextNoise(), lp, bp, hp);
        mid += cur.breath * gap * bp * 0.6f;

        // Phantom fundamental: sub -> rectify -> 140-650 Hz band
        subLP.process (dMid, lp, bp, hp);
        const float rect = std::abs (fastTanh (lp * 4.f));
        phBP.process (rect, lp, bp, hp);
        mid += cur.phantomLow * bp * 1.2f;

        // Midrange knock around 1.2 kHz
        knockBP.process (dMid, lp, bp, hp);
        mid += cur.tinySpeaker * (0.5f + trans) * bp * 0.8f;

        // Vacuum: duck side on transients, inverse-phase micro-delay on snap
        const float duck = cur.vacuum * trans;
        const float side = dSide * (1.f - duck) - duck * 0.5f * sideMicro;
        visDuck = duck;

        float outL = (mid + side) * cur.outGain;
        float outR = (mid - side) * cur.outGain;
        L = softClip (outL);
        R = softClip (outR);

        w = (w + 1) % bufLen;
    }

private:
    static float fastTanh (float x) noexcept
    {
        const float x2 = x * x;
        return juce::jlimit (-1.f, 1.f, x * (27.f + x2) / (27.f + 9.f * x2));
    }

    static float softClip (float x) noexcept
    {
        constexpr float ceiling = 0.966f; // -0.3 dBFS
        return ceiling * x / (ceiling + std::abs (x));
    }

    float nextNoise() noexcept
    {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return static_cast<float> (static_cast<int32_t> (rng)) * (1.f / 2147483648.f);
    }

    static void glide (float& c, float t) noexcept { c += (t - c) * 0.25f; }

    void updateControl() noexcept
    {
        glide (cur.morph, target.morph);
        glide (cur.transientGuard, target.transientGuard);
        glide (cur.vacuum, target.vacuum);
        glide (cur.throat, target.throat);
        glide (cur.breath, target.breath);
        glide (cur.phantomLow, target.phantomLow);
        glide (cur.tinySpeaker, target.tinySpeaker);
        glide (cur.outGain, target.outGain);

        visF1 = juce::jlimit (300.f, 900.f, trackedHz);
        visF2 = juce::jlimit (900.f, 2600.f, trackedHz * 2.5f);

        bp1.setup (visF1, 6.f, fs);
        bp2.setup (visF2, 5.f, fs);
        subLP.setup (110.f, 0.7f, fs);
        phBP.setup (300.f, 1.2f, fs);
        knockBP.setup (1200.f, 1.4f, fs);
        breathBP.setup (visF1, 2.f, fs);
    }

    float fs = 44100.f;
    int lookLen = 0, sideLen = 0, bufLen = 0, w = 0, ctr = 0;
    std::vector<float> dL, dR, sideBuf;

    Params cur {}, target {};
    Env fastEnv, slowEnv, rmsEnv;
    SVF bp1, bp2, subLP, phBP, knockBP, breathBP;

    float prevMid = 0.f, trackedHz = 500.f;
    int zc = 0, zcN = 0;
    uint32_t rng = 0x2468ACE1u;

    float visF1 = 500.f, visF2 = 1500.f, visDuck = 0.f;
};
} // namespace Phonemorph
