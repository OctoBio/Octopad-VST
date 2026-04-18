#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

namespace octopad
{

// Live parameter snapshot, pushed each block by the processor.
struct PadParams
{
    // Tone sources
    float character = 0.5f;   // 0..1 : saw <-> square <-> sine blend (osc shape mix)
    float detune    = 0.25f;  // 0..1 : supersaw-style spread in semitones
    float sub       = 0.3f;   // 0..1 : sub osc level (one octave below)
    float noise     = 0.08f;  // 0..1 : air/texture noise

    // Filter
    float cutoff    = 0.55f;  // 0..1 -> 60..12000 Hz (log)
    float resonance = 0.15f;  // 0..1 -> 0.2..6.0 Q
    float drive     = 0.1f;   // 0..1 : pre-filter softclip

    // Envelope (pad-friendly ranges)
    float attack    = 0.5f;   // 0..1 -> 0.01..4.0 s
    float release   = 0.7f;   // 0..1 -> 0.1..8.0 s

    // Motion
    float movement  = 0.35f;  // LFO depth -> cutoff/pitch
    float motionRate= 0.25f;  // 0..1 -> 0.05..4 Hz
    float glide     = 0.3f;   // 0..1 -> 0..1.5 s portamento
    float spread    = 0.6f;   // stereo width of the voices

    // FX bus
    float chorus    = 0.35f;
    float shimmer   = 0.2f;   // octave-up shimmer in reverb
    float reverbSize= 0.7f;
    float reverbMix = 0.35f;
    float delayTime = 0.4f;   // 0..1 -> 40..900 ms
    float delayFb   = 0.3f;
    float delayMix  = 0.2f;

    // Master
    float tone      = 0.5f;   // tilt EQ (0..1 : dark..bright)
    float gain      = 0.8f;
};

// Single pad voice: 3 detuned oscs + sub + noise, per-voice amp envelope.
struct PadVoice
{
    bool  active = false;
    int   midiNote = 60;
    float velocity = 0.8f;

    // Phase accumulators
    float phaseA = 0.0f, phaseB = 0.0f, phaseC = 0.0f, phaseSub = 0.0f;

    // Portamento
    float currentFreq = 440.0f;
    float targetFreq  = 440.0f;

    // Amp ADSR (pad: long A, long R)
    float envLevel = 0.0f;
    enum class Stage { Idle, Attack, Sustain, Release } stage = Stage::Idle;

    // Subtle per-voice analog drift
    float driftPhase = 0.0f;
    float driftOffset = 0.0f;

    void noteOn  (int note, float vel);
    void noteOff ();
    void reset   ();
};

class PadEngine
{
public:
    PadEngine();

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    void setParams (const PadParams& p) { params = p; }

    void noteOn  (int midiNote, float velocity);
    void noteOff (int midiNote);
    void allNotesOff();   // soft: send every voice to Release
    void panic();         // hard: silence everything immediately

    // Renders `numSamples` of stereo output into buffer (additive - call clear first).
    void process (juce::AudioBuffer<float>& buffer, int startSample, int numSamples);

private:
    static constexpr int kNumVoices = 12;
    std::array<PadVoice, kNumVoices> voices;

    double sr = 44100.0;
    PadParams params;

    // LFO
    float lfoPhase = 0.0f;

    // Per-channel filter state (stereo SVF lowpass)
    struct SVF { float ic1eq = 0.0f, ic2eq = 0.0f; };
    SVF filterL, filterR;

    // Chorus (2 LFO-modulated delay lines per side)
    juce::AudioBuffer<float> chorusBuf;
    int chorusWrite = 0;
    float chorusPhase = 0.0f;

    // Ping-pong delay
    juce::AudioBuffer<float> delayBuf;
    int delayWrite = 0;

    // Shimmer pitch-shifter (simple octave-up via granular overlap)
    juce::AudioBuffer<float> shimmerBuf;
    int shimmerWrite = 0;
    float shimmerReadA = 0.0f, shimmerReadB = 0.0f;

    // Reverb (JUCE built-in - sober, clean)
    juce::dsp::Reverb reverb;

    // Output limiter
    juce::dsp::Limiter<float> limiter;

    // Tilt EQ state (one-pole LP + one-pole HP mixed)
    float tiltLpL = 0.0f, tiltLpR = 0.0f;
    float tiltHpL = 0.0f, tiltHpR = 0.0f;
    float tiltPrevL = 0.0f, tiltPrevR = 0.0f;

    // Helpers
    PadVoice* findFreeVoice (int note);
    static float midiToHz (float note) { return 440.0f * std::pow (2.0f, (note - 69.0f) / 12.0f); }
    void renderVoice (PadVoice& v, float* left, float* right, int numSamples, float lfoValue);
    void applyFilter (float* left, float* right, int numSamples, float cutoffHz, float q, float lfoValue);
    void applyChorus (float* left, float* right, int numSamples);
    void applyDelay  (float* left, float* right, int numSamples);
    void applyShimmer(float* left, float* right, int numSamples);
    void applyTilt   (float* left, float* right, int numSamples);
};

} // namespace octopad
