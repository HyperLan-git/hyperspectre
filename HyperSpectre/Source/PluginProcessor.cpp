/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

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
      window(fftSize, juce::dsp::WindowingFunction<float>::hann),
      fft(fftOrder - 1)
#endif
{
    constexpr float pi = juce::MathConstants<float>().pi;
    for (int i = 0; i < fftSize; i++) {
        // just x * h(x)
        tWindow[i] =
            i * (0.5 - 0.5 * std::cos(2 * pi * i / (fftSize - 1))) / fftSize;
        // derive -.5 * cos(2*pi*x / (size-1))
        // => pi * sin(2*pi*x / (size-1)) / (size-1)
        dhtWindow[i] = std::sin(2 * pi * i / (fftSize - 1));
    }
}

TestAudioProcessor::~TestAudioProcessor() {}

//==============================================================================
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

void TestAudioProcessor::setCurrentProgram(int index) {}

const juce::String TestAudioProcessor::getProgramName(int index) { return {}; }

void TestAudioProcessor::changeProgramName(int index,
                                           const juce::String& newName) {}

//==============================================================================
void TestAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
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
    int inputs = getTotalNumInputChannels();
    int outputs = getTotalNumOutputChannels();
    constexpr size_t sz = fftSize * sizeof(float);

    if (this->getPlayHead() == nullptr ||
        !this->getPlayHead()->getPosition().hasValue() ||
        !this->getPlayHead()->getPosition()->getTimeInSeconds().hasValue())
        return;

    int samples = buffer.getNumSamples();
    long double sampleRate = this->getSampleRate();

    const float* channelData = buffer.getReadPointer(0);

    int len = samples;
    if (len >= fftSize)
        std::memcpy(fftTemp, channelData + (len - fftSize), sz);
    else {
        int savedBlockSize = fftSize - len;
        std::memmove(fftTemp, fftTemp + len, savedBlockSize * sizeof(float));
        std::memcpy(fftTemp + savedBlockSize, channelData, len * sizeof(float));
    }

    if (!fftLock.try_lock()) return;

    {
        std::memcpy(fftData, fftTemp, sz);
        window.multiplyWithWindowingTable(fftData, fftSize);
        fft.performRealOnlyForwardTransform(fftData);
    }

    {
        std::memcpy(fftTh, fftTemp, sz);
        std::memcpy(fftDht, fftTemp, sz);

        juce::FloatVectorOperations::multiply(fftTh, tWindow, (int)fftSize);
        juce::FloatVectorOperations::multiply(fftDht, dhtWindow, (int)fftSize);

        fft.performRealOnlyForwardTransform(fftTh);
        fft.performRealOnlyForwardTransform(fftDht);
    }

    /*
     * t' = t - real(Xth * conj(X) / abs(X)^2)
     * t' = t - (real(Xth) * real(X) + img(Xth) * img(X)) / abs(X)^2
     *
     * om' = om + img(Xdh * conj(X) / abs(X)^2)
     * om' = om + (-real(Xdh) * img(X) + img(Xdh) * real(X)) / abs(X)^2
     */
    float t = *(this->getPlayHead()->getPosition()->getTimeInSeconds());
    for (int i = 0; i < fftSize / 2 - 1; i++) {
        int idx = i * 2;
        float mag =
            fftData[idx] * fftData[idx] + fftData[idx + 1] * fftData[idx + 1];
        float f = i * sampleRate / fftSize * 2;

        fftFrequencies[i] = 0;
        if (std::log(mag / fftSize) < -5) continue;

        if (f >= 20000) continue;

        fftTimes[i] = t - (fftTh[idx] * fftData[idx] +
                           fftTh[idx + 1] * fftData[idx + 1]) /
                              mag;
        fftFrequencies[i] = f + (fftDht[idx] * fftData[idx + 1] -
                                 fftDht[idx + 1] * fftData[idx]) /
                                    mag;
        fftAmps[i] = (std::log(mag / fftSize) + 5);
    }
    this->lastTimeProcessed = t;

    fftLock.unlock();
    if (this->getActiveEditor() != nullptr)
        this->getActiveEditor()->repaint(this->getActiveEditor()->getBounds());
}

//==============================================================================
bool TestAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* TestAudioProcessor::createEditor() {
    return new TestAudioProcessorEditor(*this);
}

//==============================================================================
void TestAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {}

void TestAudioProcessor::setStateInformation(const void* data,
                                             int sizeInBytes) {}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new TestAudioProcessor();
}
