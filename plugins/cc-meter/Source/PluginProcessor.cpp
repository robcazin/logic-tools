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
        "phraseLengthMode", "Phrase Length Mode", juce::StringArray{"Manual", "Auto"}, 0));
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
        "voicing", "Voicing", 0, 100, 0));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "humanize", "Humanize", 0, 100, 0));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "floor", "Floor", 1, 127, 1));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "compThreshold", "Comp Threshold", 1, 127, 127));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "compRatio", "Comp Ratio", 1.0f, 20.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "squish", "Squish", -100, 100, 0));
    
    // Pulse/Drift controls
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "pulseDepth", "Pulse Depth", 0, 100, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "pulseRate", "Pulse Rate", juce::StringArray{"1/4 bar", "1/2 bar", "1 bar", "2 bars", "4 bars"}, 2));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "pulseOffset", "Pulse Offset", 0, 100, 0));
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
    
    for (auto& note : noteRing) {
        note.pitch = 0;
        note.inputVel = 0;
        note.outputVel = 0;
        note.timestamp = 0;
    }
}

std::array<RubatoProcessor::NoteEvent, RubatoProcessor::NOTE_RING_SIZE> RubatoProcessor::getNoteRing()
{
    return noteRing;
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

void RubatoProcessor::applyLife(juce::AudioPlayHead::PositionInfo& pos, float& amount)
{
    const int pulseDepth = apvts.getRawParameterValue("pulseDepth")->load();
    const int driftDepth = apvts.getRawParameterValue("driftDepth")->load();
    
    if (pulseDepth == 0 && driftDepth == 0)
        return;
    
    auto ppqPos = pos.getPpqPosition();
    auto timeSignature = pos.getTimeSignature();
    
    if (!ppqPos.hasValue() || !timeSignature.hasValue())
        return;
    
    double beatsPerBar = timeSignature->numerator * 4.0 / timeSignature->denominator;
    double blockStartBeat = *ppqPos - anchorBeat;
    if (blockStartBeat < 0.0)
        blockStartBeat = 0.0;
    double bars = blockStartBeat / beatsPerBar;
    
    const float pi = juce::MathConstants<float>::pi;
    float base = amount;
    
    if (pulseDepth > 0) {
        const int pulseRateIndex = apvts.getRawParameterValue("pulseRate")->load();
        const int pulseOffsetPct = apvts.getRawParameterValue("pulseOffset")->load();
        
        float pulsePeriodBars = 1.0f;
        if (pulseRateIndex == 0) pulsePeriodBars = 0.25f;
        else if (pulseRateIndex == 1) pulsePeriodBars = 0.5f;
        else if (pulseRateIndex == 2) pulsePeriodBars = 1.0f;
        else if (pulseRateIndex == 3) pulsePeriodBars = 2.0f;
        else pulsePeriodBars = 4.0f;
        
        float offset01 = pulseOffsetPct / 100.0f;
        float unipolarPulse = 0.5f * (1.0f + std::sin(2.0f * pi * (static_cast<float>(bars) / pulsePeriodBars + offset01)));
        
        float pulseDepth01 = pulseDepth / 100.0f;
        amount = base + (1.0f - base) * pulseDepth01 * unipolarPulse;
    }
    
    if (driftDepth > 0) {
        float driftPeriodBars = 2.0f;
        float unipolarDrift = 0.5f * (1.0f + std::sin(2.0f * pi * (static_cast<float>(bars) / driftPeriodBars)));
        
        float driftAmt = 0.35f * (driftDepth / 100.0f);
        base = amount;
        amount = base + (1.0f - base) * driftAmt * unipolarDrift;
    }
}

int RubatoProcessor::compressVelocity(int vel, int threshold, float ratio)
{
    if (ratio <= 1.0f)
        return vel;
    
    if (vel > threshold) {
        int compressed = threshold + static_cast<int>((vel - threshold) / ratio);
        return juce::jlimit(1, 127, compressed);
    }
    
    return vel;
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
        
        int phraseLengthMode = apvts.getRawParameterValue("phraseLengthMode")->load();
        int phraseBars;
        if (phraseLengthMode == 1) {
            phraseBars = timeSignature->numerator;
        } else {
            phraseBars = apvts.getRawParameterValue("phraseLength")->load();
        }
        phraseBars = juce::jlimit(1, 32, phraseBars);
        
        double phraseLengthBeats = phraseBars * beatsPerBar;
        
        double relativeBeat = *ppqPos - anchorBeat;
        if (relativeBeat < 0)
            relativeBeat = 0.0;
        
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
        absoluteSampleClock += buffer.getNumSamples();
        return;
    }
    
    auto posInfo = playHead->getPosition();
    if (!posInfo.hasValue()) {
        absoluteSampleClock += buffer.getNumSamples();
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
        anchorBeat = 0.0;
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
    
    bool reAnchor = apvts.getRawParameterValue("reAnchor")->load() > 0.5f;
    if (reAnchor && !lastReAnchor && ppqPos.hasValue()) {
        anchorBeat = *ppqPos;
    }
    lastReAnchor = reAnchor;
    
    const bool xlMode = apvts.getRawParameterValue("xlMode")->load() > 0.5f;
    const int amountMs = apvts.getRawParameterValue("amountMs")->load();
    const int shapeIndex = apvts.getRawParameterValue("shape")->load();
    const int velocityBoost = apvts.getRawParameterValue("velocityBoost")->load();
    const int velocityReplace = apvts.getRawParameterValue("velocityReplace")->load();
    const int velocityShapeIndex = apvts.getRawParameterValue("velocityShape")->load();
    const int voicing = apvts.getRawParameterValue("voicing")->load();
    const int humanize = apvts.getRawParameterValue("humanize")->load();
    const int floor = apvts.getRawParameterValue("floor")->load();
    const int compThreshold = apvts.getRawParameterValue("compThreshold")->load();
    const float compRatio = apvts.getRawParameterValue("compRatio")->load();
    const int squish = apvts.getRawParameterValue("squish")->load();
    const int chordSpreadMs = apvts.getRawParameterValue("chordSpreadMs")->load();
    
    float phraseFrac = calculatePhraseFraction(pos);
    float phraseCurveVal = 0.0f;
    
    if (phraseFrac >= 0.0f) {
        float curveVal = curveValue(phraseFrac, shapeIndex);
        phraseCurveVal = curveVal;
        phraseCurveValue.store(static_cast<int>(curveVal * 127.0f), std::memory_order_relaxed);
        
        float timingAmount = amountMs * curveVal;
        applyLife(pos, timingAmount);
        
        float normalizedIntensity = juce::jlimit(0.0f, 1.0f, timingAmount / 120.0f);
        phraseTimingIntensity.store(static_cast<int>(normalizedIntensity * 127.0f), std::memory_order_relaxed);
        
        float velCurveVal = curveValue(phraseFrac, velocityShapeIndex);
        velocityShapeCurve.store(static_cast<int>(velCurveVal * 127.0f), std::memory_order_relaxed);
    } else {
        phraseTimingIntensity.store(0, std::memory_order_relaxed);
        velocityShapeCurve.store(0, std::memory_order_relaxed);
    }
    
    juce::Random blockRandom(static_cast<int64_t>(absoluteSampleClock));
    int chordHumanizeOffset = 0;
    if (humanize > 0) {
        float humanizeScale = humanize / 100.0f;
        float maxJitter = 12.0f;
        chordHumanizeOffset = static_cast<int>((blockRandom.nextFloat() * 2.0f - 1.0f) * maxJitter * humanizeScale);
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
                    
                    const char* paramIds[] = {"amountMs", "velocityReplace", "floor", "compThreshold", "phraseLength", "squish"};
                    int paramIndex = ccNum - 21;
                    
                    if (paramIndex < 6) {
                        auto param = apvts.getParameter(paramIds[paramIndex]);
                        if (param != nullptr) {
                            param->setValueNotifyingHost(ccVal / 127.0f);
                        }
                    }
                    continue;
                }
                
                if (ccNum == 76) {
                    int ccVal = msg.getControllerValue();
                    auto param = apvts.getParameter("chordSpreadMs");
                    if (param != nullptr) {
                        param->setValueNotifyingHost(ccVal / 127.0f);
                    }
                    continue;
                }
                
                if (ccNum == 77) {
                    int ccVal = msg.getControllerValue();
                    auto param = apvts.getParameter("velocityBoost");
                    if (param != nullptr) {
                        param->setValueNotifyingHost(ccVal / 127.0f);
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
            
            if (phraseFrac >= 0.0f) {
                float curveVal = curveValue(phraseFrac, shapeIndex);
                float velCurveVal = curveValue(phraseFrac, velocityShapeIndex);
                
                int lo = juce::jlimit(1, 127, floor);
                int hi = juce::jlimit(1, 127, compThreshold);
                if (hi < lo) hi = lo;
                
                int newVelocity = velocity;
                if (velocityReplace > 0) {
                    int shaped = lo + static_cast<int>(std::round(velCurveVal * static_cast<float>(hi - lo)));
                    newVelocity = static_cast<int>(velocity * (1.0f - velocityReplace / 100.0f) + shaped * (velocityReplace / 100.0f));
                }
                
                if (velocityBoost != 0) {
                    int boostAmount = static_cast<int>(velCurveVal * velocityBoost);
                    newVelocity += boostAmount;
                }
                
                newVelocity = juce::jlimit(1, 127, newVelocity);
                
                PendingNote pending;
                pending.pitch = pitch;
                pending.channel = channel;
                pending.velocity = newVelocity;
                pending.inputVel = inputVel;
                pending.samplePos = samplePos;
                pending.arrivalSample = absoluteSampleClock + samplePos;
                clusterBuffer.push_back(pending);
            } else {
                float spreadMs = 0.0f;
                if (chordSpreadMs > 0) {
                    spreadMs = juce::Random::getSystemRandom().nextFloat() * chordSpreadMs;
                }
                
                int newVelocity = compressVelocity(velocity, compThreshold, compRatio);
                
                int delaySamples = static_cast<int>((spreadMs / 1000.0) * currentSampleRate);
                int64_t targetSample = absoluteSampleClock + samplePos + delaySamples;
                DelayedNote delayed{juce::MidiMessage::noteOn(channel, pitch, (juce::uint8)newVelocity), targetSample, pitch, channel};
                delayQueue.push_back(delayed);
                
                int key = channel * 128 + pitch;
                activeDelayedNotes.insert(key);
                noteOffDelays[key].push_back(static_cast<int>(spreadMs));
                
                int writePos = noteRingWritePos.load(std::memory_order_relaxed);
                noteRing[writePos % NOTE_RING_SIZE] = {static_cast<uint8_t>(pitch), static_cast<uint8_t>(inputVel), static_cast<uint8_t>(newVelocity), absoluteSampleClock};
                noteRingWritePos.store(writePos + 1, std::memory_order_relaxed);
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
    
    if (!clusterBuffer.empty()) {
        std::sort(clusterBuffer.begin(), clusterBuffer.end(), 
            [](const PendingNote& a, const PendingNote& b) { return a.pitch < b.pitch; });
        
        int splitPoint = -1;
        int maxGap = 0;
        for (size_t i = 1; i < clusterBuffer.size(); ++i) {
            int gap = clusterBuffer[i].pitch - clusterBuffer[i-1].pitch;
            if (gap >= 5 && gap > maxGap) {
                maxGap = gap;
                splitPoint = static_cast<int>(i);
            }
        }
        
        bool hasTwoHands = (splitPoint > 0 && splitPoint < static_cast<int>(clusterBuffer.size()));
        
        for (size_t i = 0; i < clusterBuffer.size(); ++i) {
            auto& note = clusterBuffer[i];
            int vel = note.velocity;
            
            if (voicing > 0) {
                float voicingScale = voicing / 100.0f * 8.0f;
                
                bool isBass = (i == 0 && clusterBuffer.size() > 1 && 
                               (clusterBuffer[1].pitch - note.pitch) >= 7);
                
                if (!isBass) {
                    int handStart, handEnd;
                    bool isLH = false;
                    
                    if (hasTwoHands) {
                        if (static_cast<int>(i) < splitPoint) {
                            handStart = 0;
                            handEnd = splitPoint - 1;
                            isLH = true;
                        } else {
                            handStart = splitPoint;
                            handEnd = static_cast<int>(clusterBuffer.size()) - 1;
                            isLH = false;
                        }
                    } else {
                        handStart = 0;
                        handEnd = static_cast<int>(clusterBuffer.size()) - 1;
                        isLH = (clusterBuffer[handEnd].pitch < 60);
                    }
                    
                    int handSize = handEnd - handStart + 1;
                    if (handSize > 1) {
                        int posInHand = static_cast<int>(i) - handStart;
                        float t = posInHand / static_cast<float>(handSize - 1);
                        
                        float cosVal = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi * t));
                        
                        float tilt = isLH ? t : (1.0f - t);
                        float curve = cosVal + 0.3f * tilt;
                        
                        int voicingAdjust = static_cast<int>((curve - 0.65f) * voicingScale);
                        vel += voicingAdjust;
                    }
                } else {
                    vel += static_cast<int>(voicingScale * 0.3f);
                }
            }
            
            vel = juce::jlimit(1, 127, vel);
            
            if (humanize > 0) {
                float humanizeScale = humanize / 100.0f;
                float phraseCurveScale = 0.4f + 0.6f * phraseCurveVal;
                float velScale = 0.35f + 0.65f * (vel / 127.0f);
                
                juce::Random noteRandom(static_cast<int64_t>(absoluteSampleClock + note.pitch));
                int perNoteJitter = static_cast<int>((noteRandom.nextFloat() * 2.0f - 1.0f) * 2.0f);
                
                int totalJitter = static_cast<int>(chordHumanizeOffset * phraseCurveScale * velScale) + perNoteJitter;
                vel += totalJitter;
                vel = juce::jlimit(1, 127, vel);
            }
            
            vel = compressVelocity(vel, compThreshold, compRatio);
            
            if (squish != 0) {
                float u = juce::jlimit(1.0f / 128.0f, 127.0f / 128.0f, vel / 128.0f);
                float logit = std::log(u / (1.0f - u));
                float k = squish * (1.0f / 20.0f);
                float s = 1.0f / (1.0f + std::exp(-(logit + k)));
                vel = juce::jlimit(1, 127, static_cast<int>(std::round(s * 128.0f)));
            }
            
            int bandIndex = getBandIndex(note.pitch);
            if (outputBandPeaks[bandIndex] < vel)
                outputBandPeaks[bandIndex] = vel;
            outputBandVelocities[bandIndex].value.store(outputBandPeaks[bandIndex], std::memory_order_relaxed);
            
            float curveVal = curveValue(phraseFrac, shapeIndex);
            float timingAmount = amountMs * curveVal;
            applyLife(pos, timingAmount);
            
            float spreadMs = 0.0f;
            if (chordSpreadMs > 0) {
                spreadMs = juce::Random::getSystemRandom().nextFloat() * chordSpreadMs;
            }
            float totalDelayMs = timingAmount + spreadMs;
            
            int delaySamples = static_cast<int>((totalDelayMs / 1000.0) * currentSampleRate);
            int64_t targetSample = note.arrivalSample + delaySamples;
            DelayedNote delayed{juce::MidiMessage::noteOn(note.channel, note.pitch, (juce::uint8)vel), targetSample, note.pitch, note.channel};
            delayQueue.push_back(delayed);
            
            int key = note.channel * 128 + note.pitch;
            activeDelayedNotes.insert(key);
            noteOffDelays[key].push_back(static_cast<int>(totalDelayMs));
            
            int writePos = noteRingWritePos.load(std::memory_order_relaxed);
            noteRing[writePos % NOTE_RING_SIZE] = {static_cast<uint8_t>(note.pitch), static_cast<uint8_t>(note.inputVel), static_cast<uint8_t>(vel), absoluteSampleClock};
            noteRingWritePos.store(writePos + 1, std::memory_order_relaxed);
        }
        
        clusterBuffer.clear();
    }
    
    auto it = delayQueue.begin();
    while (it != delayQueue.end()) {
        if (it->targetSample <= absoluteSampleClock + buffer.getNumSamples() - 1) {
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
