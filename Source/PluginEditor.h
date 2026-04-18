#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "LiquidView.h"

// Minimal, refined LookAndFeel : charcoal + champagne gold accent.
class OctoPadLookAndFeel : public juce::LookAndFeel_V4
{
public:
    OctoPadLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    juce::Label* createSliderTextBox (juce::Slider&) override;
    juce::Font   getLabelFont (juce::Label&) override;
    void drawLabel (juce::Graphics&, juce::Label&) override;

    void drawComboBox (juce::Graphics&, int width, int height,
                       bool, int, int, int, int, juce::ComboBox&) override;
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu, const juce::String& text,
                            const juce::String& shortcutText, const juce::Drawable*,
                            const juce::Colour*) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;

    static const juce::Colour bg;
    static const juce::Colour panel;
    static const juce::Colour text;
    static const juce::Colour dim;
    static const juce::Colour accent;
};

class LabeledKnob : public juce::Component
{
public:
    LabeledKnob (const juce::String& caption);
    void resized() override;
    void paint (juce::Graphics& g) override;
    juce::Slider slider;
private:
    juce::String caption;
};

class OctoPadEditor : public juce::AudioProcessorEditor
{
public:
    explicit OctoPadEditor (OctoPadAudioProcessor&);
    ~OctoPadEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    OctoPadAudioProcessor& proc;
    OctoPadLookAndFeel lnf;

    // Central reactive liquid simulation
    std::unique_ptr<LiquidView> liquid;

    // Header
    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };

    struct Row { juce::String section; std::vector<std::pair<juce::String, juce::String>> knobs; };
    std::vector<Row> layout;

    std::vector<std::unique_ptr<LabeledKnob>> knobs;
    std::vector<juce::String> knobIds; // parallel to knobs
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;

    void buildUI();
    void refreshPresetSelection();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OctoPadEditor)
};
