# VST1HOSTFORVST3
This is a concept to host VST1 plugins in VST3 host plugin so that old VST1 plugins can be usable again
Current build error

Rebuild started at 4:00 AM...
1>------ Rebuild All started: Project: VST1toVST3_VST3ManifestHelper, Configuration: Debug x64 ------
2>------ Rebuild All started: Project: VST1toVST3_SharedCode, Configuration: Debug x64 ------
1>juce_VST3ManifestHelper.cpp
2>PluginEditor.cpp
2>PluginProcessor.cpp
2>BinaryData.cpp
2>include_juce_audio_basics.cpp
2>include_juce_audio_devices.cpp
2>include_juce_audio_formats.cpp
2>include_juce_audio_plugin_client_ARA.cpp
2>include_juce_audio_processors_ara.cpp
2>include_juce_audio_processors_lv2_libs.cpp
2>include_juce_audio_utils.cpp
2>include_juce_core_CompilationTime.cpp
2>include_juce_data_structures.cpp
2>include_juce_dsp.cpp
2>include_juce_events.cpp
2>include_juce_graphics_Harfbuzz.cpp
2>include_juce_gui_extra.cpp
2>include_juce_osc.cpp
1>VST1toVST3_VST3ManifestHelper.vcxproj -> C:\Development\JUCE\VST1toVST3\Builds\VisualStudio2022\x64\Debug\VST3 Manifest Helper\juce_vst3_helper.exe
2>C:\Development\JUCE\VST1toVST3\Source\PluginEditor.cpp(21,33): warning C4996: 'juce::Font::Font': Use the constructor that takes a FontOptions argument
2>include_juce_audio_processors.cpp
2>include_juce_core.cpp
2>include_juce_graphics.cpp
2>include_juce_gui_basics.cpp
2>include_juce_graphics_Sheenbidi.c
2>VST1toVST3_SharedCode.vcxproj -> C:\Development\JUCE\VST1toVST3\Builds\VisualStudio2022\x64\Debug\Shared Code\VST1toVST3.lib
2>Done building project "VST1toVST3_SharedCode.vcxproj".
3>------ Rebuild All started: Project: VST1toVST3_VST3, Configuration: Debug x64 ------
3>include_juce_audio_plugin_client_VST3.cpp
3>C:\Development\JUCE\modules\juce_audio_plugin_client\juce_audio_plugin_client_VST3.cpp(99,1): error C1189: #error:  You may have a conflict with parameter automation between VST2 and VST3 versions of your plugin. See the comment above for more details.
3>(compiling source file '../../JuceLibraryCode/include_juce_audio_plugin_client_VST3.cpp')
3>Done building project "VST1toVST3_VST3.vcxproj" -- FAILED.
========== Rebuild All: 2 succeeded, 1 failed, 0 skipped ==========
---
Need to use Juce 8 version font forms need to fix. The VST2 vs VST3 conflict is something I need to look ino more changing c++ preprocessor didn't fix it.
