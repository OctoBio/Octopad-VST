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

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    OctoPadAudioProcessor& proc;
    juce::OpenGLContext openGLContext;

    std::unique_ptr<juce::OpenGLShaderProgram> shader;
    juce::uint32 vao = 0;

    struct U
    {
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> ptr;
        void create (juce::OpenGLShaderProgram& s, const char* name)
        {
            ptr = std::make_unique<juce::OpenGLShaderProgram::Uniform> (s, name);
        }
    };

    U uRes, uTime, uLevel, uLfo;
    U uCharacter, uDetune, uSub, uNoise;
    U uCutoff, uReso, uDrive, uTone;
    U uAttack, uRelease, uGlide, uSpread, uMovement;
    U uChorus, uShimmer;
    U uReverbSize, uReverbMix;
    U uDelayTime, uDelayFb, uDelayMix;
    U uGain;

    double startTimeSec = 0.0;
    bool shaderReady = false;

    float readParam (const char* id) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LiquidView)
};
