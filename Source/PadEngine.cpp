#include "PadEngine.h"

namespace octopad
{

// ---------- helpers ----------

static inline float fastTanh (float x)
{
    // Padé approximation - fast analog-ish soft clip.
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

static inline float mapExp (float v, float lo, float hi)
{
    // 0..1 -> lo..hi exponential
    return lo * std::pow (hi / lo, juce::jlimit (0.0f, 1.0f, v));
}

static inline float mapLin (float v, float lo, float hi)
{
    return lo + juce::jlimit (0.0f, 1.0f, v) * (hi - lo);
}

// Polyblep-ish saw (less aliasing than naive).
static inline float polyblep (float t, float dt)
{
    if (t < dt)        { t /= dt;        return t + t - t * t - 1.0f; }
    else if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}

static inline float sawBL (float& phase, float inc)
{
    phase += inc;
    if (phase >= 1.0f) phase -= 1.0f;
    float v = 2.0f * phase - 1.0f;
    v -= polyblep (phase, inc);
    return v;
}

static inline float squareBL (float& phase, float inc)
{
    phase += inc;
    if (phase >= 1.0f) phase -= 1.0f;
    float v = phase < 0.5f ? 1.0f : -1.0f;
    v += polyblep (phase, inc);
    float p2 = phase + 0.5f; if (p2 >= 1.0f) p2 -= 1.0f;
    v -= polyblep (p2, inc);
    return v;
}

static inline float sineOsc (float& phase, float inc)
{
    phase += inc;
    if (phase >= 1.0f) phase -= 1.0f;
    return std::sin (phase * juce::MathConstants<float>::twoPi);
}

// ---------- PadVoice ----------

void PadVoice::noteOn (int note, float vel)
{
    midiNote = note;
    velocity = vel;
    targetFreq = 440.0f * std::pow (2.0f, (note - 69.0f) / 12.0f);
    if (stage == Stage::Idle)
        currentFreq = targetFreq;
    stage = Stage::Attack;
    active = true;
}

void PadVoice::noteOff()
{
    if (stage != Stage::Idle)
        stage = Stage::Release;
}

void PadVoice::reset()
{
    active = false;
    phaseA = phaseB = phaseC = phaseSub = 0.0f;
    envLevel = 0.0f;
    stage = Stage::Idle;
    driftPhase = 0.0f;
    driftOffset = 0.0f;
}

// ---------- PadEngine ----------

PadEngine::PadEngine() = default;

void PadEngine::prepare (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    for (auto& v : voices) v.reset();

    // Chorus: up to ~30 ms
    int chorusLen = juce::nextPowerOfTwo ((int) (sampleRate * 0.05));
    chorusBuf.setSize (2, chorusLen, false, true, true);
    chorusBuf.clear();
    chorusWrite = 0;

    // Delay: up to 1200 ms
    int delayLen = juce::nextPowerOfTwo ((int) (sampleRate * 1.5));
    delayBuf.setSize (2, delayLen, false, true, true);
    delayBuf.clear();
    delayWrite = 0;

    // Shimmer: ~200 ms grain buffer
    int shimmerLen = juce::nextPowerOfTwo ((int) (sampleRate * 0.3));
    shimmerBuf.setSize (2, shimmerLen, false, true, true);
    shimmerBuf.clear();
    shimmerWrite = 0;
    shimmerReadA = 0.0f;
    shimmerReadB = (float) (shimmerLen / 2);

    // Reverb
    reverb.reset();
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };
    reverb.prepare (spec);

    // Limiter
    limiter.prepare (spec);
    limiter.setThreshold (-0.3f);
    limiter.setRelease (80.0f);

    filterL = {}; filterR = {};
    tiltLpL = tiltLpR = tiltHpL = tiltHpR = tiltPrevL = tiltPrevR = 0.0f;
    lfoPhase = 0.0f;
    chorusPhase = 0.0f;
}

void PadEngine::reset()
{
    for (auto& v : voices) v.reset();
    chorusBuf.clear();
    delayBuf.clear();
    shimmerBuf.clear();
    reverb.reset();
    limiter.reset();
    filterL = {}; filterR = {};
    tiltLpL = tiltLpR = tiltHpL = tiltHpR = tiltPrevL = tiltPrevR = 0.0f;
}

PadVoice* PadEngine::findFreeVoice (int note)
{
    // 1) Re-trigger if same note is already sounding (merge, no stuck voice).
    for (auto& v : voices)
        if (v.active && v.midiNote == note)
            return &v;

    // 2) Prefer a completely idle voice.
    for (auto& v : voices)
        if (v.stage == PadVoice::Stage::Idle)
            return &v;

    // 3) Steal the quietest (lowest envLevel) voice.
    PadVoice* steal = &voices[0];
    for (auto& v : voices)
        if (v.envLevel < steal->envLevel)
            steal = &v;
    steal->reset(); // hard reset so phases don't carry over into the new pitch
    return steal;
}

void PadEngine::noteOn (int midiNote, float velocity)
{
    if (auto* v = findFreeVoice (midiNote))
        v->noteOn (midiNote, velocity);
}

void PadEngine::noteOff (int midiNote)
{
    // Release every voice matching this note (defensive: more than one could exist
    // in edge cases due to voice stealing).
    for (auto& v : voices)
        if (v.active && v.midiNote == midiNote
            && v.stage != PadVoice::Stage::Release
            && v.stage != PadVoice::Stage::Idle)
            v.noteOff();
}

void PadEngine::allNotesOff()
{
    // Soft all-notes-off: send every active voice to Release.
    for (auto& v : voices)
        if (v.active && v.stage != PadVoice::Stage::Idle)
            v.noteOff();
}

void PadEngine::panic()
{
    // Hard kill: silence everything immediately, clear FX buffers.
    for (auto& v : voices) v.reset();
    chorusBuf.clear();
    delayBuf.clear();
    shimmerBuf.clear();
    reverb.reset();
    limiter.reset();
    filterL = {}; filterR = {};
    tiltLpL = tiltLpR = tiltHpL = tiltHpR = tiltPrevL = tiltPrevR = 0.0f;
}

void PadEngine::renderVoice (PadVoice& v, float* left, float* right, int numSamples, float lfoValue)
{
    if (! v.active && v.envLevel < 1.0e-5f) return;

    const float detuneCents = mapLin (params.detune, 0.0f, 40.0f);          // +/- semitone-ish
    const float detA = std::pow (2.0f, ( detuneCents * 0.5f) / 1200.0f);
    const float detB = std::pow (2.0f, (-detuneCents * 0.5f) / 1200.0f);
    const float detC = 1.0f;

    const float shape = juce::jlimit (0.0f, 1.0f, params.character);
    // 0 = saw, 0.5 = square+saw, 1 = sine
    const float sawAmt  = juce::jmax (0.0f, 1.0f - 2.0f * shape);
    const float sqAmt   = 1.0f - std::abs (2.0f * shape - 1.0f);
    const float sineAmt = juce::jmax (0.0f, 2.0f * shape - 1.0f);

    const float subLvl   = params.sub;
    const float noiseLvl = params.noise;

    // ADSR params
    const float attackSec  = mapExp (params.attack,  0.01f, 4.0f);
    const float releaseSec = mapExp (params.release, 0.1f,  8.0f);
    const float attInc  = 1.0f / (float) (attackSec  * sr);
    const float relDec  = 1.0f / (float) (releaseSec * sr);

    // Portamento coefficient
    const float glideSec = mapExp (juce::jmax (0.001f, params.glide), 0.001f, 1.5f);
    const float glideCoeff = std::exp (-1.0f / (float) (glideSec * sr));

    // Drift: subtle 0.2..0.6 Hz random walk per voice
    const float driftInc = 0.4f / (float) sr;

    // Voice pan from spread (deterministic per midi note for stability)
    const float panOffset = ((v.midiNote % 7) - 3) / 3.0f;
    const float pan = juce::jlimit (-1.0f, 1.0f, panOffset * params.spread);
    const float panL = std::sqrt (0.5f * (1.0f - pan));
    const float panR = std::sqrt (0.5f * (1.0f + pan));

    for (int n = 0; n < numSamples; ++n)
    {
        // Portamento toward target freq
        v.currentFreq = v.targetFreq + (v.currentFreq - v.targetFreq) * glideCoeff;

        // Analog drift (tiny pitch wobble)
        v.driftPhase += driftInc;
        if (v.driftPhase >= 1.0f) v.driftPhase -= 1.0f;
        v.driftOffset = std::sin (v.driftPhase * juce::MathConstants<float>::twoPi) * 0.003f;

        const float pitchMod = lfoValue * params.movement * 0.004f; // subtle LFO vibrato
        const float freq = v.currentFreq * (1.0f + v.driftOffset + pitchMod);

        const float incA = (freq * detA) / (float) sr;
        const float incB = (freq * detB) / (float) sr;
        const float incC = (freq * detC) / (float) sr;
        const float incSub = (freq * 0.5f) / (float) sr;

        auto osc = [&](float& ph, float inc)
        {
            float s = 0.0f;
            if (sawAmt  > 0.0f) s += sawAmt    * sawBL    (ph, inc);
            if (sqAmt   > 0.0f) s += sqAmt * 0.6f * squareBL (ph, inc);
            if (sineAmt > 0.0f) s += sineAmt   * sineOsc  (ph, inc);
            return s;
        };

        float sA = osc (v.phaseA, incA);
        float sB = osc (v.phaseB, incB);
        float sC = osc (v.phaseC, incC);
        float sSub = std::sin (v.phaseSub * juce::MathConstants<float>::twoPi) * subLvl;
        v.phaseSub += incSub; if (v.phaseSub >= 1.0f) v.phaseSub -= 1.0f;

        float voiceSum = 0.33f * (sA + sB + sC) + sSub;

        if (noiseLvl > 0.0f)
        {
            float nz = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f);
            voiceSum += nz * noiseLvl * 0.25f;
        }

        // --- ADSR (simplified AR with sustain at 1.0) ---
        switch (v.stage)
        {
            case PadVoice::Stage::Attack:
                v.envLevel += attInc;
                if (v.envLevel >= 1.0f) { v.envLevel = 1.0f; v.stage = PadVoice::Stage::Sustain; }
                break;
            case PadVoice::Stage::Sustain:
                v.envLevel = 1.0f;
                break;
            case PadVoice::Stage::Release:
                v.envLevel -= relDec;
                if (v.envLevel <= 0.0f)
                {
                    v.envLevel = 0.0f;
                    v.stage = PadVoice::Stage::Idle;
                    v.active = false;
                }
                break;
            case PadVoice::Stage::Idle:
                v.envLevel = 0.0f;
                break;
        }

        float out = voiceSum * v.envLevel * v.velocity * 0.25f;
        left[n]  += out * panL;
        right[n] += out * panR;
    }
}

