
// ==============================================================================
// FILE: Bridge32/Bridge32Main.cpp (32-bit executable)
// ==============================================================================
#include <JuceHeader.h>
#include "../BridgeProtocol.h"

// VST SDK includes (you need to download VST 2.4 SDK)
#include "pluginterfaces/vst2.x/aeffect.h"
#include "pluginterfaces/vst2.x/aeffectx.h"

class VST1BridgeApp
{
public:
    VST1BridgeApp(const juce::String& pipeNameTo, const juce::String& pipeNameFrom)
    {
        pipeIn = std::make_unique<juce::NamedPipe>();
        pipeOut = std::make_unique<juce::NamedPipe>();

        if (!pipeIn->openExisting(pipeNameTo) || !pipeOut->openExisting(pipeNameFrom))
        {
            DBG("Failed to connect to parent pipes");
            return;
        }

        DBG("Bridge32 connected to parent process");
        messageLoop();
    }

    ~VST1BridgeApp()
    {
        unloadPlugin();
    }

private:
    void messageLoop()
    {
        while (true)
        {
            VST1Bridge::MessageHeader header;
            if (pipeIn->read(&header, sizeof(header), -1) != sizeof(header))
                break;

            handleMessage(header);
        }
    }

    void handleMessage(const VST1Bridge::MessageHeader& header)
    {
        VST1Bridge::ResponseMessage response;
        response.success = false;
        response.errorMessage[0] = '\0';

        switch (header.type)
        {
        case VST1Bridge::MessageType::LoadPlugin:
        {
            VST1Bridge::LoadPluginMessage msg;
            pipeIn->read(&msg, sizeof(msg), 1000);
            response.success = loadPlugin(msg.dllPath);
            break;
        }

        case VST1Bridge::MessageType::UnloadPlugin:
            unloadPlugin();
            response.success = true;
            break;

        case VST1Bridge::MessageType::SetSampleRate:
        {
            VST1Bridge::SetSampleRateMessage msg;
            pipeIn->read(&msg, sizeof(msg), 1000);
            if (effect)
            {
                dispatcher(effSetSampleRate, 0, 0, nullptr, (float)msg.sampleRate);
                response.success = true;
            }
            break;
        }

        case VST1Bridge::MessageType::SetBlockSize:
        {
            VST1Bridge::SetBlockSizeMessage msg;
            pipeIn->read(&msg, sizeof(msg), 1000);
            if (effect)
            {
                dispatcher(effSetBlockSize, 0, msg.blockSize, nullptr, 0.0f);
                response.success = true;
            }
            break;
        }

        case VST1Bridge::MessageType::Resume:
            if (effect)
            {
                dispatcher(effMainsChanged, 0, 1, nullptr, 0.0f);
                response.success = true;
            }
            break;

        case VST1Bridge::MessageType::Suspend:
            if (effect)
            {
                dispatcher(effMainsChanged, 0, 0, nullptr, 0.0f);
                response.success = true;
            }
            break;

        case VST1Bridge::MessageType::ProcessAudio:
        {
            response.success = processAudio(header);
            return; // processAudio sends its own response
        }

        case VST1Bridge::MessageType::Shutdown:
            response.success = true;
            sendResponse(response);
            return;

        default:
            break;
        }

        sendResponse(response);
    }

    bool loadPlugin(const char* dllPath)
    {
        unloadPlugin();

        vstLib = std::make_unique<juce::DynamicLibrary>();
        if (!vstLib->open(dllPath))
        {
            DBG("Failed to load DLL: " + juce::String(dllPath));
            return false;
        }

        // Try "main" first (VST1), then "VSTPluginMain" (VST2 fallback)
        typedef AEffect* (*VstMainProc)(audioMasterCallback);
        VstMainProc mainProc = (VstMainProc)vstLib->getFunction("main");
        if (!mainProc)
            mainProc = (VstMainProc)vstLib->getFunction("VSTPluginMain");

        if (!mainProc)
        {
            DBG("VST entry point not found");
            vstLib.reset();
            return false;
        }

        effect = mainProc(hostCallbackStatic);
        if (!effect || effect->magic != kEffectMagic)
        {
            DBG("Invalid VST plugin");
            vstLib.reset();
            effect = nullptr;
            return false;
        }

        // Store instance pointer for callback
        effect->resvd1 = (VstIntPtr)this;

        dispatcher(effOpen, 0, 0, nullptr, 0.0f);

        DBG("VST1 plugin loaded successfully");
        return true;
    }

