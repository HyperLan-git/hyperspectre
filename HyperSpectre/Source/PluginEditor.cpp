/*
    ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

    ==============================================================================
*/

#include "PluginProcessor.hpp"
#include "PluginEditor.hpp"

//==============================================================================
TestAudioProcessorEditor::TestAudioProcessorEditor(TestAudioProcessor& p)
    : AudioProcessorEditor(&p), spectrogram(p) {
    setSize(800, 600);
    this->addChildComponent(spectrogram, 1);
}

TestAudioProcessorEditor::~TestAudioProcessorEditor() {}

//==============================================================================

void TestAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0, 0, 0));
    this->spectrogram.paint(g);
}

void TestAudioProcessorEditor::resized() {}
