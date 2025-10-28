// ==============================================================================
// SETUP INSTRUCTIONS FOR PROJUCER
// ==============================================================================
/*

1. CREATE NEW JUCE PROJECT:
   - Open Projucer
   - Create new "Audio Plug-In" project named "VST1Bridge"
   - Set formats: VST3 only
   - Plugin Characteristics: Check "Plugin is a Synth" and "Plugin MIDI Input"

2. PROJECT SETTINGS:
   Main Plugin (64-bit):
   - Replace PluginProcessor.h/cpp and PluginEditor.h/cpp with code above
   - Add BridgeProtocol.h to Source/
   - In Projucer modules, ensure JUCE modules are enabled:
     * juce_audio_basics
     * juce_audio_processors
     * juce_core
     * juce_data_structures
     * juce_events
     * juce_graphics
     * juce_gui_basics

3. CREATE BRIDGE32 EXECUTABLE:
   - In Projucer, add new exporter target (Visual Studio)
   - Create folder: Source/Bridge32/
   - Add Bridge32Main.cpp to this folder
   - In Projucer, add new GUI Application project OR:
     * In same project, add a new build configuration
     * Set Win32 (x86) architecture
     * Add Bridge32Main.cpp as a separate build target

4. VST SDK SETUP:
   - Download VST 2.4 SDK from Steinberg (archive version)
   - Extract to a folder like C:/SDKs/vstsdk2.4/
   - In Projucer, add to "Header Search Paths":
     C:/SDKs/vstsdk2.4/
   - Copy these files to your project or reference them:
     * pluginterfaces/vst2.x/aeffect.h
     * pluginterfaces/vst2.x/aeffectx.h

5. BUILD CONFIGURATIONS:
   
   A. For VST1Bridge.vst3 (64-bit):
      - Visual Studio: Select x64 configuration
      - Build normally
      - Output: Builds/VisualStudio2022/x64/Release/VST3/VST1Bridge.vst3
   
   B. For VST1Bridge32.exe (32-bit):
      - Create separate Console Application project in Projucer, OR
      - In Visual Studio, add new Win32 Console project to solution
      - Add Bridge32Main.cpp
      - Link against: juce_core, juce_events (32-bit versions)
      - Build as Win32/x86
      - Output: VST1Bridge32.exe

6. DEPLOYMENT:
   Copy to your VST3 folder (e.g., C:/Program Files/Common Files/VST3/):
   - VST1Bridge.vst3/ (folder - the 64-bit plugin)
   - VST1Bridge.vst3/Contents/x86_64-win/VST1Bridge.vst3 (the actual DLL)
   - VST1Bridge32.exe (in same folder as above, or parent folder)

7. FOR FL STUDIO:
   - Rescan plugins in FL Studio
   - Add VST1Bridge from plugin list
   - Click "Load VST1 Plugin..." button
   - Browse to your 32-bit VST1 .dll file
   - Plugin should load and process audio

TROUBLESHOOTING:
- If Bridge32.exe doesn't start, check Windows Event Viewer
- Ensure both .vst3 and .exe are in same directory
- Test with known working VST1 plugins first (e.g., old Steinberg plugins)
- Check FL Studio's plugin scanner log for errors
- Use DebugView++ to see DBG() output messages

ALTERNATIVE SIMPLER SETUP (Two Separate Projects):
Instead of one complex project, create TWO Projucer projects:

Project 1: VST1Bridge (64-bit VST3)
- Audio Plugin project
- x64 only
- Contains PluginProcessor, PluginEditor, BridgeProtocol.h

Project 2: VST1Bridge32 (32-bit Console App)  
- Console Application project
- Win32/x86 only
- Contains Bridge32Main.cpp and BridgeProtocol.h
- Link minimal JUCE modules (core, events)

This is MUCH easier to set up! Both can be in same solution folder.

BUILDING:
1. Open VST1Bridge.jucer, generate VS project, build x64
2. Open VST1Bridge32.jucer, generate VS project, build Win32
3. Copy VST1Bridge32.exe next to VST1Bridge.vst3
4. Done!

TESTING:
- Load in FL Studio
- Look for "VST1 Bridge" in plugin list
- Open UI, click Load button
- Select a 32-bit VST1 DLL
- Route audio through it
- Should process audio from legacy plugin!

*/