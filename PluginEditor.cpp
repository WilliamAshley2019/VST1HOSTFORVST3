// ==============================================================================
// FILE: PluginEditor.cpp
// ==============================================================================
#include "PluginEditor.h"

VST1BridgeEditor::VST1BridgeEditor(VST1BridgeProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(400, 200);

    loadButton.setButtonText("Load VST1 Plugin...");
    loadButton.onClick = [this] { loadButtonClicked(); };
    addAndMakeVisible(loadButton);

    statusLabel.setText("No plugin loaded", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusLabel);

    pathLabel.setText("", juce::dontSendNotification);
    pathLabel.setJustificationType(juce::Justification::centred);
    pathLabel.setFont(juce::Font(12.0f));
    addAndMakeVisible(pathLabel);

    if (processor.isPluginLoaded())
    {
        statusLabel.setText("Plugin Loaded", juce::dontSendNotification);
        pathLabel.setText(processor.getLoadedPluginPath(), juce::dontSendNotification);
    }
}

VST1BridgeEditor::~VST1BridgeEditor()
{
}

void VST1BridgeEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    g.drawFittedText("VST1 Bridge", getLocalBounds().removeFromTop(40),
        juce::Justification::centred, 1);
}

void VST1BridgeEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(40); // Title space

    loadButton.setBounds(area.removeFromTop(40).reduced(50, 5));
    area.removeFromTop(10);
    statusLabel.setBounds(area.removeFromTop(30));
    area.removeFromTop(5);
    pathLabel.setBounds(area.removeFromTop(60));
}

void VST1BridgeEditor::loadButtonClicked()
{
    auto chooserFlags = juce::FileBrowserComponent::openMode |
        juce::FileBrowserComponent::canSelectFiles;

    fileChooser = std::make_unique<juce::FileChooser>("Select VST1 Plugin (32-bit DLL)",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.dll");

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser)
        {
            auto selectedFile = chooser.getResult();

            if (selectedFile != juce::File{})
            {
                if (processor.loadVST1Plugin(selectedFile))
                {
                    statusLabel.setText("Plugin Loaded Successfully!", juce::dontSendNotification);
                    pathLabel.setText(selectedFile.getFullPathName(), juce::dontSendNotification);
                }
                else
                {
                    statusLabel.setText("Failed to load plugin", juce::dontSendNotification);
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                        "Load Error",
                        "Failed to load VST1 plugin. Make sure it's a valid 32-bit VST1 DLL.");
                }
            }
        });
}