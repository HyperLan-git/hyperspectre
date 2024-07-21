#include "PluginEditor.hpp"
#include "PluginProcessor.hpp"

#include <algorithm>

//==============================================================================
TestAudioProcessor::TestAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
              ),
#endif
      timeScale(new juce::AudioParameterFloat({"timeScale", 1}, "Time scale",
                                              .2f, 5.f, 2.f)),
      minAmp(new juce::AudioParameterFloat({"minAmp", 1}, "Minimal amplitude",
                                           0.f, 3.f, 1.f)),
      fadeAmp(new juce::AudioParameterFloat({"fadeAmp", 1}, "Amplitude fading",
                                            0.f, 30.f, 10.f)),
      fftSizeParam(new juce::AudioParameterInt({"fftSize", 1}, "FFT size", 5,
                                               MAX_FFT_ORDER, 10)),
      fftListener(*this),
      fft(std::make_shared<juce::dsp::FFT>(1)),
      window(std::make_shared<juce::dsp::WindowingFunction<float>>(
          2, juce::dsp::WindowingFunction<float>::rectangular)),
      dhtWindow() {

    addParameter(timeScale);
    addParameter(minAmp);
    addParameter(fadeAmp);
    addParameter(fftSizeParam);

    fftSizeParam->addListener(&fftListener);

    fftListener.parameterValueChanged(fftSizeParam->getParameterIndex(),
                                      (float)fftSizeParam->get());
}

const juce::String TestAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool TestAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool TestAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool TestAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double TestAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int TestAudioProcessor::getNumPrograms() { return 1; }

int TestAudioProcessor::getCurrentProgram() { return 0; }

void TestAudioProcessor::setCurrentProgram(int index) {
    (void) index; 
}

const juce::String TestAudioProcessor::getProgramName(int index) {
    (void) index;
    return {};
}

void TestAudioProcessor::changeProgramName(int index,
                                           const juce::String& newName) {
    (void)index;
    (void)newName;
}

void TestAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    (void) sampleRate;
    (void) samplesPerBlock;
}

void TestAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool TestAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const {
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void TestAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;

    (void) midiMessages;

    int inputs = getTotalNumInputChannels();
    int fftOrder = this->getFFTOrder(), fftSize = 1 << fftOrder;
    if (inputs < 1) return;
    if (fftOrder > MAX_FFT_ORDER || fftOrder <= 1) return;
    if (this->getPlayHead() == nullptr ||
        !this->getPlayHead()->getPosition().hasValue() ||
        !this->getPlayHead()->getPosition()->getTimeInSeconds().hasValue())
        return;

    size_t sz = fftSize * sizeof(float);

    int samples = buffer.getNumSamples();
    double sampleRate = this->getSampleRate();

    const float* channelData = buffer.getReadPointer(0);

    int len = samples;
    if (len >= fftSize)
        std::memcpy(fftTemp, channelData + (len - fftSize), sz);
    else {
        int savedBlockSize = fftSize - len;
        std::memmove(fftTemp, fftTemp + len, savedBlockSize * sizeof(float));
        std::memcpy(fftTemp + savedBlockSize, channelData, len * sizeof(float));
    }

    {
        auto win = this->window;
        auto ft = this->fft;
        if (!win || !fft) return;

        std::memcpy(fftData, fftTemp, sz);
        std::memcpy(fftTh, fftTemp, sz);
        std::memcpy(fftDht, fftTemp, sz);

        win->multiplyWithWindowingTable(fftData, fftSize);
        juce::FloatVectorOperations::multiply(fftTh, tWindow, (int)fftSize);
        juce::FloatVectorOperations::multiply(fftDht, dhtWindow, (int)fftSize);

        ft->performRealOnlyForwardTransform(fftData);
        ft->performRealOnlyForwardTransform(fftTh);
        ft->performRealOnlyForwardTransform(fftDht);
    }

    if (!fftLock.try_lock()) return;
    /*
     * t' = t - real(Xth * conj(X) / abs(X)^2)
     * t' = t - (real(Xth) * real(X) + img(Xth) * img(X)) / abs(X)^2
     *
     * om' = om + img(Xdh * conj(X) / abs(X)^2)
     * om' = om + (-real(Xdh) * img(X) + img(Xdh) * real(X)) / abs(X)^2
     * 2pi*f' = 2pi*f + (-real(Xdh) * img(X) + img(Xdh) * real(X)) / abs(X)^2
     * f' = f + (-real(Xdh) * img(X) + img(Xdh) * real(X)) / abs(X)^2 / (2pi)
     */
    double t = *(this->getPlayHead()->getPosition()->getTimeInSeconds());
    float lg = std::log((float)fftSize);

    std::memset(fftFrequencies, 0, fftSize * sizeof(float) / 2);

    float min = std::exp(*this->minAmp), fade = (*this->fadeAmp) + 1;
    double freqStep = sampleRate / fftSize * (fftOrder % 2 ? 2 : 1);
    const auto M = juce::MathConstants<float>();
    for (int i = 1; i < fftSize / 2 - 1; i++) {
        int idx = i * 2;
        float f = (float)(i * freqStep);
        if (f >= 20000 || f <= 10) continue;

        float real = fftData[idx], img = fftData[idx + 1];
        float mag = real * real + img * img;

        fftAmps[i] = (std::log(mag) - 2 * lg + 25 - min) / fade;
        if (fftAmps[i] <= 0) continue;

        fftTimes[i] = (float)(t + (fftTh[idx] * real + fftTh[idx + 1] * img) / mag);
        // XXX figure out why this formula is like that (probably something to
        // convert rads/s to hz)
        float freq = f + (fftDht[idx] * img - fftDht[idx + 1] * real) * 10000 *
                             M.pi / mag;
        if (freq > 0 && freq < 20000) fftFrequencies[i] = freq;
    }
    this->lastTimeProcessed = (float)(t + .3);
    fftLock.unlock();
}

