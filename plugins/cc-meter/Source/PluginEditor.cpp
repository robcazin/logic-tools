#include "PluginEditor.h"

RubatoEditor::RubatoEditor(RubatoProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(800, 600);
    
    displayBandVelocities.fill(0);
    
    inOutToggle.setButtonText("In / Out");
    inOutToggle.setClickingTogglesState(true);
    inOutToggle.onClick = [this]() { showInput = !inOutToggle.getToggleState(); };
    addAndMakeVisible(inOutToggle);
    
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
    
    if (needsRepaint)
        repaint();
}

void RubatoEditor::drawPhraseMeter(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(bounds);
    
    g.setColour(juce::Colour(0xff404040));
    g.drawRect(bounds, 1);
    
    const float fillWidth = (displayPhraseCurve / 127.0f) * (bounds.getWidth() - 2);
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
    
    g.setColour(juce::Colour(0xff9370db));
    g.fillRect(fillRect);
}

void RubatoEditor::drawKeyboardBands(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    const int bandWidth = bounds.getWidth() / 16;
    const int spacing = 2;
    
    for (int i = 0; i < 16; ++i)
    {
        int x = bounds.getX() + i * bandWidth;
        int bandDisplayWidth = bandWidth - spacing;
        
        juce::Rectangle<int> bandBounds(x, bounds.getY(), bandDisplayWidth, bounds.getHeight());
        
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRect(bandBounds);
        
        g.setColour(juce::Colour(0xff404040));
        g.drawRect(bandBounds, 1);
        
        const float fillHeight = (displayBandVelocities[i] / 127.0f) * (bandBounds.getHeight() - 4);
        const float fillY = bandBounds.getBottom() - 2 - fillHeight;
        
        juce::Rectangle<float> fillRect(
            static_cast<float>(bandBounds.getX() + 2),
            fillY,
            static_cast<float>(bandBounds.getWidth() - 4),
            fillHeight
        );
        
        fillRect = fillRect.constrainedWithin(bandBounds.toFloat().reduced(2.0f));
        
        g.setColour(juce::Colour(0xff00bfff));
        g.fillRect(fillRect);
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
    
    juce::Rectangle<int> shapeMeterBounds(20, 80, 30, 120);
    drawShapeMeter(g, shapeMeterBounds);
    
    juce::Rectangle<int> keyboardBandsBounds(60, 80, 720, 120);
    drawKeyboardBands(g, keyboardBandsBounds);
    
    juce::Rectangle<int> timeGroupBounds(20, 240, 240, 320);
    drawControlGroup(g, timeGroupBounds, "Time");
    
    juce::Rectangle<int> velocityGroupBounds(280, 240, 240, 320);
    drawControlGroup(g, velocityGroupBounds, "Velocity");
    
    juce::Rectangle<int> pulseGroupBounds(540, 240, 240, 160);
    drawControlGroup(g, pulseGroupBounds, "Pulse / Drift");
}

void RubatoEditor::resized()
{
    inOutToggle.setBounds(60, 210, 80, 20);
    
    int timeX = 30;
    int timeY = 265;
    int controlSpacing = 60;
    
    amountLabel.setBounds(timeX, timeY, 100, 20);
    amountSlider.setBounds(timeX + 110, timeY - 10, 80, 80);
    
    shapeLabel.setBounds(timeX, timeY + controlSpacing, 100, 20);
    shapeBox.setBounds(timeX + 110, timeY + controlSpacing, 100, 20);
    
    phraseLengthLabel.setBounds(timeX, timeY + controlSpacing * 2, 100, 20);
    phraseLengthSlider.setBounds(timeX + 110, timeY + controlSpacing * 2 - 10, 80, 80);
    
    triggerModeLabel.setBounds(timeX, timeY + controlSpacing * 3, 100, 20);
    triggerModeBox.setBounds(timeX + 110, timeY + controlSpacing * 3, 100, 20);
    
    reAnchorButton.setBounds(timeX, timeY + controlSpacing * 4, 100, 25);
    
    chordSpreadLabel.setBounds(timeX, timeY + controlSpacing * 5 - 10, 100, 20);
    chordSpreadSlider.setBounds(timeX + 110, timeY + controlSpacing * 5 - 20, 80, 80);
    
    int velX = 290;
    int velY = 265;
    
    velocityBoostLabel.setBounds(velX, velY, 100, 20);
    velocityBoostSlider.setBounds(velX + 110, velY - 10, 80, 80);
    
    velocityReplaceLabel.setBounds(velX, velY + controlSpacing, 100, 20);
    velocityReplaceSlider.setBounds(velX + 110, velY + controlSpacing - 10, 80, 80);
    
    velocityShapeLabel.setBounds(velX, velY + controlSpacing * 2, 100, 20);
    velocityShapeBox.setBounds(velX + 110, velY + controlSpacing * 2, 100, 20);
    
    compThresholdLabel.setBounds(velX, velY + controlSpacing * 3, 100, 20);
    compThresholdSlider.setBounds(velX + 110, velY + controlSpacing * 3 - 10, 80, 80);
    
    compRatioLabel.setBounds(velX, velY + controlSpacing * 4, 100, 20);
    compRatioSlider.setBounds(velX + 110, velY + controlSpacing * 4 - 10, 80, 80);
    
    int pulseX = 550;
    int pulseY = 265;
    
    pulseDepthLabel.setBounds(pulseX, pulseY, 100, 20);
    pulseDepthSlider.setBounds(pulseX + 110, pulseY - 10, 80, 80);
    
    driftDepthLabel.setBounds(pulseX, pulseY + controlSpacing, 100, 20);
    driftDepthSlider.setBounds(pulseX + 110, pulseY + controlSpacing - 10, 80, 80);
    
    xlModeToggle.setBounds(550, 420, 100, 25);
}
