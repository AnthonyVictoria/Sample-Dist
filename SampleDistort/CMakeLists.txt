cmake_minimum_required(VERSION 3.22)

project(SampleDistort VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED JUCE_PATH)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
        set(JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/JUCE")
    else()
        message(FATAL_ERROR
            "No encuentro JUCE. Clona el framework en ./JUCE o configura -DJUCE_PATH=/ruta/a/JUCE")
    endif()
endif()

add_subdirectory(${JUCE_PATH} JUCE)

juce_add_plugin(SampleDistort
    COMPANY_NAME "SampleDistort"
    BUNDLE_ID com.sampledistort.plugin
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE SmDt
    PLUGIN_CODE SpDs
    FORMATS VST3 Standalone
    PRODUCT_NAME "Sample Distort"
    VST3_CATEGORIES Fx Distortion)

target_sources(SampleDistort
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
        juce_generate_juce_header(SampleDistort)

target_compile_definitions(SampleDistort
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(SampleDistort
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
