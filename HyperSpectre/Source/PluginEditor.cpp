#include "PluginProcessor.hpp"
#include "PluginEditor.hpp"

TestAudioProcessorEditor::TestAudioProcessorEditor(TestAudioProcessor& p)
    : AudioProcessorEditor(&p),
      spectrogram(p),
      timeKnob(p.getTimeScaleParam(), .5, 3) {
    setSize(800, 700);

    this->addAndMakeVisible(spectrogram);
    this->addAndMakeVisible(timeKnob);
}

void TestAudioProcessorEditor::resized() {
    timeKnob.setBounds({0, 0, 100, 100});

    spectrogram.setBounds({0, 100, 800, 600});
}
