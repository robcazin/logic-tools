#include "PluginEditor.h"

RubatoEditor::RubatoEditor(RubatoProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(800, 930);
    
    displayBandVelocities.fill(0);
    
    preButton.setButtonText("PRE");
    preButton.setClickingTogglesState(true);
    preButton.setToggleState(true, juce::dontSendNotification);
    preButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff4a90e2));
    preButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    preButton.onClick = [this]() {
        if (!preButton.getToggleState()) {
            preButton.setToggleState(true, juce::dontSendNotification);
        } else {
            showInput = true;
            postButton.setToggleState(false, juce::dontSendNotification);
            repaint();
        }
    };
    addAndMakeVisible(preButton);
    
    postButton.setButtonText("POST");
    postButton.setClickingTogglesState(true);
    postButton.setToggleState(false, juce::dontSendNotification);
    postButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff4a90e2));
    postButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    postButton.onClick = [this]() {
        if (!postButton.getToggleState()) {
            postButton.setToggleState(true, juce::dontSendNotification);
        } else {
            showInput = false;
            preButton.setToggleState(false, juce::dontSendNotification);
            repaint();
        }
    };
    addAndMakeVisible(postButton);
    
    amountLabel.setText("Amount (ms)", juce::dontSendNotification);
    amountLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(amountLabel);
    amountSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    amountSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(amountSlider);
    amountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getApvts(), "amountMs", amountSlider);
    
    shapeLabel.setText("Shape", juce::dontSendNotification);
    shapeLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(shapeLabel);
    shapeBox.addItem("Symmetric bell", 1);
    shapeBox.addItem("Early drag", 2);
    shapeBox.addItem("Late drag", 3);
    addAndMakeVisible(shapeBox);
    shapeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.getApvts(), "shape", shapeBox);
    
    phraseLengthLabel.setText("Phrase Length", juce::dontSendNotification);
    phraseLengthLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(phraseLengthLabel);
    phraseLengthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    phraseLengthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(phraseLengthSlider);
    phraseLengthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getApvts(), "phraseLength", phraseLengthSlider);
    
    triggerModeLabel.setText("Trigger Mode", juce::dontSendNotification);
    triggerModeLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(triggerModeLabel);
    triggerModeBox.addItem("Cycle Region", 1);
    triggerModeBox.addItem("Fixed Bar Length", 2);
    addAndMakeVisible(triggerModeBox);
    triggerModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.getApvts(), "triggerMode", triggerModeBox);
    
    reAnchorButton.setButtonText("Re-Anchor");
    addAndMakeVisible(reAnchorButton);
    reAnchorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.getApvts(), "reAnchor", reAnchorButton);
    
    chordSpreadLabel.setText("Chord Spread", juce::dontSendNotification);
    chordSpreadLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(chordSpreadLabel);
    chordSpreadSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    chordSpreadSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(chordSpreadSlider);
    chordSpreadAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getApvts(), "chordSpreadMs", chordSpreadSlider);
    
    velocityBoostLabel.setText("Boost", juce::dontSendNotification);
    velocityBoostLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(velocityBoostLabel);
    velocityBoostSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    velocityBoostSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(velocityBoostSlider);
    velocityBoostAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getApvts(), "velocityBoost", velocityBoostSlider);
    
    velocityReplaceLabel.setText("Replace", juce::dontSendNotification);
    velocityReplaceLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(velocityReplaceLabel);
    velocityReplaceSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    velocityReplaceSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(velocityReplaceSlider);
    velocityReplaceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getApvts(), "velocityReplace", velocityReplaceSlider);
    
    velocityShapeLabel.setText("Velocity Shape", juce::dontSendNotification);
    velocityShapeLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(velocityShapeLabel);
    velocityShapeBox.addItem("Symmetric bell", 1);
    velocityShapeBox.addItem("Early swell", 2);
    velocityShapeBox.addItem("Late swell", 3);
    addAndMakeVisible(velocityShapeBox);
    velocityShapeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.getApvts(), "velocityShape", velocityShapeBox);
    
    voicingLabel.setText("Voicing", juce::dontSendNotification);
    voicingLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(voicingLabel);
    voicingSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    voicingSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(voicingSlider);
    voicingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getApvts(), "voicing", voicingSlider);
    
    humanizeLabel.setText("Humanize", juce::dontSendNotification);
    humanizeLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(humanizeLabel);
    humanizeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    humanizeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(humanizeSlider);
    humanizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getApvts(), "humanize", humanizeSlider);
    
    floorLabel.setText("Floor", juce::dontSendNotification);
    floorLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(floorLabel);
    floorSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    floorSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(floorSlider);
    floorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getApvts(), "floor", floorSlider);
    
    compThresholdLabel.setText("Comp Threshold", juce::dontSendNotification);
    compThresholdLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(compThresholdLabel);
    compThresholdSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    compThresholdSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(compThresholdSlider);
    compThresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getApvts(), "compThreshold", compThresholdSlider);
    
    compRatioLabel.setText("Comp Ratio", juce::dontSendNotification);
    compRatioLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(compRatioLabel);
    compRatioSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    compRatioSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(compRatioSlider);
    compRatioAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getApvts(), "compRatio", compRatioSlider);
    
    pulseDepthLabel.setText("Pulse Depth", juce::dontSendNotification);
    pulseDepthLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(pulseDepthLabel);
    pulseDepthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    pulseDepthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(pulseDepthSlider);
    pulseDepthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getApvts(), "pulseDepth", pulseDepthSlider);
    
    pulseRateLabel.setText("Pulse Rate", juce::dontSendNotification);
    pulseRateLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(pulseRateLabel);
    pulseRateBox.addItem("1/4 bar", 1);
    pulseRateBox.addItem("1/2 bar", 2);
    pulseRateBox.addItem("1 bar", 3);
    pulseRateBox.addItem("2 bars", 4);
    pulseRateBox.addItem("4 bars", 5);
    addAndMakeVisible(pulseRateBox);
    pulseRateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.getApvts(), "pulseRate", pulseRateBox);
    
    pulseOffsetLabel.setText("Pulse Offset", juce::dontSendNotification);
    pulseOffsetLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(pulseOffsetLabel);
    pulseOffsetSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    pulseOffsetSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(pulseOffsetSlider);
    pulseOffsetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getApvts(), "pulseOffset", pulseOffsetSlider);
    
    driftDepthLabel.setText("Drift Depth", juce::dontSendNotification);
    driftDepthLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(driftDepthLabel);
    driftDepthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    driftDepthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(driftDepthSlider);
    driftDepthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getApvts(), "driftDepth", driftDepthSlider);
    
    xlModeToggle.setButtonText("XL Mode");
    addAndMakeVisible(xlModeToggle);
    xlModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.getApvts(), "xlMode", xlModeToggle);
    
    startTimerHz(30);
}

