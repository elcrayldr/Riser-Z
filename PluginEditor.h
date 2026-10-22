#pragma once

#include "PluginProcessor.h"
#include <array>

class RiserLabEditor : public juce::AudioProcessorEditor,
                       private juce::Timer
{
public:
    explicit RiserLabEditor (RiserLabProcessor&);
    ~RiserLabEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct CaptureButton : public juce::Button
    {
        CaptureButton() : juce::Button ("Capture") {}
        void paintButton (juce::Graphics&, bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;
        void setActive (bool shouldBeActive) { active = shouldBeActive; repaint(); }
        bool active = false;
    };

    // Zona que muestra la onda del riser y permite arrastrarlo al DAW.
    struct DragArea : public juce::Component
    {
        void setData (std::vector<float> newPeaks, const juce::File& newFile);
        void paint (juce::Graphics&) override;
        void mouseDrag (const juce::MouseEvent&) override;

        std::vector<float> peaks;
        juce::File file;
    };

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    void timerCallback() override;
    void onCaptureClicked();
    void onGenerateClicked();

    RiserLabProcessor& proc;

    CaptureButton captureButton;
    juce::TextButton generateButton;
    juce::Label statusLabel;
    juce::ComboBox presetBox;
    std::array<Knob, 8> knobs;
    juce::ToggleButton cutToggle;
    std::unique_ptr<ButtonAttachment> cutAttachment;
    DragArea dragArea;

    juce::Rectangle<int> titleArea;
    juce::Rectangle<int> subtitleArea;
    juce::String lastMessage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RiserLabEditor)
};
