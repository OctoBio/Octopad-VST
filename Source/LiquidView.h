#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_opengl/juce_opengl.h>
#include "PluginProcessor.h"

// Reactive fluid metaball visualisation rendered with a GLSL fragment shader.
// Reads audio RMS + APVTS params from the processor and draws a morphing
// ink-blob at a smooth 60 fps regardless of UI thread load.
class LiquidView : public juce::Component, private juce::OpenGLRenderer
{
public:
    explicit LiquidView (OctoPadAudioProcessor& p);
    ~LiquidView() override;

    void paint (juce::Graphics& g) override;   // fallback when GL not ready
    void resized() override;

private:
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    OctoPadAudioProcessor& proc;
    juce::OpenGLContext openGLContext;

    std::unique_ptr<juce::OpenGLShaderProgram> shader;
    juce::uint32 vao = 0;

    // Cached uniform locations
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uRes, uTime, uLevel, uLfo,
        uCutoff, uReso, uShimmer, uDrive, uCharacter, uMovement, uDetune;

    double startTimeSec = 0.0;
    bool shaderReady = false;
    float fallbackPhase = 0.0f;

    float readParam (const char* id) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LiquidView)
};
