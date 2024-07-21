#include "PluginProcessor.hpp"
#include "PluginEditor.hpp"

TestAudioProcessorEditor::TestAudioProcessorEditor(TestAudioProcessor& p)
    : AudioProcessorEditor(&p),
      spectrogram(p),
      timeKnob(p.getTimeScaleParam()),
      minAmp(p.getMinAmpParam()),
      fadeAmp(p.getFadeAmpParam()),
      fftKnob(p.getFFTOrderParam()) {
    setSize(800, 700);

    this->addAndMakeVisible(spectrogram);
    this->addAndMakeVisible(timeKnob);
    this->addAndMakeVisible(minAmp);
    this->addAndMakeVisible(fadeAmp);
    this->addAndMakeVisible(fftKnob);
}

void TestAudioProcessorEditor::resized() {
    timeKnob.setBounds({0, 0, 100, 100});
    minAmp.setBounds({100, 0, 100, 100});
    fadeAmp.setBounds({200, 0, 100, 100});
    fftKnob.setBounds({300, 0, 100, 100});

    spectrogram.setBounds({0, 100, 800, 600});
}
