#include "PluginEditor.h"

CcMeterEditor::CcMeterEditor(CcMeterProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(120, 400);
    
    ccNumberLabel.setText("CC", juce::dontSendNotification);
    ccNumberLabel.setJustificationType(juce::Justification::centred);
    ccNumberLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(ccNumberLabel);
    
    for (int i = 0; i <= 127; ++i)
        ccNumberBox.addItem(juce::String(i), i + 1);
    
    ccNumberBox.setSelectedId(120, juce::dontSendNotification);
    ccNumberBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a2a));
    ccNumberBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    ccNumberBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff404040));
    ccNumberBox.setColour(juce::ComboBox::arrowColourId, juce::Colours::lightgrey);
    addAndMakeVisible(ccNumberBox);
    
    ccNumberAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.getApvts(), "ccNumber", ccNumberBox);
    
    startTimerHz(30);
}

CcMeterEditor::~CcMeterEditor()
{
    stopTimer();
}

void CcMeterEditor::timerCallback()
{
    const int newValue = processor.getLastCcValue();
    if (newValue != displayValue)
    {
        displayValue = newValue;
        repaint();
    }
}

void CcMeterEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    const int meterX = 20;
    const int meterY = 70;
    const int meterWidth = 80;
    const int meterHeight = 250;
    
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(meterX, meterY, meterWidth, meterHeight);
    
    g.setColour(juce::Colour(0xff404040));
    g.drawRect(meterX, meterY, meterWidth, meterHeight, 2);
    
    const float fillHeight = (displayValue / 127.0f) * meterHeight;
    const float fillY = meterY + meterHeight - fillHeight;
    
    juce::ColourGradient gradient(
        juce::Colour(0xff00ff00), meterX, meterY + meterHeight,
        juce::Colour(0xffffff00), meterX, meterY + meterHeight * 0.5f,
        false);
    gradient.addColour(0.0, juce::Colour(0xffff0000));
    
    g.setGradientFill(gradient);
    g.fillRect(meterX + 2, fillY, meterWidth - 4, fillHeight - 2);
    
    g.setColour(juce::Colour(0xff606060));
    for (int i = 0; i <= 8; ++i)
    {
        const int y = meterY + (meterHeight * i / 8);
        g.drawLine(meterX, y, meterX + meterWidth, y, 1.0f);
    }
    
    g.setColour(juce::Colours::lightgrey);
    g.setFont(32.0f);
    g.drawText(juce::String(displayValue), 
               meterX, meterY + meterHeight + 10, 
               meterWidth, 40, 
               juce::Justification::centred);
    
    g.setFont(12.0f);
    g.setColour(juce::Colour(0xff808080));
    g.drawText("127", meterX + meterWidth + 5, meterY - 8, 30, 16, juce::Justification::left);
    g.drawText("0", meterX + meterWidth + 5, meterY + meterHeight - 8, 30, 16, juce::Justification::left);
}

void CcMeterEditor::resized()
{
    ccNumberLabel.setBounds(10, 10, 40, 25);
    ccNumberBox.setBounds(55, 10, 55, 25);
}
