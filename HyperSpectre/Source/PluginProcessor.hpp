#pragma once

#include <JuceHeader.h>
#include <JucePluginDefines.h>
#include <cmath>

typedef unsigned long size_t;

constexpr int MAX_FFT_ORDER = 13;
constexpr int MAX_FFT_SIZE = 1 << MAX_FFT_ORDER;

#define GET_PARAM_NORMALIZED(param) (param->convertTo0to1(*param))
#define SET_PARAM_NORMALIZED(param, value) \
    param->setValueNotifyingHost(param->convertTo0to1(value))

class TestAudioProcessor : public juce::AudioProcessor {
   public:
    TestAudioProcessor();
    ~TestAudioProcessor() = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    inline const float* const getSampleData() const { return this->fftTemp; }

    inline const float* const getFreqData() const {
        return this->fftFrequencies;
    }
    inline const float* const getTimeData() const { return this->fftTimes; }
    inline const float* const getAmpData() const { return this->fftAmps; }

    inline const float getLastTimeProcessed() {
        return this->lastTimeProcessed;
    }

    inline void lockFFT() { this->fftLock.lock(); }
    inline bool trylockFFT() { return this->fftLock.try_lock(); }
    inline void unlockFFT() { this->fftLock.unlock(); }

    inline float getTimeScale() const { return this->timeScale->get(); }
    inline int getFFTSize() const { return 1 << this->fftSizeParam->get(); }
    inline int getFFTOrder() const { return this->fftSizeParam->get(); }

    inline juce::AudioParameterFloat* getTimeScaleParam() {
        return this->timeScale;
    }
    inline juce::AudioParameterFloat* getMinAmpParam() { return this->minAmp; }
    inline juce::AudioParameterFloat* getFadeAmpParam() {
        return this->fadeAmp;
    }
    inline juce::AudioParameterInt* getFFTOrderParam() {
        return this->fftSizeParam;
    }

   private:
    class FFTListener : public juce::AudioProcessorParameter::Listener {
       public:
        FFTListener(TestAudioProcessor& proc);

        void parameterValueChanged(int parameterIndex, float newValue) override;
        void parameterGestureChanged(int parameterIndex,
                                     bool gestureIsStarting) override;

       private:
        TestAudioProcessor& proc;
    };

    FFTListener fftListener;

    std::shared_ptr<juce::dsp::WindowingFunction<float>> window;
    float dhtWindow[MAX_FFT_SIZE];
    float tWindow[MAX_FFT_SIZE];

    // TODO optimise this shit cuz a dsp library is apparently not built for fft
    std::shared_ptr<juce::dsp::FFT> fft;

    std::mutex fftLock;

    float fftFrequencies[MAX_FFT_SIZE / 2];
    float fftAmps[MAX_FFT_SIZE / 2];
    float fftTimes[MAX_FFT_SIZE / 2];

    float fftTemp[MAX_FFT_SIZE];

    float fftData[MAX_FFT_SIZE];
    // windowing is t*h(t)
    float fftTh[MAX_FFT_SIZE];
    // windowing is dh(t)/dt
    float fftDht[MAX_FFT_SIZE];

    float lastTimeProcessed = 0;

    juce::AudioParameterFloat* timeScale;
    juce::AudioParameterFloat* minAmp;
    juce::AudioParameterFloat* fadeAmp;
    juce::AudioParameterInt* fftSizeParam;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestAudioProcessor)
};
