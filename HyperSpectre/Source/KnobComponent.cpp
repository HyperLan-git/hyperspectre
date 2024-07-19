#include "KnobComponent.hpp"

ParamListener::ParamListener(juce::AudioParameterFloat* param) : param(param) {}

void ParamListener::sliderValueChanged(juce::Slider* slider) {
    param->setValueNotifyingHost(slider->getValue());
}

void ParamListener::sliderDragStarted(juce::Slider* slider) {
    param->beginChangeGesture();
}

void ParamListener::sliderDragEnded(juce::Slider* slider) {
    param->endChangeGesture();
}

KnobComponent::KnobComponent(juce::AudioParameterFloat* param, double min,
                             double max, double step)
    : paramListener(param) {
    addAndMakeVisible(knob);

    knob.setLookAndFeel(&lf);
    knob.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    knob.setTextBoxStyle(juce::Slider::TextBoxAbove, true, 100, 20);

    knob.addListener(&paramListener);

    knob.setRange(min, max, step);

    setSize(100, 100);
}
KnobComponent::~KnobComponent() { knob.setLookAndFeel(nullptr); }

void KnobComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::black);
}

double KnobComponent::getValue() const { return knob.getValue(); }

void KnobComponent::resized() { knob.setBounds(0, 0, 100, 100); }
