// ==============================================================================
// FILE: PluginProcessor.cpp
// ==============================================================================
#include "PluginProcessor.h"
#include "PluginEditor.h"

VST1BridgeProcessor::VST1BridgeProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    pipeName = "VST1Bridge_" + juce::String(juce::Time::currentTimeMillis());
    startBridgeProcess();
}

VST1BridgeProcessor::~VST1BridgeProcessor()
{
    unloadVST1Plugin();
    stopBridgeProcess();
}

bool VST1BridgeProcessor::startBridgeProcess()
{
    // Get path to Bridge32.exe (should be in same folder as VST3)
    juce::File exeFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
        .getParentDirectory()
        .getChildFile("VST1Bridge32.exe");

    if (!exeFile.existsAsFile())
    {
        DBG("Bridge32.exe not found at: " + exeFile.getFullPathName());
        return false;
    }

    // Create named pipes
    pipeToChild = std::make_unique<juce::NamedPipe>();
    pipeFromChild = std::make_unique<juce::NamedPipe>();

    if (!pipeToChild->createNewPipe(pipeName + "_to", false) ||
        !pipeFromChild->createNewPipe(pipeName + "_from", false))
    {
        DBG("Failed to create named pipes");
        return false;
    }

    // Launch bridge process with pipe names as arguments
    juce::String commandLine = exeFile.getFullPathName().quoted() + " " +
        (pipeName + "_to").quoted() + " " +
        (pipeName + "_from").quoted();

    if (!bridgeProcess.start(commandLine))
    {
        DBG("Failed to start bridge process");
        return false;
    }

    // Wait for connection (timeout 5 seconds)
    int timeout = 50; // 5 seconds
    while (timeout-- > 0)
    {
        if (pipeToChild->openExisting(pipeName + "_to") &&
            pipeFromChild->openExisting(pipeName + "_from"))
        {
            DBG("Bridge process connected successfully");
            return true;
        }
        juce::Thread::sleep(100);
    }

    DBG("Bridge process connection timeout");
    stopBridgeProcess();
    return false;
}

void VST1BridgeProcessor::stopBridgeProcess()
{
    if (bridgeProcess.isRunning())
    {
        VST1Bridge::MessageHeader header;
        header.type = VST1Bridge::MessageType::Shutdown;
        header.dataSize = 0;
        header.sequenceId = messageSequence++;
        sendMessage(header);

        juce::Thread::sleep(100);
        bridgeProcess.kill();
    }

    pipeToChild.reset();
    pipeFromChild.reset();
}

bool VST1BridgeProcessor::sendMessage(const VST1Bridge::MessageHeader& header, const void* data)
{
    if (!pipeToChild || !pipeToChild->isOpen())
        return false;

    juce::ScopedLock lock(processLock);

    // Send header
    if (pipeToChild->write(&header, sizeof(header), 1000) != sizeof(header))
        return false;

    // Send data if present
    if (data && header.dataSize > 0)
    {
        if (pipeToChild->write(data, header.dataSize, 1000) != (int)header.dataSize)
            return false;
    }

    return true;
}

bool VST1BridgeProcessor::receiveResponse(VST1Bridge::ResponseMessage& response)
{
    if (!pipeFromChild || !pipeFromChild->isOpen())
        return false;

    VST1Bridge::MessageHeader header;
    if (pipeFromChild->read(&header, sizeof(header), 2000) != sizeof(header))
        return false;

    if (header.type != VST1Bridge::MessageType::Response)
        return false;

    if (pipeFromChild->read(&response, sizeof(response), 2000) != sizeof(response))
        return false;

    return true;
}

bool VST1BridgeProcessor::loadVST1Plugin(const juce::File& dllFile)
{
    if (!dllFile.existsAsFile())
        return false;

    unloadVST1Plugin();

    VST1Bridge::LoadPluginMessage loadMsg;
    dllFile.getFullPathName().copyToUTF8(loadMsg.dllPath, sizeof(loadMsg.dllPath));

    VST1Bridge::MessageHeader header;
    header.type = VST1Bridge::MessageType::LoadPlugin;
    header.dataSize = sizeof(loadMsg);
    header.sequenceId = messageSequence++;

    if (!sendMessage(header, &loadMsg))
        return false;

    VST1Bridge::ResponseMessage response;
    if (!receiveResponse(response) || !response.success)
        return false;

    pluginLoaded = true;
    loadedPluginPath = dllFile.getFullPathName();

    // Send current sample rate and block size
    if (getSampleRate() > 0)
    {
        VST1Bridge::SetSampleRateMessage srMsg;
        srMsg.sampleRate = getSampleRate();
        header.type = VST1Bridge::MessageType::SetSampleRate;
        header.dataSize = sizeof(srMsg);
        header.sequenceId = messageSequence++;
        sendMessage(header, &srMsg);
        receiveResponse(response);
    }

    return true;
}

