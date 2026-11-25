cmake_minimum_required(VERSION 3.22)
project(Phonemorph3056 VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Phonemorph3056
    COMPANY_NAME           "Phonemorph"
    PLUGIN_MANUFACTURER_CODE Phmo
    PLUGIN_CODE            Ph30
    FORMATS                VST3
    PRODUCT_NAME           "PHONEMORPH 3056"
    IS_SYNTH               FALSE
    NEEDS_MIDI_INPUT       FALSE
    NEEDS_MIDI_OUTPUT      FALSE
    IS_MIDI_EFFECT         FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(Phonemorph3056 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(Phonemorph3056 PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_USE_OGGVORBIS=0
    JUCE_USE_MP3AUDIOFORMAT=0
    JUCE_USE_FLAC=0)

target_link_libraries(Phonemorph3056 PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra)

if(MSVC)
    target_compile_options(Phonemorph3056 PRIVATE /O2 /W4)
else()
    target_compile_options(Phonemorph3056 PRIVATE -O3 -ffast-math -Wall -Wextra)
endif()

set_property(TARGET Phonemorph3056 PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