RubatoEditor::~RubatoEditor()
{
    stopTimer();
}

void RubatoEditor::timerCallback()
{
    const int newPhraseCurve = processor.getPhraseCurveValue();
    if (newPhraseCurve != displayPhraseCurve)
    {
        displayPhraseCurve = newPhraseCurve;
        repaint();
    }
    
    const int newTimingIntensity = processor.getPhraseTimingIntensity();
    if (newTimingIntensity != displayTimingIntensity)
    {
        displayTimingIntensity = newTimingIntensity;
        repaint();
    }
    
    bool needsRepaint = false;
    for (int i = 0; i < 16; ++i)
    {
        int newVal;
        if (showInput)
            newVal = processor.inputBandVelocities[i].value.load(std::memory_order_relaxed);
        else
            newVal = processor.outputBandVelocities[i].value.load(std::memory_order_relaxed);
        
        if (displayBandVelocities[i] != newVal)
        {
            displayBandVelocities[i] = newVal;
            needsRepaint = true;
        }
    }
    
    int currentWritePos = processor.getNoteRingWritePos();
    auto noteRing = processor.getNoteRing(currentWritePos);
    
    int newNoteCount = (currentWritePos - lastSeenWritePos + RubatoProcessor::NOTE_RING_SIZE) % RubatoProcessor::NOTE_RING_SIZE;
    
    for (int i = 0; i < newNoteCount; ++i) {
        int idx = (lastSeenWritePos + i) % RubatoProcessor::NOTE_RING_SIZE;
        const auto& note = noteRing[idx];
        displayNotes.push_back({note.pitch, note.inputVel, note.outputVel, 0});
    }
    
    lastSeenWritePos = currentWritePos;
    
    const int msPerFrame = 33;
    const int fadeMs = 1800;
    const int maxDisplayNotes = 512;
    
    auto it = displayNotes.begin();
    while (it != displayNotes.end()) {
        it->ageMs += msPerFrame;
        if (it->ageMs > fadeMs) {
            it = displayNotes.erase(it);
        } else {
            ++it;
        }
    }
    
    while (displayNotes.size() > maxDisplayNotes) {
        displayNotes.erase(displayNotes.begin());
    }
    
    if (!displayNotes.empty() || needsRepaint)
        repaint();
}