void PadEngine::applyFilter (float* left, float* right, int numSamples, float cutoffHz, float q, float lfoValue)
{
    // LFO modulation on cutoff
    const float lfoMod = 1.0f + lfoValue * params.movement * 0.8f;
    cutoffHz = juce::jlimit (30.0f, (float) sr * 0.45f, cutoffHz * lfoMod);

    const float g = std::tan (juce::MathConstants<float>::pi * cutoffHz / (float) sr);
    const float k = 1.0f / juce::jlimit (0.15f, 8.0f, q);
    const float a1 = 1.0f / (1.0f + g * (g + k));
    const float a2 = g * a1;

    const float drv = 1.0f + params.drive * 4.0f;
    const float invDrv = 1.0f / drv;

    auto tick = [&] (float x, SVF& s)
    {
        x = fastTanh (x * drv) * invDrv;
        float v3 = x - s.ic2eq;
        float v1 = a1 * s.ic1eq + a2 * v3;
        float v2 = s.ic2eq + g * v1;
        s.ic1eq = 2.0f * v1 - s.ic1eq;
        s.ic2eq = 2.0f * v2 - s.ic2eq;
        return v2; // lowpass
    };

    for (int n = 0; n < numSamples; ++n)
    {
        left[n]  = tick (left[n],  filterL);
        right[n] = tick (right[n], filterR);
    }
}

