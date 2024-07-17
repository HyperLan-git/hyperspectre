/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "PluginProcessor.hpp"
#include "SpectrogramComponent.hpp"

//==============================================================================
/**
 */
class TestAudioProcessorEditor : public juce::AudioProcessorEditor {
   public:
    TestAudioProcessorEditor(TestAudioProcessor&);
    ~TestAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

   private:
   SpectrogramComponent spectrogram;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestAudioProcessorEditor)
};
