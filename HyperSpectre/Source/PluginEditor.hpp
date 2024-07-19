#pragma once

#include "PluginProcessor.hpp"
#include "SpectrogramComponent.hpp"
#include "KnobComponent.hpp"

class TestAudioProcessorEditor : public juce::AudioProcessorEditor {
   public:
    TestAudioProcessorEditor(TestAudioProcessor&);

    void resized() override;

   private:
    SpectrogramComponent spectrogram;
    KnobComponent timeKnob;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestAudioProcessorEditor)
};