    void unloadPlugin()
    {
        if (effect)
        {
            dispatcher(effMainsChanged, 0, 0, nullptr, 0.0f);
            dispatcher(effClose, 0, 0, nullptr, 0.0f);
            effect = nullptr;
        }
        vstLib.reset();
    }

    VstIntPtr dispatcher(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
    {
        if (!effect || !effect->dispatcher)
            return 0;
        return effect->dispatcher(effect, opcode, index, value, ptr, opt);
    }

    static VstIntPtr VSTCALLBACK hostCallbackStatic(AEffect* effect, VstInt32 opcode,
        VstInt32 index, VstIntPtr value,
        void* ptr, float opt)
    {
        VST1BridgeApp* instance = (VST1BridgeApp*)effect->resvd1;
        return instance ? instance->hostCallback(opcode, index, value, ptr, opt) : 0;
    }

    VstIntPtr hostCallback(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
    {
        switch (opcode)
        {
        case audioMasterVersion: return 2400;
        case audioMasterCurrentId: return effect ? effect->uniqueID : 0;
        case audioMasterGetSampleRate: return (VstIntPtr)44100;
        case audioMasterGetBlockSize: return 512;
        case audioMasterGetNumAudioIns: return 2;
        case audioMasterGetNumAudioOuts: return 2;
        default: return 0;
        }
    }

    bool processAudio(const VST1Bridge::MessageHeader& header)
    {
        VST1Bridge::ProcessAudioMessage msg;
        pipeIn->read(&msg, sizeof(msg), 1000);

        if (!effect)
            return false;

        // Read interleaved audio data
        juce::HeapBlock<float> audioData;
        int dataSize = msg.numSamples * msg.numInputs;
        audioData.allocate(dataSize, true);

        if (pipeIn->read(audioData, dataSize * sizeof(float), 2000) != dataSize * (int)sizeof(float))
            return false;

        // Prepare non-interleaved buffers for VST
        juce::HeapBlock<float*> inputs, outputs;
        juce::HeapBlock<float> inputBuffer, outputBuffer;

        inputs.calloc(msg.numInputs);
        outputs.calloc(msg.numOutputs);
        inputBuffer.allocate(msg.numSamples * msg.numInputs, true);
        outputBuffer.allocate(msg.numSamples * msg.numOutputs, true);

        // Deinterleave input
        for (int ch = 0; ch < msg.numInputs; ++ch)
        {
            inputs[ch] = inputBuffer + (ch * msg.numSamples);
            for (int i = 0; i < msg.numSamples; ++i)
                inputs[ch][i] = audioData[i * msg.numInputs + ch];
        }

        for (int ch = 0; ch < msg.numOutputs; ++ch)
            outputs[ch] = outputBuffer + (ch * msg.numSamples);

        // Process
        if (effect->flags & effFlagsCanReplacing)
            effect->processReplacing(effect, inputs, outputs, msg.numSamples);
        else
            effect->process(effect, inputs, outputs, msg.numSamples);

        // Send response
        VST1Bridge::ResponseMessage response;
        response.success = true;
        sendResponse(response);

        // Interleave and send output
        for (int ch = 0; ch < msg.numOutputs; ++ch)
            for (int i = 0; i < msg.numSamples; ++i)
                audioData[i * msg.numOutputs + ch] = outputs[ch][i];

        pipeOut->write(audioData, msg.numSamples * msg.numOutputs * sizeof(float), 2000);
        return true;
    }

    void sendResponse(const VST1Bridge::ResponseMessage& response)
    {
        VST1Bridge::MessageHeader header;
        header.type = VST1Bridge::MessageType::Response;
        header.dataSize = sizeof(response);
        header.sequenceId = 0;

        pipeOut->write(&header, sizeof(header), 1000);
        pipeOut->write(&response, sizeof(response), 1000);
    }

    std::unique_ptr<juce::NamedPipe> pipeIn, pipeOut;
    std::unique_ptr<juce::DynamicLibrary> vstLib;
    AEffect* effect = nullptr;
};

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        DBG("Usage: VST1Bridge32.exe <pipeNameTo> <pipeNameFrom>");
        return 1;
    }

    juce::String pipeNameTo = argv[1];
    juce::String pipeNameFrom = argv[2];

    VST1BridgeApp app(pipeNameTo, pipeNameFrom);

    return 0;
}

