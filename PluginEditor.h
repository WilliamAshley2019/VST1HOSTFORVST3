// ==============================================================================
// FILE: PluginEditor.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class VST1BridgeEditor : public juce::AudioProcessorEditor
{
public:
    VST1BridgeEditor(VST1BridgeProcessor&);
    ~VST1BridgeEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void loadButtonClicked();

    VST1BridgeProcessor& processor;
    juce::TextButton loadButton;
    juce::Label statusLabel;
    juce::Label pathLabel;
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VST1BridgeEditor)
};