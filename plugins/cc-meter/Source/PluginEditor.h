#pragma once

#include "PluginProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>

class RubatoEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit RubatoEditor(RubatoProcessor&);
    ~RubatoEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawPhraseMeter(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawShapeMeter(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawKeyboardBands(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawControlGroup(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title);

    RubatoProcessor& processor;
    
    juce::TextButton preButton;
    juce::TextButton postButton;
    bool showInput = true;
    
    juce::Label amountLabel;
    juce::Slider amountSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttachment;
    
    juce::Label shapeLabel;
    juce::ComboBox shapeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttachment;
    
    juce::Label phraseLengthLabel;
    juce::Slider phraseLengthSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> phraseLengthAttachment;
    
    juce::Label triggerModeLabel;
    juce::ComboBox triggerModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> triggerModeAttachment;
    
    juce::TextButton reAnchorButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> reAnchorAttachment;
    
    juce::Label chordSpreadLabel;
    juce::Slider chordSpreadSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> chordSpreadAttachment;
    
    juce::Label velocityBoostLabel;
    juce::Slider velocityBoostSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> velocityBoostAttachment;
    
    juce::Label velocityReplaceLabel;
    juce::Slider velocityReplaceSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> velocityReplaceAttachment;
    
    juce::Label velocityShapeLabel;
    juce::ComboBox velocityShapeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> velocityShapeAttachment;
    
    juce::Label compThresholdLabel;
    juce::Slider compThresholdSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> compThresholdAttachment;
    
    juce::Label compRatioLabel;
    juce::Slider compRatioSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> compRatioAttachment;
    
    juce::Label pulseDepthLabel;
    juce::Slider pulseDepthSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pulseDepthAttachment;
    
    juce::Label pulseRateLabel;
    juce::ComboBox pulseRateBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> pulseRateAttachment;
    
    juce::Label pulseOffsetLabel;
    juce::Slider pulseOffsetSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pulseOffsetAttachment;
    
    juce::Label driftDepthLabel;
    juce::Slider driftDepthSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driftDepthAttachment;
    
    juce::ToggleButton xlModeToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> xlModeAttachment;
    
    int displayPhraseCurve = 0;
    int displayTimingIntensity = 0;
    std::array<int, 16> displayBandVelocities;
    
    struct DisplayNote {
        uint8_t pitch;
        uint8_t inputVel;
        uint8_t outputVel;
        int ageMs;
    };
    std::vector<DisplayNote> displayNotes;
    int64_t lastNoteTimestamp = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RubatoEditor)
};
