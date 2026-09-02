#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

juce::AudioProcessorValueTreeState::ParameterLayout RubatoProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Time controls
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "amountMs", "Amount (ms)", 0, 120, 40));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "shape", "Shape", juce::StringArray{"Symmetric bell", "Early drag", "Late drag"}, 0));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "phraseLength", "Phrase Length (bars)", 1, 32, 4));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "triggerMode", "Trigger Mode", juce::StringArray{"Cycle Region", "Fixed Bar Length"}, 1));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "reAnchor", "Re-Anchor", false));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "chordSpreadMs", "Chord Spread (ms)", 0, 30, 0));
    
    // Velocity controls
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "velocityBoost", "Velocity Boost", -40, 40, 0));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "velocityReplace", "Velocity Replace (%)", 0, 100, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "velocityShape", "Velocity Shape", juce::StringArray{"Symmetric bell", "Early swell", "Late swell"}, 0));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "compThreshold", "Comp Threshold", 1, 127, 127));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "compRatio", "Comp Ratio", 1.0f, 20.0f, 1.0f));
    
    // Pulse/Drift controls
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "pulseDepth", "Pulse Depth", 0, 100, 0));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "driftDepth", "Drift Depth", 0, 100, 0));
    
    // XL mode
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "xlMode", "XL Mode", false));
    
    return layout;
}

RubatoProcessor::RubatoProcessor()
    : AudioProcessor(BusesProperties()),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    inputBandPeaks.fill(0);
    outputBandPeaks.fill(0);
    inputBandDecay.fill(0);
    outputBandDecay.fill(0);
    
    absoluteSampleClock = 0;
    lastPpqPosition = -1.0;
    wasPlaying = false;
}

void RubatoProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate;
}

void RubatoProcessor::releaseResources()
{
}

bool RubatoProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return true;
}

float RubatoProcessor::curveValue(float t, int shapeIndex)
{
    const float pi = juce::MathConstants<float>::pi;
    
    if (shapeIndex == 0) {
        return std::sin(pi * t);
    } else if (shapeIndex == 1) {
        return std::sin(pi * std::pow(t, 0.6f));
    } else {
        return std::sin(pi * std::pow(t, 1.6f));
    }
}

void RubatoProcessor::applyLife(float t, float& amount)
{
    const int pulseDepth = apvts.getRawParameterValue("pulseDepth")->load();
    const int driftDepth = apvts.getRawParameterValue("driftDepth")->load();
    
    if (pulseDepth == 0 && driftDepth == 0)
        return;
    
    const float pi = juce::MathConstants<float>::pi;
    float pulse = (pulseDepth / 100.0f) * std::sin(pi * t);
    
    float drift = 0.0f;
    if (driftDepth > 0) {
        drift = (driftDepth / 100.0f) * std::sin(pi * t / 2.0f);
    }
    
    amount *= (1.0f + pulse + drift);
}

int RubatoProcessor::compressVelocity(int vel, int threshold, float ratio)
{
    if (ratio <= 1.0f || vel <= threshold)
        return vel;
    
    int compressed = threshold + static_cast<int>((vel - threshold) / ratio);
    
    if (vel < threshold && ratio > 1.0f) {
        float floor = threshold / ratio;
        compressed = static_cast<int>(floor + (vel - floor) * ratio);
    }
    
    return juce::jlimit(1, 127, compressed);
}

int RubatoProcessor::getBandIndex(int pitch)
{
    pitch = juce::jlimit(12, 107, pitch);
    int octave = (pitch - 12) / 12;
    int half = (pitch % 12) < 6 ? 0 : 1;
    return juce::jlimit(0, 15, octave * 2 + half);
}

