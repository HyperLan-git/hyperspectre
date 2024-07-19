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
      window(fftSize, juce::dsp::WindowingFunction<float>::hann),
      fft(fftOrder - 1),
      timeScale(new juce::AudioParameterFloat({"timeScale", 1}, "Time scale",
                                              .2, 5, 2)) {
    constexpr float pi = juce::MathConstants<float>().pi;
    for (int i = 0; i < fftSize; i++) {
        // just x * h(x)
        tWindow[i] =
            i * (0.5 - 0.5 * std::cos(2 * pi * i / (fftSize - 1))) / fftSize;
        // derive -.5 * cos(2*pi*x / (size-1))
        // => pi * sin(2*pi*x / (size-1)) / (size-1)
        dhtWindow[i] =
            pi * std::sin(2 * pi * i / (fftSize - 1)) / (fftSize - 1);
    }

    addParameter(timeScale);
}

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

    /*
    for (int ch = 0; ch < inputs; ch++) {
        float* data = buffer.getWritePointer(ch);
        double t = std::fmod(
            *(this->getPlayHead()->getPosition()->getTimeInSeconds()), 3);
        for (int i = 0; i < samples; i++) {
            data[i] = sin(t * 3.141596 * 2 * 440 * sin(t * 3.141596));
            t += 1. / sampleRate;
        }
    }*/

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
    fftLock.unlock();

    {
        std::memcpy(fftTh, fftTemp, sz);
        std::memcpy(fftDht, fftTemp, sz);

        juce::FloatVectorOperations::multiply(fftTh, tWindow, (int)fftSize);
        juce::FloatVectorOperations::multiply(fftDht, dhtWindow, (int)fftSize);

        fft.performRealOnlyForwardTransform(fftTh);
        fft.performRealOnlyForwardTransform(fftDht);
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
    float t = *(this->getPlayHead()->getPosition()->getTimeInSeconds());
    constexpr float lg = std::log((float)fftSize);
    std::memset(fftFrequencies, 0, fftSize * sizeof(float) / 2);
    for (int i = 1; i < fftSize / 2 - 1; i++) {
        int idx = i * 2;
        float mag =
            fftData[idx] * fftData[idx] + fftData[idx + 1] * fftData[idx + 1];
        float f = i * sampleRate / fftSize * 2;

        fftAmps[i] = (std::log(mag) - 2 * lg + 15) / 40;
        if (fftAmps[i] < 0) continue;

        if (f >= 20000 || f <= 10) continue;

        fftTimes[i] =
            t - .3 +
            (fftTh[idx] * fftData[idx] + fftTh[idx + 1] * fftData[idx + 1]) /
                mag;
        // XXX figure out why this formula is like that (probably something to
        // convert rads/s to hz)
        fftFrequencies[i] = f + (fftDht[idx] * fftData[idx + 1] -
                                 fftDht[idx + 1] * fftData[idx]) /
                                    mag * 2048 * 16;
        // if (fftAmps[i] > 1) std::cout << fftFrequencies[i] << '\n';
    }
    this->lastTimeProcessed = t;

    fftLock.unlock();

    auto editor = this->getActiveEditor();
    if (editor != nullptr) {
        juce::MessageManager::callAsync(
            [editor]() { editor->repaint(editor->getBounds()); });
    }
}

bool TestAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* TestAudioProcessor::createEditor() {
    return new TestAudioProcessorEditor(*this);
}

//==============================================================================
void TestAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {}

void TestAudioProcessor::setStateInformation(const void* data,
                                             int sizeInBytes) {}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new TestAudioProcessor();
}