void PadEngine::applyChorus (float* left, float* right, int numSamples)
{
    const float mix = juce::jlimit (0.0f, 1.0f, params.chorus);
    if (mix < 0.001f) return;

    auto* bufL = chorusBuf.getWritePointer (0);
    auto* bufR = chorusBuf.getWritePointer (1);
    const int size = chorusBuf.getNumSamples();
    const int mask = size - 1;

    const float rate = 0.5f;           // Hz
    const float phaseInc = rate / (float) sr;
    const float depthMs = 7.0f;
    const float baseMs  = 12.0f;

    for (int n = 0; n < numSamples; ++n)
    {
        chorusPhase += phaseInc; if (chorusPhase >= 1.0f) chorusPhase -= 1.0f;
        float lfoA = std::sin (chorusPhase * juce::MathConstants<float>::twoPi);
        float lfoB = std::sin ((chorusPhase + 0.33f) * juce::MathConstants<float>::twoPi);

        bufL[chorusWrite] = left[n];
        bufR[chorusWrite] = right[n];

        auto readDelay = [&] (const float* buf, float ms)
        {
            float d = ms * 0.001f * (float) sr;
            float r = (float) chorusWrite - d;
            while (r < 0.0f) r += size;
            int i0 = (int) r & mask;
            int i1 = (i0 + 1) & mask;
            float frac = r - std::floor (r);
            return buf[i0] + frac * (buf[i1] - buf[i0]);
        };

        float dLms = baseMs + depthMs * lfoA;
        float dRms = baseMs + depthMs * lfoB;
        float wetL = readDelay (bufL, dLms);
        float wetR = readDelay (bufR, dRms);

        left[n]  = left[n]  * (1.0f - 0.5f * mix) + wetR * 0.5f * mix;
        right[n] = right[n] * (1.0f - 0.5f * mix) + wetL * 0.5f * mix;

        chorusWrite = (chorusWrite + 1) & mask;
    }
}

