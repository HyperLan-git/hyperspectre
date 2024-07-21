#pragma once

#include "PluginProcessor.hpp"

class SpectrogramComponent : public juce::AnimatedAppComponent {
   public:
    SpectrogramComponent(TestAudioProcessor& audioProcessor);

    void paint(juce::Graphics& g) override;
    void update() override;

   private:
    TestAudioProcessor& audioProcessor;

    float fbuffer[MAX_FFT_SIZE / 2];
    float tbuffer[MAX_FFT_SIZE / 2];
    float abuffer[MAX_FFT_SIZE / 2];

    juce::Image render;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrogramComponent)
};