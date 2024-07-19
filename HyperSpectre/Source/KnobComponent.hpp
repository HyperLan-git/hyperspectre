#pragma once

#include <JuceHeader.h>

class ParamListener : public juce::Slider::Listener {
   public:
    ParamListener(juce::AudioParameterFloat* param);

    void sliderValueChanged(juce::Slider* slider) override;

    void sliderDragStarted(juce::Slider* slider) override;

    void sliderDragEnded(juce::Slider* slider) override;

   private:
    juce::AudioParameterFloat* param;
};

class KnobComponent : public juce::Component {
   public:
    KnobComponent(juce::AudioParameterFloat* param, double min = 0,
                  double max = 10, double step = .001);
    ~KnobComponent() override;

    void paint(juce::Graphics& g) override;

    void resized() override;

    double getValue() const;

   private:
    juce::Slider knob;
    juce::LookAndFeel_V4 lf;

    ParamListener paramListener;
};