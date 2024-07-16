/*
    ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

    ==============================================================================
*/

#include "PluginProcessor.hpp"
#include "PluginEditor.hpp"

//==============================================================================
TestAudioProcessorEditor::TestAudioProcessorEditor(TestAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p) {
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(800, 600);
}

TestAudioProcessorEditor::~TestAudioProcessorEditor() {}

//==============================================================================
void TestAudioProcessorEditor::paint(juce::Graphics& g) {
    constexpr auto scopeSize = TestAudioProcessor::scopeSize;
    constexpr auto fftSize = TestAudioProcessor::fftSize;
    constexpr auto points = TestAudioProcessor::points;
    // (Our component is opaque, so we must completely fill the background with
    // a solid colour)

    g.setFont(juce::FontOptions(15.0f));
    // g.drawFittedText("Hello World!", getLocalBounds(),
    //                 juce::Justification::centred, 1);

    if (!this->audioProcessor.trylockFFT()) {
        this->repaint(getLocalBounds());
        return;
    }
    const auto fdata = this->audioProcessor.getFreqData();
    const auto tdata = this->audioProcessor.getTimeData();
    std::memcpy(fbuffer, fdata, points * sizeof(float));
    std::memcpy(tbuffer, tdata, points * sizeof(float));
    this->audioProcessor.unlockFFT();

    float lastTimeProcessed = this->audioProcessor.getLastTimeProcessed();

    g.fillAll(
        getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    g.setColour(juce::Colours::yellow);
    int w = g.getClipBounds().getWidth(), h = g.getClipBounds().getHeight();
    for (size_t i = 0; i < points; i++) {
        if (fbuffer[i] == 0) continue;
        float x = tbuffer[i] - lastTimeProcessed;
        if (x < -2) continue;
        x = (x + 2) * w / 2;
        float y = std::exp(std::log(1.0f - fbuffer[i] / (float)20000) * 2);
        y *= h;
        std::cout << fbuffer[i] << ',' << y << std::endl;
        if (!juce::juce_isfinite(x) || !juce::juce_isfinite(y)) continue;
        g.drawEllipse(juce::Rectangle<float>(x, y, 1, 1), 2);
    }

    this->repaint(getLocalBounds());
}

void TestAudioProcessorEditor::resized() {
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
