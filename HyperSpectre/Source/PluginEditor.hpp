/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.hpp"

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
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    TestAudioProcessor& audioProcessor;

    float fbuffer[TestAudioProcessor::points];
    float tbuffer[TestAudioProcessor::points];
    float abuffer[TestAudioProcessor::points];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestAudioProcessorEditor)
};
