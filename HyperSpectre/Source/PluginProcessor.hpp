/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <JucePluginDefines.h>
#include <cmath>

typedef unsigned long size_t;

//==============================================================================
class TestAudioProcessor : public juce::AudioProcessor {
   public:
    enum {
        fftOrder = 11,            // [1]
        fftSize = 1 << fftOrder,  // [2]
        scopeSize = 512           // [3]
    };
    //==============================================================================
    TestAudioProcessor();
    ~TestAudioProcessor() noexcept override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    inline const float* const getData() const { return this->scopeData; }
    inline const float* const getSampleData() const { return this->fftTemp; }

    inline const float* const getFreqData() const {
        return this->fftFrequencies;
    }
    inline const float* const getTimeData() const { return this->fftTimes; }

    inline const float getLastTimeProcessed() {
        return this->lastTimeProcessed;
    }

    inline void lockFFT() { this->fftLock.lock(); }
    inline bool trylockFFT() { return this->fftLock.try_lock(); }
    inline void unlockFFT() { this->fftLock.unlock(); }

    static constexpr size_t points = 500;

   private:
    juce::dsp::WindowingFunction<float> window;
    float dhtWindow[fftSize];
    float tWindow[fftSize];

    juce::dsp::FFT fft;

    std::mutex fftLock;

    float scopeData[scopeSize];
    float fftTemp[fftSize];

    float fftData[fftSize];
    // windowing is t*h(t)
    float fftTh[fftSize];
    // windowing is dh(t)/dt
    float fftDht[fftSize];

    float fftTimes[points];
    float fftFrequencies[points];

    size_t pointIdx = 0;

    float lastTimeProcessed = 0;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestAudioProcessor)
};