void VST1BridgeProcessor::unloadVST1Plugin()
{
    if (!pluginLoaded)
        return;

    VST1Bridge::MessageHeader header;
    header.type = VST1Bridge::MessageType::UnloadPlugin;
    header.dataSize = 0;
    header.sequenceId = messageSequence++;

    sendMessage(header);

    VST1Bridge::ResponseMessage response;
    receiveResponse(response);

    pluginLoaded = false;
    loadedPluginPath.clear();
}

void VST1BridgeProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    if (!pluginLoaded)
        return;

    VST1Bridge::SetSampleRateMessage srMsg;
    srMsg.sampleRate = sampleRate;

    VST1Bridge::MessageHeader header;
    header.type = VST1Bridge::MessageType::SetSampleRate;
    header.dataSize = sizeof(srMsg);
    header.sequenceId = messageSequence++;

    sendMessage(header, &srMsg);

    VST1Bridge::ResponseMessage response;
    receiveResponse(response);

    VST1Bridge::SetBlockSizeMessage bsMsg;
    bsMsg.blockSize = samplesPerBlock;

    header.type = VST1Bridge::MessageType::SetBlockSize;
    header.dataSize = sizeof(bsMsg);
    header.sequenceId = messageSequence++;

    sendMessage(header, &bsMsg);
    receiveResponse(response);

    // Resume processing
    header.type = VST1Bridge::MessageType::Resume;
    header.dataSize = 0;
    header.sequenceId = messageSequence++;
    sendMessage(header);
    receiveResponse(response);
}

void VST1BridgeProcessor::releaseResources()
{
    if (!pluginLoaded)
        return;

    VST1Bridge::MessageHeader header;
    header.type = VST1Bridge::MessageType::Suspend;
    header.dataSize = 0;
    header.sequenceId = messageSequence++;

    sendMessage(header);

    VST1Bridge::ResponseMessage response;
    receiveResponse(response);
}

void VST1BridgeProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)

{
    juce::ScopedNoDenormals noDenormals;

    if (!pluginLoaded || !pipeToChild || !pipeFromChild)
    {
        buffer.clear();
        return;
    }

    int numSamples = buffer.getNumSamples();
    int numInputs = getTotalNumInputChannels();
    int numOutputs = getTotalNumOutputChannels();

    // Prepare message
    VST1Bridge::ProcessAudioMessage procMsg;
    procMsg.numSamples = numSamples;
    procMsg.numInputs = numInputs;
    procMsg.numOutputs = numOutputs;

    VST1Bridge::MessageHeader header;
    header.type = VST1Bridge::MessageType::ProcessAudio;
    header.dataSize = sizeof(procMsg) + (numSamples * numInputs * sizeof(float));
    header.sequenceId = messageSequence++;

    // Send header and message
    if (!sendMessage(header, &procMsg))
    {
        buffer.clear();
        return;
    }

    // Send audio data (interleaved)
    juce::HeapBlock<float> interleavedData;
    interleavedData.allocate(numSamples * numInputs, true);

    for (int ch = 0; ch < numInputs; ++ch)
    {
        const float* channelData = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
            interleavedData[i * numInputs + ch] = channelData[i];
    }

    if (pipeToChild->write(interleavedData, numSamples * numInputs * sizeof(float), 1000) !=
        numSamples * numInputs * (int)sizeof(float))
    {
        buffer.clear();
        return;
    }

    // Receive processed audio
    VST1Bridge::ResponseMessage response;
    if (!receiveResponse(response) || !response.success)
    {
        buffer.clear();
        return;
    }

    // Read audio data back
    if (pipeFromChild->read(interleavedData, numSamples * numOutputs * sizeof(float), 2000) !=
        numSamples * numOutputs * (int)sizeof(float))
    {
        buffer.clear();
        return;
    }

    // Deinterleave back to buffer
    for (int ch = 0; ch < numOutputs; ++ch)
    {
        float* channelData = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
            channelData[i] = interleavedData[i * numOutputs + ch];
    }
}

void VST1BridgeProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::XmlElement xml("VST1BridgeState");
    xml.setAttribute("pluginPath", loadedPluginPath);
    copyXmlToBinary(xml, destData);
}

void VST1BridgeProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));

    if (xml && xml->hasTagName("VST1BridgeState"))
    {
        juce::String path = xml->getStringAttribute("pluginPath");
        if (path.isNotEmpty())
        {
            juce::File pluginFile(path);
            if (pluginFile.existsAsFile())
                loadVST1Plugin(pluginFile);
        }
    }
}

juce::AudioProcessorEditor* VST1BridgeProcessor::createEditor()
{
    return new VST1BridgeEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VST1BridgeProcessor();
}
