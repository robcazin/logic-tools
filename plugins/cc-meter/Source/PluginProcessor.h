#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <deque>
#include <map>
#include <set>

class RubatoProcessor : public juce::AudioProcessor
{
public:
    RubatoProcessor();
    ~RubatoProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Rubato"; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getApvts() { return apvts; }
    
    int getPhraseCurveValue() const { return phraseCurveValue.load(std::memory_order_relaxed); }
    int getPhraseTimingIntensity() const { return phraseTimingIntensity.load(std::memory_order_relaxed); }
    
    struct BandVelocity {
        std::atomic<int> value{0};
    };
    std::array<BandVelocity, 16> inputBandVelocities;
    std::array<BandVelocity, 16> outputBandVelocities;
    
    struct NoteEvent {
        uint8_t pitch;
        uint8_t inputVel;
        uint8_t outputVel;
        int64_t timestamp;
    };
    
    static constexpr int NOTE_RING_SIZE = 256;
    std::array<NoteEvent, NOTE_RING_SIZE> noteRing;
    std::atomic<int> noteRingWritePos{0};
    
    std::array<NoteEvent, NOTE_RING_SIZE> getNoteRing();
    int getNoteRingWritePos() const { return noteRingWritePos.load(std::memory_order_relaxed); }

private:
    juce::AudioProcessorValueTreeState apvts;
    std::atomic<int> phraseCurveValue{0};
    std::atomic<int> phraseTimingIntensity{0};
    
    double currentSampleRate = 44100.0;
    int64_t absoluteSampleClock = 0;
    double lastPpqPosition = -1.0;
    bool wasPlaying = false;
    
    struct DelayedNote {
        juce::MidiMessage message;
        int64_t targetSample;
        int noteNumber;
        int channel;
    };
    std::deque<DelayedNote> delayQueue;
    std::set<int> activeDelayedNotes;
    
    struct PendingNote {
        int pitch;
        int channel;
        int velocity;
        int inputVel;
        int samplePos;
        int64_t arrivalSample;
    };
    std::vector<PendingNote> clusterBuffer;
    int64_t lastClusterFlushSample = 0;
    
    struct NoteOffDelay {
        int delayMs;
    };
    std::map<int, std::deque<int>> noteOffDelays;
    
    double anchorBeat = 1.0;
    bool transportWarm = false;
    int warmBlocks = 0;
    std::map<int, int> lastCCValues;
    
    std::array<int, 16> inputBandPeaks;
    std::array<int, 16> outputBandPeaks;
    std::array<int, 16> inputBandDecay;
    std::array<int, 16> outputBandDecay;
    
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    float calculatePhraseFraction(juce::AudioPlayHead::PositionInfo& pos);
    float curveValue(float t, int shapeIndex);
    void applyLife(juce::AudioPlayHead::PositionInfo& pos, float& amount);
    int compressVelocity(int vel, int threshold, float ratio);
    int getBandIndex(int pitch);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RubatoProcessor)
};
