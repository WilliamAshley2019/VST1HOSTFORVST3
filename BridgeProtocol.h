// ==============================================================================
// FILE: BridgeProtocol.h (Shared between 64-bit and 32-bit processes)
// ==============================================================================
#pragma once
#include <cstdint>

namespace VST1Bridge {

    enum class MessageType : uint32_t {
        LoadPlugin,
        UnloadPlugin,
        SetSampleRate,
        SetBlockSize,
        ProcessAudio,
        ProcessMidi,
        SetParameter,
        GetParameter,
        Suspend,
        Resume,
        Shutdown,
        Response
    };

    struct MessageHeader {
        MessageType type;
        uint32_t dataSize;
        uint32_t sequenceId;
    };

    struct LoadPluginMessage {
        char dllPath[512];
    };

    struct SetSampleRateMessage {
        double sampleRate;
    };

    struct SetBlockSizeMessage {
        int32_t blockSize;
    };

    struct ProcessAudioMessage {
        int32_t numSamples;
        int32_t numInputs;
        int32_t numOutputs;
        // Followed by float audio data
    };

    struct SetParameterMessage {
        int32_t index;
        float value;
    };

    struct GetParameterMessage {
        int32_t index;
    };

    struct ResponseMessage {
        bool success;
        char errorMessage[256];
        union {
            float paramValue;
            int32_t intValue;
        };
    };

} // namespace VST1Bridge
