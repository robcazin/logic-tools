#pragma once

#include "PluginProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>

class CcMeterEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit CcMeterEditor(CcMeterProcessor&);
    ~CcMeterEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    CcMeterProcessor& processor;
    
    juce::Label ccNumberLabel;
    juce::ComboBox ccNumberBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> ccNumberAttachment;
    
    int displayValue = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CcMeterEditor)
};