void RubatoEditor::drawPhraseMeter(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(bounds);
    
    g.setColour(juce::Colour(0xff404040));
    g.drawRect(bounds, 1);
    
    const float fillWidth = (displayTimingIntensity / 127.0f) * (bounds.getWidth() - 2);
    juce::Rectangle<float> fillRect(
        static_cast<float>(bounds.getX() + 1),
        static_cast<float>(bounds.getY() + 1),
        fillWidth,
        static_cast<float>(bounds.getHeight() - 2)
    );
    
    fillRect = fillRect.constrainedWithin(bounds.toFloat().reduced(1.0f));
    
    g.setColour(juce::Colour(0xff9370db));
    g.fillRect(fillRect);
}

void RubatoEditor::drawShapeMeter(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(bounds);
    
    g.setColour(juce::Colour(0xff404040));
    g.drawRect(bounds, 1);
    
    const float fillHeight = (displayPhraseCurve / 127.0f) * (bounds.getHeight() - 2);
    const float fillY = bounds.getBottom() - 1 - fillHeight;
    
    juce::Rectangle<float> fillRect(
        static_cast<float>(bounds.getX() + 1),
        fillY,
        static_cast<float>(bounds.getWidth() - 2),
        fillHeight
    );
    
    fillRect = fillRect.constrainedWithin(bounds.toFloat().reduced(1.0f));
    
    g.setColour(juce::Colour(0xffff8c00));
    g.fillRect(fillRect);
}

void RubatoEditor::drawKeyboardBands(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(bounds);
    
    g.setColour(juce::Colour(0xff404040));
    g.drawRect(bounds, 1);
    
    const int minPitch = 12;
    const int maxPitch = 107;
    const int pitchRange = maxPitch - minPitch;
    const int fadeMs = 1800;
    
    for (const auto& note : displayNotes) {
        int pitch = juce::jlimit(minPitch, maxPitch, static_cast<int>(note.pitch));
        
        float pitchNorm = (pitch - minPitch) / static_cast<float>(pitchRange);
        float x = bounds.getX() + pitchNorm * bounds.getWidth();
        
        int velocity = showInput ? note.inputVel : note.outputVel;
        float velNorm = velocity / 127.0f;
        float y = bounds.getBottom() - velNorm * bounds.getHeight();
        
        float alpha = 1.0f - (note.ageMs / static_cast<float>(fadeMs));
        alpha = juce::jlimit(0.0f, 1.0f, alpha);
        
        juce::Colour lowColor(0xff7b2d8e);
        juce::Colour highColor(0xffe0ffff);
        juce::Colour noteColor = lowColor.interpolatedWith(highColor, pitchNorm);
        noteColor = noteColor.withAlpha(alpha);
        
        g.setColour(noteColor);
        g.fillEllipse(x - 3.0f, y - 3.0f, 6.0f, 6.0f);
    }
}

void RubatoEditor::drawControlGroup(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title)
{
    g.setColour(juce::Colour(0xff303030));
    g.drawRect(bounds, 1);
    
    g.setColour(juce::Colours::lightgrey);
    g.setFont(12.0f);
    g.drawText(title, bounds.withHeight(20), juce::Justification::centred);
}

void RubatoEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    juce::Rectangle<int> phraseMeterBounds(20, 20, 400, 8);
    drawPhraseMeter(g, phraseMeterBounds);
    
    g.setColour(juce::Colours::lightgrey);
    g.setFont(16.0f);
    g.drawText("Rubato", juce::Rectangle<int>(20, 32, 400, 20), juce::Justification::centred);
    
    juce::Rectangle<int> shapeMeterBounds(20, 80, 30, 120);
    drawShapeMeter(g, shapeMeterBounds);
    
    juce::Rectangle<int> keyboardBandsBounds(60, 80, 720, 120);
    drawKeyboardBands(g, keyboardBandsBounds);
    
    g.setColour(juce::Colours::lightgrey);
    g.setFont(11.0f);
    g.drawText("All notes", juce::Rectangle<int>(60, 208, 80, 12), juce::Justification::centred);
    
    juce::Rectangle<int> timeGroupBounds(20, 240, 240, 380);
    drawControlGroup(g, timeGroupBounds, "Time");
    
    juce::Rectangle<int> velocityGroupBounds(280, 240, 240, 665);
    drawControlGroup(g, velocityGroupBounds, "Velocity");
    
    juce::Rectangle<int> pulseGroupBounds(540, 240, 240, 280);
    drawControlGroup(g, pulseGroupBounds, "Pulse / Drift");
}

