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

const float b1 = std::log10(1 / 20000.) / (1 - 20000),
            a1 = 20000. / std::pow(10, b1 * 20000.);
float linToLogF(float val) { return 20000 + std::log10(val / a1) / b1; }

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
    const auto adata = this->audioProcessor.getAmpData();
    std::memcpy(fbuffer, fdata, points * sizeof(float));
    std::memcpy(tbuffer, tdata, points * sizeof(float));
    std::memcpy(abuffer, adata, points * sizeof(float));
    this->audioProcessor.unlockFFT();

    float lastTimeProcessed = this->audioProcessor.getLastTimeProcessed();

    g.setOpacity(1);
    g.fillAll(
        getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    g.setColour(juce::Colours::yellow);
    int w = g.getClipBounds().getWidth(), h = g.getClipBounds().getHeight();
    for (size_t i = 0; i < points; i++) {
        if (fbuffer[i] == 0) continue;
        float x = tbuffer[i] - lastTimeProcessed;
        x = (x + 2) * w / 2;
        float y = 4 - (linToLogF(fbuffer[i]) * 2 / 20000);
        // std::cout << fbuffer[i] << ',' << y << '\n';
        y *= h;
        g.setOpacity(abuffer[i] > 1 ? 1 : abuffer[i]);
        g.getInternalContext().drawLineWithThickness(
            juce::Line<float>(x, y, x + 1, y), 2);
    }

    this->repaint(getLocalBounds());
}

void TestAudioProcessorEditor::resized() {
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
