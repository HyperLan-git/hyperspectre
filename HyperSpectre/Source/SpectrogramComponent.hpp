#pragma once

#include "PluginProcessor.hpp"

class SpectrogramComponent : public juce::AnimatedAppComponent {
   public:
    SpectrogramComponent(TestAudioProcessor& audioProcessor);

    void paint(juce::Graphics& g) override;
    void update() override;

   private:
    TestAudioProcessor& audioProcessor;

    float fbuffer[TestAudioProcessor::fftSize / 2];
    float tbuffer[TestAudioProcessor::fftSize / 2];
    float abuffer[TestAudioProcessor::fftSize / 2];

    juce::Image render;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrogramComponent)
};