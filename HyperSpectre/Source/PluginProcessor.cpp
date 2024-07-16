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
        // derive -.5 * cos(2*pi*x) / (size-1) => pi * sin(2*pi*x) / (size-1)
        dhtWindow[i] = pi * std::sin(2 * pi * i) / (fftSize - 1);
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

int TestAudioProcessor::getNumPrograms() {
    return 1;  // NB: some hosts don't cope very well if you tell them there are
               // 0 programs, so this should be at least 1, even if you're not
               // really implementing programs.
}

int TestAudioProcessor::getCurrentProgram() { return 0; }

void TestAudioProcessor::setCurrentProgram(int index) {}

const juce::String TestAudioProcessor::getProgramName(int index) { return {}; }

void TestAudioProcessor::changeProgramName(int index,
                                           const juce::String& newName) {}

//==============================================================================
void TestAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void TestAudioProcessor::releaseResources() {
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool TestAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const {
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

        // This checks if the input layout matches the output layout
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

    int samples = buffer.getNumSamples();
    double sampleRate = this->getSampleRate();

    constexpr auto M = juce::MathConstants<float>();
    long double dt = 1;
    dt /= sampleRate;
    juce::Random rand;
    for (int channel = 0; channel < inputs; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        long double t = std::fmod(
            *(this->getPlayHead()->getPosition()->getTimeInSeconds()), 1);
        for (int i = 0; i < samples; ++i) {
            // channelData[i] = (rand.nextFloat() * 2 - 1);
            constexpr float f = 10000;
            // channelData[i] = std::fmod(t * f, 1) * 2 - 1;
            // channelData[i] = sin(t * M.twoPi * (f + sin(t * 6) * 500));
            channelData[i] = sin(t * M.twoPi * f);
            channelData[i] += sin(t * M.twoPi * 30);
            channelData[i] += sin(t * M.twoPi * 300);
            channelData[i] += sin(t * M.twoPi * 3000);
            channelData[i] /= 4;
            /*channelData[i] += sin(t * M.twoPi * f * 3) / 9;
            channelData[i] += sin(t * M.twoPi * f * 5) / 25;
            channelData[i] += sin(t * M.twoPi * f * 7) / 49;
            channelData[i] += sin(t * M.twoPi * f * 9) / 81;*/
            t += dt;
        }
    }

    const float* channelData = buffer.getReadPointer(0);
    channelData = buffer.getWritePointer(0);
    if (inputs > 0) {
        int len = samples;
        if (len >= fftSize)
            std::memcpy(fftTemp, channelData + (len - fftSize), sz);
        else {
            int savedBlockSize = fftSize - len;
            std::memmove(fftTemp, fftTemp + len,
                         savedBlockSize * sizeof(float));
            std::memcpy(fftTemp + savedBlockSize, channelData,
                        len * sizeof(float));
        }
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
     * w' = w + img(Xdh * conj(X) / abs(X)^2)
     * w' = w + (-real(Xdh) * img(X) + img(Xdh) * real(X)) / abs(X)^2
     */
    float t = *(this->getPlayHead()->getPosition()->getTimeInSeconds());
    for (int i = 0; i < fftSize / 2; i++) {
        int idx = i * 2;
        float mag =
            fftData[idx] * fftData[idx] + fftData[idx + 1] * fftData[idx + 1];
        if (mag < 100000) continue;

        float f = i * sampleRate / fftSize;
        if (f >= 20000) continue;

        fftTimes[pointIdx] = t - (fftTh[idx] * fftData[idx] +
                                  fftTh[idx + 1] * fftData[idx + 1]) /
                                     mag;
        fftFrequencies[pointIdx] = f - (fftDht[idx] * fftData[idx + 1] -
                                        fftDht[idx + 1] * fftData[idx]) /
                                           mag;
        (++pointIdx) %= points;
        // std::cout << (f - fftFrequencies[i]) << '\n';
    }
    this->lastTimeProcessed = t;

    constexpr float mindB = -70.0f, maxdB = 0;
    for (int i = 0; i < scopeSize; ++i) {
        float skewedProportionX =
            1.0f - std::exp(std::log(1.0f - i / (float)scopeSize) * 0.2f);
        int fftDataIndex =
            juce::jlimit<int>(0, fftSize, skewedProportionX * fftSize);

        float val =
            std::sqrt(fftData[fftDataIndex] * fftData[fftDataIndex] +
                      fftData[fftDataIndex + 1] * fftData[fftDataIndex + 1]);
        float level = juce::jmap(
            juce::jlimit(mindB, maxdB,
                         juce::Decibels::gainToDecibels(val) -
                             juce::Decibels::gainToDecibels<float>(fftSize)),
            mindB, maxdB, 0.0f, 1.0f);

        scopeData[i] = level;
    }

    fftLock.unlock();
}

//==============================================================================
bool TestAudioProcessor::hasEditor() const {
    return true;  // (change this to false if you choose to not supply an
                  // editor)
}

juce::AudioProcessorEditor* TestAudioProcessor::createEditor() {
    return new TestAudioProcessorEditor(*this);
}

//==============================================================================
void TestAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void TestAudioProcessor::setStateInformation(const void* data,
                                             int sizeInBytes) {
    // You should use this method to restore your parameters from this memory
    // block, whose contents will have been created by the getStateInformation()
    // call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new TestAudioProcessor();
}