TestAudioProcessor::FFTListener::FFTListener(TestAudioProcessor& proc)
    : proc(proc) {}

void TestAudioProcessor::FFTListener::parameterValueChanged(int parameterIndex,
                                                            float newValue) {
    constexpr float pi = juce::MathConstants<float>().pi;
    (void) parameterIndex;
    if (newValue <= 1)
        newValue = proc.getFFTOrderParam()->convertFrom0to1(newValue);
    int fftOrder = proc.getFFTOrder(), fftSize = 1 << fftOrder;
    if (fftSize == proc.fft->getSize() / 2) return;
    if (fftOrder <= 1 || fftOrder > MAX_FFT_ORDER) return;
    for (int i = 0; i < fftSize; i++) {
        // just x * h(x)
        proc.tWindow[i] =
            i * (0.5f - 0.5f * std::cos(2 * pi * i / (fftSize - 1))) / fftSize;
        // derive -.5 * cos(2*pi*x / (size-1))
        // => pi * sin(2*pi*x / (size-1)) / (size-1)
        proc.dhtWindow[i] =
            pi * std::sin(2 * pi * i / (fftSize - 1)) / (fftSize - 1);
    }

    proc.fft = std::make_shared<juce::dsp::FFT>(fftOrder - 1);

    proc.window = std::make_shared<juce::dsp::WindowingFunction<float>>(
        fftSize, juce::dsp::WindowingFunction<float>::hann);
}

void TestAudioProcessor::FFTListener::parameterGestureChanged(
    int parameterIndex, bool gestureIsStarting) {
    (void) parameterIndex;
    (void) gestureIsStarting;
}

bool TestAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* TestAudioProcessor::createEditor() {
    return new TestAudioProcessorEditor(*this);
}

void TestAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    juce::MemoryOutputStream stream(destData, true);
    stream.writeFloat(GET_PARAM_NORMALIZED(timeScale));
    stream.writeFloat(GET_PARAM_NORMALIZED(minAmp));
    stream.writeFloat(GET_PARAM_NORMALIZED(fadeAmp));
    stream.writeFloat(fftSizeParam->convertTo0to1((float)*fftSizeParam));
}

void TestAudioProcessor::setStateInformation(const void* data,
                                             int sizeInBytes) {
    juce::MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes),
                                   false);
    timeScale->setValueNotifyingHost(stream.readFloat());
    minAmp->setValueNotifyingHost(stream.readFloat());
    fadeAmp->setValueNotifyingHost(stream.readFloat());
    fftSizeParam->setValueNotifyingHost(stream.readFloat());
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new TestAudioProcessor();
}
