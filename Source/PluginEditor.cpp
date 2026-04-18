#include "PluginEditor.h"
#include "Presets.h"

// -------- Palette --------
const juce::Colour OctoPadLookAndFeel::bg     { 0xff12141a };
const juce::Colour OctoPadLookAndFeel::panel  { 0xff191c24 };
const juce::Colour OctoPadLookAndFeel::text   { 0xfff2f0e6 };
const juce::Colour OctoPadLookAndFeel::dim    { 0xff7a7e8a };
const juce::Colour OctoPadLookAndFeel::accent { 0xffc9b87a };

OctoPadLookAndFeel::OctoPadLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, bg);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Label::textColourId, text);
    setColour (juce::ComboBox::backgroundColourId, panel);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::arrowColourId, accent);
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, text);
    setColour (juce::TextButton::buttonColourId, panel);
    setColour (juce::TextButton::buttonOnColourId, accent.withAlpha (0.3f));
    setColour (juce::TextButton::textColourOffId, text);
    setColour (juce::TextButton::textColourOnId,  text);
}

void OctoPadLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                           float pos, float startAng, float endAng,
                                           juce::Slider& s)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();

    // Track ring
    juce::Path ring;
    ring.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAng, endAng, true);
    g.setColour (dim.withAlpha (0.35f));
    g.strokePath (ring, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value arc
    const float angle = startAng + pos * (endAng - startAng);
    juce::Path value;
    value.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAng, angle, true);
    g.setColour (s.isEnabled() ? accent : dim);
    g.strokePath (value, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Inner disc
    const float innerR = radius * 0.72f;
    juce::Rectangle<float> disc (centre.x - innerR, centre.y - innerR, innerR * 2, innerR * 2);
    g.setColour (panel.brighter (0.08f));
    g.fillEllipse (disc);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.drawEllipse (disc, 1.0f);

    // Indicator line
    const float tickLen = innerR * 0.65f;
    juce::Point<float> tip { centre.x + std::cos (angle) * tickLen,
                             centre.y + std::sin (angle) * tickLen };
    g.setColour (accent);
    g.drawLine ({ centre, tip }, 2.0f);
    g.fillEllipse (centre.x - 2.0f, centre.y - 2.0f, 4.0f, 4.0f);
}

juce::Label* OctoPadLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setColour (juce::Label::textColourId, dim);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setJustificationType (juce::Justification::centred);
    l->setInterceptsMouseClicks (false, false);
    return l;
}

juce::Font OctoPadLookAndFeel::getLabelFont (juce::Label&)
{
    return juce::Font (juce::FontOptions ("Inter", 11.0f, juce::Font::plain));
}

void OctoPadLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& l)
{
    g.setColour (l.findColour (juce::Label::textColourId));
    g.setFont (getLabelFont (l));
    g.drawFittedText (l.getText(), l.getLocalBounds(), l.getJustificationType(), 1);
}

void OctoPadLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (panel);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (accent.withAlpha (0.5f));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
    // Arrow
    juce::Path arrow;
    const float ax = (float) w - 16.0f;
    const float ay = h * 0.5f;
    arrow.addTriangle (ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
    g.setColour (accent);
    g.fillPath (arrow);
    juce::ignoreUnused (box);
}

void OctoPadLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (panel);
    g.setColour (accent.withAlpha (0.4f));
    g.drawRect (0, 0, w, h, 1);
}

void OctoPadLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                            bool isSeparator, bool isActive, bool isHighlighted,
                                            bool isTicked, bool hasSubMenu, const juce::String& text_,
                                            const juce::String& shortcut, const juce::Drawable*,
                                            const juce::Colour*)
{
    juce::ignoreUnused (isTicked, hasSubMenu, shortcut);
    if (isSeparator)
    {
        g.setColour (dim.withAlpha (0.3f));
        g.drawLine ((float) area.getX() + 8, area.getCentreY(), (float) area.getRight() - 8, (float) area.getCentreY());
        return;
    }
    if (isHighlighted && isActive)
    {
        g.setColour (accent.withAlpha (0.2f));
        g.fillRect (area);
    }
    g.setColour (isActive ? text : dim);
    g.setFont (juce::Font (juce::FontOptions ("Inter", 12.5f, juce::Font::plain)));
    g.drawFittedText (text_, area.reduced (12, 0), juce::Justification::centredLeft, 1);
}

void OctoPadLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                               bool isOver, bool isDown)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (isDown ? accent.withAlpha (0.30f) : (isOver ? panel.brighter (0.1f) : panel));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (accent.withAlpha (isOver ? 0.7f : 0.45f));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
}

void OctoPadLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setColour (text);
    g.setFont (juce::Font (juce::FontOptions ("Inter", 13.0f, juce::Font::plain)));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, 1);
}

// -------- LabeledKnob --------
LabeledKnob::LabeledKnob (const juce::String& cap) : caption (cap)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 14);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                juce::MathConstants<float>::pi * 2.75f, true);
    slider.setDoubleClickReturnValue (true, slider.getValue());
    addAndMakeVisible (slider);
}

void LabeledKnob::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (16); // room for caption
    slider.setBounds (r);
}

void LabeledKnob::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().removeFromTop (16);
    g.setColour (OctoPadLookAndFeel::dim);
    g.setFont (juce::Font (juce::FontOptions ("Inter", 10.5f, juce::Font::plain)));
    g.drawFittedText (caption.toUpperCase(), r, juce::Justification::centred, 1);
}

// -------- Editor --------
OctoPadEditor::OctoPadEditor (OctoPadAudioProcessor& p)
    : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);
    liquid = std::make_unique<LiquidView> (proc);
    addAndMakeVisible (*liquid);
    buildUI();
    setResizable (false, false);
    setSize (960, 740);
}

OctoPadEditor::~OctoPadEditor()
{
    setLookAndFeel (nullptr);
}

void OctoPadEditor::buildUI()
{
    // --- Header ---
    addAndMakeVisible (presetBox);
    presetBox.setTextWhenNothingSelected ("Select preset");
    const auto& presets = octopad::getFactoryPresets();
    for (int i = 0; i < (int) presets.size(); ++i)
        presetBox.addItem (presets[(size_t) i].name, i + 1);
    presetBox.onChange = [this]()
    {
        const int idx = presetBox.getSelectedItemIndex();
        if (idx >= 0) proc.setCurrentProgram (idx);
    };

    addAndMakeVisible (prevBtn);
    addAndMakeVisible (nextBtn);
    prevBtn.onClick = [this]()
    {
        int n = (int) octopad::getFactoryPresets().size();
        int idx = (presetBox.getSelectedItemIndex() - 1 + n) % n;
        presetBox.setSelectedItemIndex (idx, juce::sendNotificationSync);
    };
    nextBtn.onClick = [this]()
    {
        int n = (int) octopad::getFactoryPresets().size();
        int idx = (presetBox.getSelectedItemIndex() + 1) % n;
        presetBox.setSelectedItemIndex (idx, juce::sendNotificationSync);
    };

    // --- Rows of knobs ---
    // Grouping by mental model, easy to scan.
    layout = {
        { "Source",  {{ "character",  "Character" },
                      { "detune",     "Detune" },
                      { "sub",        "Sub" },
                      { "noise",      "Texture" },
                      { "spread",     "Spread" }} },
        { "Filter",  {{ "cutoff",     "Cutoff" },
                      { "resonance",  "Reso" },
                      { "drive",      "Drive" },
                      { "tone",       "Tone" }} },
        { "Shape",   {{ "attack",     "Attack" },
                      { "release",    "Release" },
                      { "glide",      "Glide" },
                      { "movement",   "Movement" },
                      { "motionRate", "Motion" }} },
        { "Space",   {{ "chorus",     "Chorus" },
                      { "shimmer",    "Shimmer" },
                      { "delayTime",  "Dly Time" },
                      { "delayFb",    "Dly Fb" },
                      { "delayMix",   "Dly Mix" },
                      { "reverbSize", "Rvb Size" },
                      { "reverbMix",  "Rvb Mix" },
                      { "gain",       "Gain" }} },
    };

    for (const auto& row : layout)
        for (const auto& k : row.knobs)
        {
            auto knob = std::make_unique<LabeledKnob> (k.second);
            addAndMakeVisible (*knob);
            attachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
                                   (proc.apvts, k.first, knob->slider));
            knobs.push_back (std::move (knob));
            knobIds.push_back (k.first);
        }

    refreshPresetSelection();
}

