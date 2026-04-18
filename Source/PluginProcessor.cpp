#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

namespace
{
    inline float getParam (const juce::AudioProcessorValueTreeState& s, const char* id)
    {
        if (auto* p = s.getRawParameterValue (id)) return p->load();
        return 0.0f;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout OctoPadAudioProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto add = [&] (const char* id, const char* name, float def)
    {
        layout.add (std::make_unique<P> (juce::ParameterID { id, 1 }, name,
                                         juce::NormalisableRange<float> (0.0f, 1.0f, 0.0001f), def));
    };

    add ("character",  "Character",  0.5f);
    add ("detune",     "Detune",     0.25f);
    add ("sub",        "Sub",        0.3f);
    add ("noise",      "Texture",    0.08f);

    add ("cutoff",     "Cutoff",     0.55f);
    add ("resonance",  "Resonance",  0.15f);
    add ("drive",      "Drive",      0.1f);

    add ("attack",     "Attack",     0.5f);
    add ("release",    "Release",    0.7f);

    add ("movement",   "Movement",   0.35f);
    add ("motionRate", "Motion Rate",0.25f);
    add ("glide",      "Glide",      0.3f);
    add ("spread",     "Spread",     0.6f);

    add ("chorus",     "Chorus",     0.35f);
    add ("shimmer",    "Shimmer",    0.2f);
    add ("reverbSize", "Reverb Size",0.7f);
    add ("reverbMix",  "Reverb Mix", 0.35f);
    add ("delayTime",  "Delay Time", 0.4f);
    add ("delayFb",    "Delay FB",   0.3f);
    add ("delayMix",   "Delay Mix",  0.2f);

    add ("tone",       "Tone",       0.5f);
    add ("gain",       "Gain",       0.8f);

    return layout;
}

OctoPadAudioProcessor::OctoPadAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "OctoPad", createLayout())
{
}

void OctoPadAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    sustainPedalDown = false;
    heldByPedal.fill (false);
}

bool OctoPadAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void OctoPadAudioProcessor::pushParamsToEngine()
{
    octopad::PadParams p;
    p.character  = getParam (apvts, "character");
    p.detune     = getParam (apvts, "detune");
    p.sub        = getParam (apvts, "sub");
    p.noise      = getParam (apvts, "noise");
    p.cutoff     = getParam (apvts, "cutoff");
    p.resonance  = getParam (apvts, "resonance");
    p.drive      = getParam (apvts, "drive");
    p.attack     = getParam (apvts, "attack");
    p.release    = getParam (apvts, "release");
    p.movement   = getParam (apvts, "movement");
    p.motionRate = getParam (apvts, "motionRate");
    p.glide      = getParam (apvts, "glide");
    p.spread     = getParam (apvts, "spread");
    p.chorus     = getParam (apvts, "chorus");
    p.shimmer    = getParam (apvts, "shimmer");
    p.reverbSize = getParam (apvts, "reverbSize");
    p.reverbMix  = getParam (apvts, "reverbMix");
    p.delayTime  = getParam (apvts, "delayTime");
    p.delayFb    = getParam (apvts, "delayFb");
    p.delayMix   = getParam (apvts, "delayMix");
    p.tone       = getParam (apvts, "tone");
    p.gain       = getParam (apvts, "gain");
    engine.setParams (p);
}

void OctoPadAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals _;
    const int totalOut = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();
    for (int ch = 0; ch < totalOut; ++ch) buffer.clear (ch, 0, numSamples);

    pushParamsToEngine();

    int lastSample = 0;
    for (const auto meta : midi)
    {
        // Clamp the event position into the buffer to avoid any out-of-range render.
        const int pos = juce::jlimit (0, numSamples, meta.samplePosition);
        if (pos > lastSample)
            engine.process (buffer, lastSample, pos - lastSample);
        lastSample = pos;

        const auto msg = meta.getMessage();
        const int note = msg.getNoteNumber();

        if (msg.isNoteOn())
        {
            if (note >= 0 && note < 128)
                heldByPedal[(size_t) note] = false;
            engine.noteOn (note, msg.getFloatVelocity());
        }
        else if (msg.isNoteOff())
        {
            if (sustainPedalDown && note >= 0 && note < 128)
                heldByPedal[(size_t) note] = true;
            else
                engine.noteOff (note);
        }
        else if (msg.isAllSoundOff())
        {
            // CC120 - silence immediately
            sustainPedalDown = false;
            heldByPedal.fill (false);
            engine.panic();
        }
        else if (msg.isAllNotesOff())
        {
            // CC123 - soft release of everything
            sustainPedalDown = false;
            heldByPedal.fill (false);
            engine.allNotesOff();
        }
        else if (msg.isSustainPedalOn())
        {
            sustainPedalDown = true;
        }
        else if (msg.isSustainPedalOff())
        {
            sustainPedalDown = false;
            for (int n = 0; n < 128; ++n)
                if (heldByPedal[(size_t) n])
                {
                    heldByPedal[(size_t) n] = false;
                    engine.noteOff (n);
                }
        }
    }
    if (lastSample < numSamples)
        engine.process (buffer, lastSample, numSamples - lastSample);

    // Publish a peak level for the UI (thread-safe, lossy IIR smoothing).
    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        peak = juce::jmax (peak, buffer.getMagnitude (ch, 0, numSamples));
    const float prev = uiAudioLevel.load (std::memory_order_relaxed);
    const float smoothed = prev * 0.78f + peak * 0.22f;
    uiAudioLevel.store (smoothed, std::memory_order_relaxed);

    // Advance a coarse LFO phase for the UI, locked to Motion Rate param.
    const float motionRate = apvts.getRawParameterValue ("motionRate")->load();
    const float lfoHz = 0.05f * std::pow (80.0f, juce::jlimit (0.0f, 1.0f, motionRate));
    const float dt = (float) numSamples / (float) getSampleRate();
    float phase = uiLfoPhase.load (std::memory_order_relaxed) + lfoHz * dt;
    phase -= std::floor (phase);
    uiLfoPhase.store (phase, std::memory_order_relaxed);

    // Mirror to mono if needed
    if (totalOut == 1 && buffer.getNumChannels() >= 2)
    {
        auto* L = buffer.getWritePointer (0);
        auto* R = buffer.getReadPointer (1);
        for (int n = 0; n < buffer.getNumSamples(); ++n) L[n] = 0.5f * (L[n] + R[n]);
    }
}

juce::AudioProcessorEditor* OctoPadAudioProcessor::createEditor()
{
    return new OctoPadEditor (*this);
}

int OctoPadAudioProcessor::getNumPrograms()
{
    return (int) octopad::getFactoryPresets().size();
}

void OctoPadAudioProcessor::setCurrentProgram (int index)
{
    const auto& presets = octopad::getFactoryPresets();
    if (index < 0 || index >= (int) presets.size()) return;
    currentProgram = index;
    octopad::applyPresetToApvts (presets[(size_t) index], apvts);
}

const juce::String OctoPadAudioProcessor::getProgramName (int index)
{
    const auto& presets = octopad::getFactoryPresets();
    if (index < 0 || index >= (int) presets.size()) return {};
    return presets[(size_t) index].name;
}

void OctoPadAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void OctoPadAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OctoPadAudioProcessor();
}
