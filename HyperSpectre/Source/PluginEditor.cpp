/*
    ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

    ==============================================================================
*/

#include "PluginProcessor.hpp"
#include "PluginEditor.hpp"

//==============================================================================
TestAudioProcessorEditor::TestAudioProcessorEditor(TestAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      render(juce::Image::PixelFormat::ARGB, 800, 600, true) {
    setSize(800, 600);
}

TestAudioProcessorEditor::~TestAudioProcessorEditor() {}

//==============================================================================

const float b1 = std::log10(1 / 20000.) / (1 - 20000),
            a1 = 20000. / std::pow(10, b1 * 20000.);
float linToLogF(float val) { return 20000 + std::log10(val / a1) / b1; }

float lastTime = 0;
void TestAudioProcessorEditor::paint(juce::Graphics& g2) {
    constexpr auto fftSize = TestAudioProcessor::fftSize;
    constexpr auto points = fftSize / 2;

    g2.setOpacity(1);

    if (!this->audioProcessor.trylockFFT()) {
        g2.drawImageAt(render, 0, 0);
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

    juce::Graphics g(render);

    int w = g.getClipBounds().getWidth(), h = g.getClipBounds().getHeight();

    g.setOpacity(1);

    g.drawImageAt(render, (lastTime - lastTimeProcessed) * w / 2, 0);
    g.fillRect(w - (int)((lastTimeProcessed - lastTime) * w / 2), 0,
               (int)((lastTimeProcessed - lastTime) * w / 2), h);
    lastTime = lastTimeProcessed;

    float line = 2;
    g.setColour(juce::Colours::yellow);
    for (size_t i = 0; i < points; i++) {
        if (fbuffer[i] == 0) continue;
        float x = tbuffer[i] - lastTimeProcessed;
        x = (x + 2) * w / 2;
        float y = 4 - (linToLogF(fbuffer[i]) * 2 / 20000);
        // std::cout << fbuffer[i] << ',' << y << '\n';
        y *= h;
        g.setOpacity(abuffer[i] > 1 ? 1 : abuffer[i]);
        g.getInternalContext().drawLineWithThickness(
            juce::Line<float>(x, y, ((x + line) < w - 2) ? x + line : w - 2, y),
            2);
    }
    g2.drawImageAt(render, 0, 0);

    this->repaint(getLocalBounds());
}

void TestAudioProcessorEditor::resized() {}
