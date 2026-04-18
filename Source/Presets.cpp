#include "Presets.h"

namespace octopad
{

// Helper to construct a PadParams inline.
static PadParams mk (float character, float detune, float sub, float noise,
                     float cutoff, float resonance, float drive,
                     float attack, float release,
                     float movement, float motionRate, float glide, float spread,
                     float chorus, float shimmer,
                     float reverbSize, float reverbMix,
                     float delayTime, float delayFb, float delayMix,
                     float tone, float gain)
{
    PadParams p;
    p.character = character; p.detune = detune; p.sub = sub; p.noise = noise;
    p.cutoff = cutoff; p.resonance = resonance; p.drive = drive;
    p.attack = attack; p.release = release;
    p.movement = movement; p.motionRate = motionRate; p.glide = glide; p.spread = spread;
    p.chorus = chorus; p.shimmer = shimmer;
    p.reverbSize = reverbSize; p.reverbMix = reverbMix;
    p.delayTime = delayTime; p.delayFb = delayFb; p.delayMix = delayMix;
    p.tone = tone; p.gain = gain;
    return p;
}

static std::vector<Preset> buildPresets()
{
    // character, detune, sub, noise, cutoff, reso, drive, attack, release,
    // movement, motionRate, glide, spread, chorus, shimmer, rvbSize, rvbMix,
    // dlyTime, dlyFb, dlyMix, tone, gain
    return {
        { "Aurora",         mk (0.85f, 0.15f, 0.25f, 0.05f, 0.55f, 0.10f, 0.05f, 0.55f, 0.75f, 0.30f, 0.15f, 0.35f, 0.55f, 0.40f, 0.25f, 0.80f, 0.40f, 0.45f, 0.25f, 0.15f, 0.65f, 0.75f) },
        { "Deep Space",     mk (0.40f, 0.35f, 0.70f, 0.10f, 0.28f, 0.15f, 0.08f, 0.85f, 0.90f, 0.55f, 0.08f, 0.50f, 0.75f, 0.30f, 0.10f, 0.95f, 0.55f, 0.60f, 0.35f, 0.20f, 0.25f, 0.70f) },
        { "Glass Tower",    mk (0.95f, 0.05f, 0.10f, 0.03f, 0.75f, 0.08f, 0.02f, 0.40f, 0.70f, 0.20f, 0.30f, 0.15f, 0.45f, 0.25f, 0.75f, 0.75f, 0.50f, 0.50f, 0.40f, 0.25f, 0.80f, 0.72f) },
        { "Warm Hug",       mk (0.20f, 0.30f, 0.45f, 0.08f, 0.50f, 0.18f, 0.25f, 0.45f, 0.70f, 0.15f, 0.18f, 0.25f, 0.55f, 0.35f, 0.08f, 0.65f, 0.30f, 0.40f, 0.20f, 0.10f, 0.55f, 0.80f) },
        { "Ice Field",      mk (0.60f, 0.20f, 0.15f, 0.18f, 0.80f, 0.20f, 0.05f, 0.60f, 0.80f, 0.25f, 0.35f, 0.20f, 0.70f, 0.55f, 0.40f, 0.80f, 0.45f, 0.50f, 0.30f, 0.25f, 0.75f, 0.70f) },
        { "Ocean Drone",    mk (0.15f, 0.45f, 0.80f, 0.12f, 0.22f, 0.25f, 0.15f, 0.75f, 0.92f, 0.60f, 0.05f, 0.40f, 0.80f, 0.35f, 0.05f, 0.90f, 0.55f, 0.70f, 0.40f, 0.20f, 0.20f, 0.72f) },
        { "Celestial",      mk (0.75f, 0.10f, 0.20f, 0.04f, 0.65f, 0.12f, 0.03f, 0.70f, 0.88f, 0.30f, 0.20f, 0.30f, 0.60f, 0.45f, 0.85f, 0.95f, 0.70f, 0.55f, 0.50f, 0.35f, 0.70f, 0.70f) },
        { "Nebula",         mk (0.50f, 0.40f, 0.30f, 0.35f, 0.45f, 0.30f, 0.20f, 0.65f, 0.80f, 0.50f, 0.22f, 0.30f, 0.70f, 0.40f, 0.45f, 0.85f, 0.55f, 0.55f, 0.45f, 0.30f, 0.50f, 0.68f) },
        { "Whisper Choir",  mk (0.90f, 0.18f, 0.15f, 0.15f, 0.58f, 0.45f, 0.08f, 0.62f, 0.78f, 0.35f, 0.12f, 0.25f, 0.65f, 0.50f, 0.35f, 0.78f, 0.45f, 0.45f, 0.25f, 0.15f, 0.60f, 0.72f) },
        { "Velvet",         mk (0.30f, 0.22f, 0.35f, 0.06f, 0.42f, 0.12f, 0.10f, 0.50f, 0.72f, 0.18f, 0.18f, 0.28f, 0.50f, 0.30f, 0.15f, 0.70f, 0.32f, 0.40f, 0.20f, 0.10f, 0.45f, 0.78f) },
        { "Frozen Lake",    mk (0.85f, 0.08f, 0.25f, 0.10f, 0.60f, 0.15f, 0.03f, 0.80f, 0.90f, 0.45f, 0.06f, 0.40f, 0.70f, 0.40f, 0.50f, 0.90f, 0.55f, 0.55f, 0.40f, 0.25f, 0.70f, 0.70f) },
        { "Phantom",        mk (0.35f, 0.32f, 0.55f, 0.20f, 0.30f, 0.55f, 0.30f, 0.70f, 0.85f, 0.70f, 0.14f, 0.35f, 0.75f, 0.40f, 0.15f, 0.88f, 0.50f, 0.55f, 0.50f, 0.30f, 0.30f, 0.70f) },
        { "Silk Road",      mk (0.25f, 0.50f, 0.30f, 0.08f, 0.55f, 0.20f, 0.15f, 0.35f, 0.75f, 0.25f, 0.22f, 0.20f, 0.75f, 0.55f, 0.20f, 0.75f, 0.40f, 0.45f, 0.30f, 0.20f, 0.55f, 0.75f) },
        { "Ghost Strings",  mk (0.10f, 0.28f, 0.20f, 0.10f, 0.62f, 0.18f, 0.10f, 0.18f, 0.65f, 0.20f, 0.28f, 0.15f, 0.60f, 0.45f, 0.15f, 0.72f, 0.38f, 0.40f, 0.20f, 0.15f, 0.60f, 0.76f) },
        { "Morning Mist",   mk (0.95f, 0.12f, 0.15f, 0.08f, 0.58f, 0.10f, 0.02f, 0.60f, 0.80f, 0.22f, 0.20f, 0.25f, 0.55f, 0.35f, 0.55f, 0.82f, 0.48f, 0.50f, 0.35f, 0.25f, 0.70f, 0.72f) },
        { "Echo Valley",    mk (0.55f, 0.20f, 0.25f, 0.05f, 0.55f, 0.15f, 0.08f, 0.40f, 0.75f, 0.15f, 0.18f, 0.20f, 0.50f, 0.30f, 0.20f, 0.70f, 0.38f, 0.62f, 0.55f, 0.55f, 0.58f, 0.72f) },
        { "Lunar",          mk (0.40f, 0.35f, 0.50f, 0.12f, 0.38f, 0.28f, 0.18f, 0.65f, 0.85f, 0.45f, 0.10f, 0.30f, 0.70f, 0.40f, 0.25f, 0.85f, 0.50f, 0.50f, 0.40f, 0.25f, 0.40f, 0.72f) },
        { "Solar Winds",    mk (0.65f, 0.30f, 0.20f, 0.18f, 0.68f, 0.25f, 0.12f, 0.45f, 0.70f, 0.55f, 0.32f, 0.18f, 0.75f, 0.50f, 0.35f, 0.78f, 0.42f, 0.48f, 0.40f, 0.30f, 0.72f, 0.72f) },
        { "Temple",         mk (0.80f, 0.15f, 0.30f, 0.05f, 0.50f, 0.15f, 0.05f, 0.55f, 0.88f, 0.20f, 0.12f, 0.30f, 0.55f, 0.30f, 0.45f, 0.95f, 0.65f, 0.45f, 0.25f, 0.15f, 0.62f, 0.70f) },
        { "Dreamweaver",    mk (0.55f, 0.38f, 0.35f, 0.15f, 0.50f, 0.22f, 0.12f, 0.70f, 0.85f, 0.50f, 0.20f, 0.35f, 0.80f, 0.60f, 0.60f, 0.90f, 0.60f, 0.55f, 0.45f, 0.35f, 0.58f, 0.70f) },
    };
}

const std::vector<Preset>& getFactoryPresets()
{
    static const std::vector<Preset> presets = buildPresets();
    return presets;
}

static void setParam (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float value)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, value));
}