void PadEngine::applyDelay (float* left, float* right, int numSamples)
{
    const float mix = juce::jlimit (0.0f, 1.0f, params.delayMix);
    if (mix < 0.001f) return;

    auto* bufL = delayBuf.getWritePointer (0);
    auto* bufR = delayBuf.getWritePointer (1);
    const int size = delayBuf.getNumSamples();
    const int mask = size - 1;

    const float timeMs = mapExp (params.delayTime, 40.0f, 900.0f);
    const int delaySamples = juce::jlimit (1, size - 2, (int) (timeMs * 0.001f * sr));
    const float fb = juce::jlimit (0.0f, 0.92f, params.delayFb * 0.92f);

    for (int n = 0; n < numSamples; ++n)
    {
        int readIdx = (delayWrite - delaySamples + size) & mask;
        float dL = bufL[readIdx];
        float dR = bufR[readIdx];

        // Ping-pong cross-feed
        bufL[delayWrite] = left[n]  + dR * fb;
        bufR[delayWrite] = right[n] + dL * fb;

        left[n]  += dL * mix;
        right[n] += dR * mix;

        delayWrite = (delayWrite + 1) & mask;
    }
}

void PadEngine::applyShimmer (float* left, float* right, int numSamples)
{
    const float amt = juce::jlimit (0.0f, 1.0f, params.shimmer);
    if (amt < 0.001f) return;

    auto* bufL = shimmerBuf.getWritePointer (0);
    auto* bufR = shimmerBuf.getWritePointer (1);
    const int size = shimmerBuf.getNumSamples();
    const int mask = size - 1;
    const float grainLen = (float) size * 0.5f;

    for (int n = 0; n < numSamples; ++n)
    {
        bufL[shimmerWrite] = left[n];
        bufR[shimmerWrite] = right[n];

        auto readInterp = [&](const float* buf, float pos)
        {
            while (pos < 0.0f) pos += size;
            while (pos >= size) pos -= size;
            int i0 = (int) pos & mask;
            int i1 = (i0 + 1) & mask;
            float f = pos - std::floor (pos);
            return buf[i0] + f * (buf[i1] - buf[i0]);
        };

        // Two overlapping grains read at 2x speed (octave up), crossfaded
        shimmerReadA += 2.0f;
        shimmerReadB += 2.0f;
        if (shimmerReadA >= size) shimmerReadA -= size;
        if (shimmerReadB >= size) shimmerReadB -= size;

        float distA = shimmerWrite - shimmerReadA; while (distA < 0) distA += size;
        float distB = shimmerWrite - shimmerReadB; while (distB < 0) distB += size;
        float wA = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (distA / grainLen));
        float wB = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (distB / grainLen));
        wA = juce::jlimit (0.0f, 1.0f, wA);
        wB = juce::jlimit (0.0f, 1.0f, wB);

        float sL = readInterp (bufL, shimmerReadA) * wA + readInterp (bufL, shimmerReadB) * wB;
        float sR = readInterp (bufR, shimmerReadA) * wA + readInterp (bufR, shimmerReadB) * wB;

        left[n]  += sL * amt * 0.5f;
        right[n] += sR * amt * 0.5f;

        shimmerWrite = (shimmerWrite + 1) & mask;
    }
}

