#include "SpectrogramComponent.hpp"

SpectrogramComponent::SpectrogramComponent(TestAudioProcessor& audioProcessor)
    : audioProcessor(audioProcessor),
      render(juce::Image::PixelFormat::ARGB, 800, 600, true) {
    setSize(800, 600);
    setFramesPerSecond(60);
}

const float b1 = std::log10(1 / 20000.) / (1 - 20000),
            a1 = 20000. / std::pow(10, b1 * 20000.);
float linToLogF(float val) { return 20000 + std::log10(val / a1) / b1; }

float lastTime = 0;
void SpectrogramComponent::update() {
    constexpr auto fftSize = TestAudioProcessor::fftSize;
    constexpr auto points = fftSize / 2;

    while (!this->audioProcessor.trylockFFT()) {
        juce::Thread::sleep(5);
    }
    float lastTimeProcessed = this->audioProcessor.getLastTimeProcessed();
    const auto fdata = this->audioProcessor.getFreqData();
    const auto tdata = this->audioProcessor.getTimeData();
    const auto adata = this->audioProcessor.getAmpData();
    std::memcpy(fbuffer, fdata, points * sizeof(float));
    std::memcpy(tbuffer, tdata, points * sizeof(float));
    std::memcpy(abuffer, adata, points * sizeof(float));

    this->audioProcessor.unlockFFT();

    juce::Graphics g(render);

    int w = g.getClipBounds().getWidth(), h = g.getClipBounds().getHeight();

    g.setOpacity(1);
    float scale = this->audioProcessor.getTimeScale();

    if ((lastTime - lastTimeProcessed) < 0) {
        g.drawImageAt(render, (lastTime - lastTimeProcessed) * w / scale, 0);
        g.fillRect(w - (int)((lastTimeProcessed - lastTime) * w / scale), 0,
                   (int)((lastTimeProcessed - lastTime) * w / scale), h);
    }
    lastTime = lastTimeProcessed;

    g.setColour(juce::Colours::yellow);
    for (size_t i = 1; i < points; i++) {
        if (fbuffer[i] <= 0) continue;
        float x = tbuffer[i] - lastTime;
        x = (x + scale) * w / scale;
        float y = 2.5 - (linToLogF(fbuffer[i]) * 1.25 / 20000);
        y *= h;
        g.setOpacity(abuffer[i] > 1 ? 1 : abuffer[i]);
        g.getInternalContext().drawLineWithThickness(
            juce::Line<float>(x, y, x - 10 / scale, y), 2);
    }
}

// TODO make the image scroll left for smoothness
void SpectrogramComponent::paint(juce::Graphics& g) {
    g.setOpacity(1);
    g.drawImageAt(render, 0, 0);
}