void applyPresetToApvts (const Preset& p, juce::AudioProcessorValueTreeState& apvts)
{
    const auto& q = p.params;
    setParam (apvts, "character",  q.character);
    setParam (apvts, "detune",     q.detune);
    setParam (apvts, "sub",        q.sub);
    setParam (apvts, "noise",      q.noise);
    setParam (apvts, "cutoff",     q.cutoff);
    setParam (apvts, "resonance",  q.resonance);
    setParam (apvts, "drive",      q.drive);
    setParam (apvts, "attack",     q.attack);
    setParam (apvts, "release",    q.release);
    setParam (apvts, "movement",   q.movement);
    setParam (apvts, "motionRate", q.motionRate);
    setParam (apvts, "glide",      q.glide);
    setParam (apvts, "spread",     q.spread);
    setParam (apvts, "chorus",     q.chorus);
    setParam (apvts, "shimmer",    q.shimmer);
    setParam (apvts, "reverbSize", q.reverbSize);
    setParam (apvts, "reverbMix",  q.reverbMix);
    setParam (apvts, "delayTime",  q.delayTime);
    setParam (apvts, "delayFb",    q.delayFb);
    setParam (apvts, "delayMix",   q.delayMix);
    setParam (apvts, "tone",       q.tone);
    setParam (apvts, "gain",       q.gain);
}

} // namespace octopad