float RubatoProcessor::calculatePhraseFraction(juce::AudioPlayHead::PositionInfo& pos)
{
    const int triggerMode = apvts.getRawParameterValue("triggerMode")->load();
    
    if (triggerMode == 0) {
        if (!pos.getIsLooping())
            return -1.0f;
        
        auto loopPoints = pos.getLoopPoints();
        if (!loopPoints.hasValue())
            return -1.0f;
        
        double cycleLen = loopPoints->ppqEnd - loopPoints->ppqStart;
        if (cycleLen <= 0)
            return -1.0f;
        
        auto ppqPos = pos.getPpqPosition();
        if (!ppqPos.hasValue())
            return -1.0f;
        
        double t = (*ppqPos - loopPoints->ppqStart) / cycleLen;
        return static_cast<float>(t - std::floor(t));
    } else {
        auto timeInSamples = pos.getTimeInSamples();
        auto ppqPos = pos.getPpqPosition();
        auto timeSignature = pos.getTimeSignature();
        
        if (!ppqPos.hasValue() || !timeSignature.hasValue())
            return -1.0f;
        
        double beatsPerBar = timeSignature->numerator * 4.0 / timeSignature->denominator;
        int phraseBars = apvts.getRawParameterValue("phraseLength")->load();
        double phraseLengthBeats = phraseBars * beatsPerBar;
        
        double relativeBeat = *ppqPos - anchorBeat;
        if (relativeBeat < 0)
            return -1.0f;
        
        double phraseIndex = std::floor(relativeBeat / phraseLengthBeats);
        double phraseStartBeat = phraseIndex * phraseLengthBeats;
        return static_cast<float>((relativeBeat - phraseStartBeat) / phraseLengthBeats);
    }
}

void RubatoProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();
    
    auto playHead = getPlayHead();
    if (playHead == nullptr) {
        return;
    }
    
    auto posInfo = playHead->getPosition();
    if (!posInfo.hasValue()) {
        return;
    }
    
    auto pos = *posInfo;
    
    auto isPlaying = pos.getIsPlaying();
    bool isPlayingNow = isPlaying;
    
    if (!isPlayingNow && wasPlaying) {
        delayQueue.clear();
        
        juce::MidiBuffer clearMidi;
        for (int noteKey : activeDelayedNotes) {
            int channel = noteKey / 128;
            int pitch = noteKey % 128;
            clearMidi.addEvent(juce::MidiMessage::noteOff(channel, pitch), 0);
        }
        activeDelayedNotes.clear();
        noteOffDelays.clear();
        
        midiMessages.swapWith(clearMidi);
        
        transportWarm = false;
        warmBlocks = 0;
        anchorBeat = 1.0;
        wasPlaying = false;
        return;
    }
    
    wasPlaying = isPlayingNow;
    
    if (isPlayingNow) {
        warmBlocks++;
        if (warmBlocks >= 2)
            transportWarm = true;
    } else {
        transportWarm = false;
        warmBlocks = 0;
        anchorBeat = 1.0;
    }
    
    auto ppqPos = pos.getPpqPosition();
    if (ppqPos.hasValue()) {
        double currentPpq = *ppqPos;
        if (lastPpqPosition >= 0.0 && currentPpq < lastPpqPosition - 0.1) {
            auto it = delayQueue.begin();
            while (it != delayQueue.end()) {
                if (it->message.isNoteOn()) {
                    int key = it->channel * 128 + it->noteNumber;
                    activeDelayedNotes.erase(key);
                    if (noteOffDelays.count(key) > 0 && !noteOffDelays[key].empty()) {
                        noteOffDelays[key].pop_front();
                    }
                }
                it = delayQueue.erase(it);
            }
        }
        lastPpqPosition = currentPpq;
    }
    
    const bool xlMode = apvts.getRawParameterValue("xlMode")->load() > 0.5f;
    const int amountMs = apvts.getRawParameterValue("amountMs")->load();
    const int shapeIndex = apvts.getRawParameterValue("shape")->load();
    const int velocityBoost = apvts.getRawParameterValue("velocityBoost")->load();
    const int velocityReplace = apvts.getRawParameterValue("velocityReplace")->load();
    const int velocityShapeIndex = apvts.getRawParameterValue("velocityShape")->load();
    const int compThreshold = apvts.getRawParameterValue("compThreshold")->load();
    const float compRatio = apvts.getRawParameterValue("compRatio")->load();
    const int chordSpreadMs = apvts.getRawParameterValue("chordSpreadMs")->load();
    
    float phraseFrac = calculatePhraseFraction(pos);
    
    if (phraseFrac >= 0.0f) {
        float curveVal = curveValue(phraseFrac, shapeIndex);
        phraseCurveValue.store(static_cast<int>(curveVal * 127.0f), std::memory_order_relaxed);
        
        float timingAmount = amountMs * curveVal;
        applyLife(phraseFrac, timingAmount);
        
        float normalizedIntensity = juce::jlimit(0.0f, 1.0f, timingAmount / 120.0f);
        phraseTimingIntensity.store(static_cast<int>(normalizedIntensity * 127.0f), std::memory_order_relaxed);
    } else {
        phraseTimingIntensity.store(0, std::memory_order_relaxed);
    }
    
    juce::MidiBuffer processedMidi;
    
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        int samplePos = metadata.samplePosition;
        
        if (msg.isController())
        {
            if (xlMode) {
                int ccNum = msg.getControllerNumber();
                if (ccNum >= 21 && ccNum <= 26) {
                    int ccVal = msg.getControllerValue();
                    
                    const char* paramIds[] = {"amountMs", "velocityBoost", "compThreshold", "compRatio", "phraseLength", "chordSpreadMs"};
                    int paramIndex = ccNum - 21;
                    
                    if (paramIndex < 6) {
                        auto param = apvts.getParameter(paramIds[paramIndex]);
                        if (param != nullptr) {
                            param->setValueNotifyingHost(ccVal / 127.0f);
                        }
                    }
                    continue;
                }
            }
            
            lastCCValues[msg.getControllerNumber()] = msg.getControllerValue();
            processedMidi.addEvent(msg, samplePos);
        }
        else if (msg.isNoteOn())
        {
            int pitch = msg.getNoteNumber();
            int channel = msg.getChannel();
            int velocity = msg.getVelocity();
            
            int bandIndex = getBandIndex(pitch);
            int inputVel = velocity;
            
            if (inputBandPeaks[bandIndex] < inputVel)
                inputBandPeaks[bandIndex] = inputVel;
            inputBandVelocities[bandIndex].value.store(inputBandPeaks[bandIndex], std::memory_order_relaxed);
            
            float spreadMs = 0.0f;
            if (chordSpreadMs > 0) {
                spreadMs = juce::Random::getSystemRandom().nextFloat() * chordSpreadMs;
            }
            
            if (phraseFrac < 0.0f) {
                velocity = compressVelocity(velocity, compThreshold, compRatio);
                
                int delaySamples = static_cast<int>((spreadMs / 1000.0) * currentSampleRate);
                int64_t targetSample = absoluteSampleClock + samplePos + delaySamples;
                DelayedNote delayed{juce::MidiMessage::noteOn(channel, pitch, (juce::uint8)velocity), targetSample, pitch, channel};
                delayQueue.push_back(delayed);
                
                int key = channel * 128 + pitch;
                activeDelayedNotes.insert(key);
                noteOffDelays[key].push_back(static_cast<int>(spreadMs));
            } else {
                float curveVal = curveValue(phraseFrac, shapeIndex);
                float timingAmount = amountMs * curveVal;
                applyLife(phraseFrac, timingAmount);
                
                float totalDelayMs = timingAmount + spreadMs;
                
                float velCurveVal = curveValue(phraseFrac, velocityShapeIndex);
                
                int newVelocity = velocity;
                if (velocityReplace > 0) {
                    int replaceVel = 1 + static_cast<int>(velCurveVal * 126.0f);
                    newVelocity = static_cast<int>(velocity * (1.0f - velocityReplace / 100.0f) + replaceVel * (velocityReplace / 100.0f));
                }
                
                if (velocityBoost != 0) {
                    int boostAmount = static_cast<int>(velCurveVal * velocityBoost);
                    newVelocity += boostAmount;
                }
                
                newVelocity = juce::jlimit(1, 127, newVelocity);
                newVelocity = compressVelocity(newVelocity, compThreshold, compRatio);
                
                if (outputBandPeaks[bandIndex] < newVelocity)
                    outputBandPeaks[bandIndex] = newVelocity;
                outputBandVelocities[bandIndex].value.store(outputBandPeaks[bandIndex], std::memory_order_relaxed);
                
                int delaySamples = static_cast<int>((totalDelayMs / 1000.0) * currentSampleRate);
                int64_t targetSample = absoluteSampleClock + samplePos + delaySamples;
                DelayedNote delayed{juce::MidiMessage::noteOn(channel, pitch, (juce::uint8)newVelocity), targetSample, pitch, channel};
                delayQueue.push_back(delayed);
                
                int key = channel * 128 + pitch;
                activeDelayedNotes.insert(key);
                noteOffDelays[key].push_back(static_cast<int>(totalDelayMs));
            }
        }
        else if (msg.isNoteOff())
        {
            int pitch = msg.getNoteNumber();
            int channel = msg.getChannel();
            int key = channel * 128 + pitch;
            
            if (noteOffDelays.count(key) > 0 && !noteOffDelays[key].empty()) {
                int delayMs = noteOffDelays[key].front();
                noteOffDelays[key].pop_front();
                
                int delaySamples = static_cast<int>((delayMs / 1000.0) * currentSampleRate);
                int64_t targetSample = absoluteSampleClock + samplePos + delaySamples;
                DelayedNote delayed{juce::MidiMessage::noteOff(channel, pitch, (juce::uint8)0), targetSample, pitch, channel};
                delayQueue.push_back(delayed);
            } else {
                processedMidi.addEvent(msg, samplePos);
            }
        }
        else
        {
            processedMidi.addEvent(msg, samplePos);
        }
    }
    
    auto it = delayQueue.begin();
    while (it != delayQueue.end()) {
        if (it->targetSample < absoluteSampleClock + buffer.getNumSamples()) {
            int64_t offset = it->targetSample - absoluteSampleClock;
            int outSample = juce::jmax(0, static_cast<int>(offset));
            processedMidi.addEvent(it->message, outSample);
            
            if (it->message.isNoteOn()) {
                int key = it->channel * 128 + it->noteNumber;
            } else if (it->message.isNoteOff()) {
                int key = it->channel * 128 + it->noteNumber;
                activeDelayedNotes.erase(key);
            }
            
            it = delayQueue.erase(it);
        } else {
            ++it;
        }
    }
    
    absoluteSampleClock += buffer.getNumSamples();
    
    for (int i = 0; i < 16; ++i) {
        inputBandDecay[i]++;
        if (inputBandDecay[i] > 10) {
            inputBandPeaks[i] = static_cast<int>(inputBandPeaks[i] * 0.95f);
            inputBandVelocities[i].value.store(inputBandPeaks[i], std::memory_order_relaxed);
            inputBandDecay[i] = 0;
        }
        
        outputBandDecay[i]++;
        if (outputBandDecay[i] > 10) {
            outputBandPeaks[i] = static_cast<int>(outputBandPeaks[i] * 0.95f);
            outputBandVelocities[i].value.store(outputBandPeaks[i], std::memory_order_relaxed);
            outputBandDecay[i] = 0;
        }
    }
    
    midiMessages.swapWith(processedMidi);
}

juce::AudioProcessorEditor* RubatoProcessor::createEditor()
{
    return new RubatoEditor(*this);
}

void RubatoProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void RubatoProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RubatoProcessor();
}