void RubatoEditor::resized()
{
    preButton.setBounds(60, 220, 38, 22);
    postButton.setBounds(100, 220, 38, 22);
    
    int timeX = 30;
    int timeY = 265;
    
    int sliderRowHeight = 95;
    int comboRowHeight = 35;
    int buttonRowHeight = 40;
    
    int row = 0;
    amountLabel.setBounds(timeX, timeY + row, 100, 20);
    amountSlider.setBounds(timeX + 110, timeY + row - 10, 80, 80);
    
    row += sliderRowHeight;
    shapeLabel.setBounds(timeX, timeY + row, 100, 20);
    shapeBox.setBounds(timeX + 110, timeY + row, 100, 20);
    
    row += comboRowHeight;
    phraseLengthLabel.setBounds(timeX, timeY + row, 100, 20);
    phraseLengthSlider.setBounds(timeX + 110, timeY + row - 10, 80, 80);
    
    row += sliderRowHeight;
    triggerModeLabel.setBounds(timeX, timeY + row, 100, 20);
    triggerModeBox.setBounds(timeX + 110, timeY + row, 100, 20);
    
    row += comboRowHeight;
    reAnchorButton.setBounds(timeX, timeY + row, 100, 25);
    
    row += buttonRowHeight;
    chordSpreadLabel.setBounds(timeX, timeY + row, 100, 20);
    chordSpreadSlider.setBounds(timeX + 110, timeY + row - 10, 80, 80);
    
    int velX = 290;
    int velY = 265;
    
    row = 0;
    velocityBoostLabel.setBounds(velX, velY + row, 100, 20);
    velocityBoostSlider.setBounds(velX + 110, velY + row - 10, 80, 80);
    
    row += sliderRowHeight;
    velocityReplaceLabel.setBounds(velX, velY + row, 100, 20);
    velocityReplaceSlider.setBounds(velX + 110, velY + row - 10, 80, 80);
    
    row += sliderRowHeight;
    velocityShapeLabel.setBounds(velX, velY + row, 100, 20);
    velocityShapeBox.setBounds(velX + 110, velY + row, 100, 20);
    
    row += comboRowHeight;
    voicingLabel.setBounds(velX, velY + row, 100, 20);
    voicingSlider.setBounds(velX + 110, velY + row - 10, 80, 80);
    
    row += sliderRowHeight;
    humanizeLabel.setBounds(velX, velY + row, 100, 20);
    humanizeSlider.setBounds(velX + 110, velY + row - 10, 80, 80);
    
    row += sliderRowHeight;
    floorLabel.setBounds(velX, velY + row, 100, 20);
    floorSlider.setBounds(velX + 110, velY + row - 10, 80, 80);
    
    row += sliderRowHeight;
    compThresholdLabel.setBounds(velX, velY + row, 100, 20);
    compThresholdSlider.setBounds(velX + 110, velY + row - 10, 80, 80);
    
    row += sliderRowHeight;
    compRatioLabel.setBounds(velX, velY + row, 100, 20);
    compRatioSlider.setBounds(velX + 110, velY + row - 10, 80, 80);
    
    int pulseX = 550;
    int pulseY = 265;
    
    row = 0;
    pulseDepthLabel.setBounds(pulseX, pulseY + row, 100, 20);
    pulseDepthSlider.setBounds(pulseX + 110, pulseY + row - 10, 80, 80);
    
    row += sliderRowHeight;
    pulseRateLabel.setBounds(pulseX, pulseY + row, 100, 20);
    pulseRateBox.setBounds(pulseX + 110, pulseY + row, 100, 20);
    
    row += comboRowHeight;
    pulseOffsetLabel.setBounds(pulseX, pulseY + row, 100, 20);
    pulseOffsetSlider.setBounds(pulseX + 110, pulseY + row - 10, 80, 80);
    
    row += sliderRowHeight;
    driftDepthLabel.setBounds(pulseX, pulseY + row, 100, 20);
    driftDepthSlider.setBounds(pulseX + 110, pulseY + row - 10, 80, 80);
    
    xlModeToggle.setBounds(550, 540, 100, 25);
}