void OctoPadEditor::refreshPresetSelection()
{
    presetBox.setSelectedItemIndex (proc.getCurrentProgram(), juce::dontSendNotification);
}

void OctoPadEditor::paint (juce::Graphics& g)
{
    g.fillAll (OctoPadLookAndFeel::bg);

    // Top banner
    auto r = getLocalBounds();
    auto header = r.removeFromTop (72);
    g.setColour (OctoPadLookAndFeel::panel.withAlpha (0.6f));
    g.fillRect (header);

    // Hairline under header
    g.setColour (OctoPadLookAndFeel::accent.withAlpha (0.35f));
    g.drawHorizontalLine (header.getBottom() - 1, (float) header.getX() + 24.0f, (float) header.getRight() - 24.0f);

    // Brand
    g.setColour (OctoPadLookAndFeel::text);
    g.setFont (juce::Font (juce::FontOptions ("Inter", 22.0f, juce::Font::plain)));
    g.drawText ("OctoPad", header.removeFromLeft (200).reduced (24, 0),
                juce::Justification::centredLeft);

    g.setColour (OctoPadLookAndFeel::dim);
    g.setFont (juce::Font (juce::FontOptions ("Inter", 10.5f, juce::Font::plain)));
    g.drawText ("ambient pad synthesizer", juce::Rectangle<int> (24, 44, 200, 16),
                juce::Justification::centredLeft);

    // Liquid viewport frame (drawn behind the GL view for soft aesthetic)
    if (liquid != nullptr)
    {
        auto lb = liquid->getBounds().toFloat().expanded (1.0f);
        g.setColour (OctoPadLookAndFeel::accent.withAlpha (0.32f));
        g.drawRoundedRectangle (lb, 8.0f, 1.0f);
    }

    // Section labels (draw above each row) - computed in resized via layout data
    auto body = getLocalBounds();
    body.removeFromTop (72);
    body.removeFromTop (220); // reserved for liquid view + margin
    body = body.reduced (24, 12);

    const int rowH = body.getHeight() / (int) layout.size();
    for (int i = 0; i < (int) layout.size(); ++i)
    {
        auto rowR = body.removeFromTop (rowH).reduced (0, 4);
        auto labelR = rowR.removeFromLeft (80);
        g.setColour (OctoPadLookAndFeel::accent);
        g.setFont (juce::Font (juce::FontOptions ("Inter", 11.0f, juce::Font::plain)));
        g.drawText (layout[(size_t) i].section.toUpperCase(), labelR,
                    juce::Justification::centredLeft);

        // Faint row underline
        g.setColour (OctoPadLookAndFeel::dim.withAlpha (0.12f));
        g.drawHorizontalLine (rowR.getBottom(), (float) labelR.getX(), (float) rowR.getRight());
    }
}

void OctoPadEditor::resized()
{
    auto r = getLocalBounds();

    // Header layout
    auto header = r.removeFromTop (72);
    header.reduce (24, 18);
    header.removeFromLeft (200); // brand space

    auto presetArea = header.removeFromRight (360);
    nextBtn.setBounds (presetArea.removeFromRight (36));
    presetArea.removeFromRight (6);
    prevBtn.setBounds (presetArea.removeFromRight (36));
    presetArea.removeFromRight (6);
    presetBox.setBounds (presetArea);

    // Central liquid view
    auto liquidArea = r.removeFromTop (220).reduced (24, 10);
    if (liquid != nullptr)
        liquid->setBounds (liquidArea);

    // Body: rows by section
    auto body = r.reduced (24, 12);
    const int rowH = body.getHeight() / (int) layout.size();

    size_t knobIdx = 0;
    for (const auto& row : layout)
    {
        auto rowR = body.removeFromTop (rowH).reduced (0, 4);
        rowR.removeFromLeft (80); // section label column
        const int n = (int) row.knobs.size();
        if (n == 0) continue;
        const int knobW = rowR.getWidth() / n;
        for (int i = 0; i < n; ++i)
        {
            auto cell = rowR.removeFromLeft (knobW).reduced (6, 4);
            if (knobIdx < knobs.size())
                knobs[knobIdx]->setBounds (cell);
            ++knobIdx;
        }
    }
}
