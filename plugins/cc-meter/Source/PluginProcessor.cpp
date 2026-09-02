#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout CcMeterProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "ccNumber",
        "CC Number",
        0, 127, 119));
    
    return layout;
}

CcMeterProcessor::CcMeterProcessor()
    : AudioProcessor(BusesProperties()),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

void CcMeterProcessor::prepareToPlay(double, int)
{
}

void CcMeterProcessor::releaseResources()
{
}

bool CcMeterProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return true;
}

void CcMeterProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();
    
    const int watchedCc = apvts.getRawParameterValue("ccNumber")->load();
    juce::MidiBuffer filteredMidi;
    
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        
        if (msg.isController())
        {
            const int ccNum = msg.getControllerNumber();
            const int ccVal = msg.getControllerValue();
            
            if (ccNum == watchedCc)
            {
                lastCcValue.store(ccVal, std::memory_order_relaxed);
            }
            else
            {
                filteredMidi.addEvent(msg, metadata.samplePosition);
            }
        }
        else
        {
            filteredMidi.addEvent(msg, metadata.samplePosition);
        }
    }
    
    midiMessages.swapWith(filteredMidi);
}

juce::AudioProcessorEditor* CcMeterProcessor::createEditor()
{
    return new CcMeterEditor(*this);
}

void CcMeterProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void CcMeterProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CcMeterProcessor();
}