void PadEngine::applyTilt (float* left, float* right, int numSamples)
{
    // Tilt EQ: one-pole smoothing at ~1kHz split, cross-fade dark/bright
    const float cutoff = 900.0f;
    const float coef = std::exp (-juce::MathConstants<float>::twoPi * cutoff / (float) sr);
    const float bright = juce::jlimit (0.0f, 1.0f, params.tone);
    const float darkGain  = juce::jmap (bright, 1.6f, 0.5f);
    const float brightGain = juce::jmap (bright, 0.5f, 1.6f);

    for (int n = 0; n < numSamples; ++n)
    {
        tiltLpL = (1.0f - coef) * left[n]  + coef * tiltLpL;
        tiltLpR = (1.0f - coef) * right[n] + coef * tiltLpR;
        float hpL = left[n]  - tiltLpL;
        float hpR = right[n] - tiltLpR;
        left[n]  = tiltLpL * darkGain + hpL * brightGain;
        right[n] = tiltLpR * darkGain + hpR * brightGain;
    }
}

void PadEngine::process (juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (numSamples <= 0) return;
    if (buffer.getNumChannels() < 2) return;

    float* left  = buffer.getWritePointer (0) + startSample;
    float* right = buffer.getWritePointer (1) + startSample;

    // LFO (sine)
    const float lfoHz = mapExp (params.motionRate, 0.05f, 4.0f);
    const float lfoInc = lfoHz / (float) sr;

    // Generate LFO buffer for voice+filter
    std::vector<float> lfoVals ((size_t) numSamples);
    for (int n = 0; n < numSamples; ++n)
    {
        lfoPhase += lfoInc;
        if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
        lfoVals[(size_t) n] = std::sin (lfoPhase * juce::MathConstants<float>::twoPi);
    }

    // Render voices additively (single LFO value average is fine since movement is subtle)
    float lfoAvg = 0.0f;
    for (auto v : lfoVals) lfoAvg += v;
    lfoAvg /= (float) numSamples;

    // Clear output region
    juce::FloatVectorOperations::clear (left,  numSamples);
    juce::FloatVectorOperations::clear (right, numSamples);

    for (auto& v : voices)
        renderVoice (v, left, right, numSamples, lfoAvg);

    // Filter with LFO modulation
    const float cutoffHz = mapExp (params.cutoff, 60.0f, 12000.0f);
    const float q = mapLin (params.resonance, 0.3f, 6.0f);
    applyFilter (left, right, numSamples, cutoffHz, q, lfoAvg);

    // Chorus for stereo width
    applyChorus (left, right, numSamples);

    // Shimmer feeds into reverb (add octave-up content)
    applyShimmer (left, right, numSamples);

    // Delay (ping-pong)
    applyDelay (left, right, numSamples);

    // Reverb
    juce::dsp::Reverb::Parameters rp;
    rp.roomSize = juce::jlimit (0.0f, 1.0f, params.reverbSize);
    rp.damping  = 0.35f;
    rp.width    = 1.0f;
    rp.wetLevel = juce::jlimit (0.0f, 1.0f, params.reverbMix);
    rp.dryLevel = 1.0f - 0.5f * rp.wetLevel;
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp);

    juce::dsp::AudioBlock<float> block (buffer);
    auto sub = block.getSubBlock ((size_t) startSample, (size_t) numSamples);
    juce::dsp::ProcessContextReplacing<float> ctx (sub);
    reverb.process (ctx);

    // Tilt EQ (tone)
    applyTilt (left, right, numSamples);

    // Output gain
    const float g = params.gain;
    juce::FloatVectorOperations::multiply (left,  g, numSamples);
    juce::FloatVectorOperations::multiply (right, g, numSamples);

    // Limiter (transparent safety)
    limiter.process (ctx);
}

} // namespace octopad
