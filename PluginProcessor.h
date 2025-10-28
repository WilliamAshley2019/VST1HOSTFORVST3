// ==============================================================================
// FILE: PluginProcessor.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>
#include "BridgeProtocol.h"

class VST1BridgeProcessor : public juce::AudioProcessor
{
public:
    VST1BridgeProcessor();
    ~VST1BridgeProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VST1 Bridge"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // VST1 Plugin Management
    bool loadVST1Plugin(const juce::File& dllFile);
    void unloadVST1Plugin();
    bool isPluginLoaded() const { return pluginLoaded; }
    juce::String getLoadedPluginPath() const { return loadedPluginPath; }

private:
    bool startBridgeProcess();
    void stopBridgeProcess();
    bool sendMessage(const VST1Bridge::MessageHeader& header, const void* data = nullptr);
    bool receiveResponse(VST1Bridge::ResponseMessage& response);

    juce::ChildProcess bridgeProcess;
    std::unique_ptr<juce::NamedPipe> pipeToChild;
    std::unique_ptr<juce::NamedPipe> pipeFromChild;

    juce::String pipeName;
    bool pluginLoaded = false;
    juce::String loadedPluginPath;

    juce::CriticalSection processLock;
    uint32_t messageSequence = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VST1BridgeProcessor)
};
