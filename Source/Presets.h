#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PadEngine.h"

namespace octopad
{

struct Preset
{
    juce::String name;
    PadParams    params;
};

// The 20 factory presets, each aiming for a distinct mood but all usable as a pad.
const std::vector<Preset>& getFactoryPresets();

// Apply a preset to an APVTS (sets all 22 params).
void applyPresetToApvts (const Preset& p, juce::AudioProcessorValueTreeState& apvts);

} // namespace octopad
