cmake_minimum_required(VERSION 3.22)
project(RiddimLab VERSION 0.4.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# Only the header artwork (logo + title) is embedded. Panels, knobs, meters and
# tape reels are drawn in code, so they stay sharp at any display scale.
juce_add_binary_data(RiddimLabArtwork
    SOURCES
        Assets/Artwork/header_art.png
)

juce_add_plugin(RiddimLab

    COMPANY_NAME "Eclectic Prodz"
    BUNDLE_ID "com.eclecticprodz.riddimlab"
    PLUGIN_MANUFACTURER_CODE EcPr
    PLUGIN_CODE RdLb
    FORMATS VST3 AU
    PRODUCT_NAME "Riddim Lab"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(RiddimLab PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)
target_compile_features(RiddimLab PUBLIC cxx_std_17)
target_compile_definitions(RiddimLab PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)
target_link_libraries(RiddimLab PRIVATE
    RiddimLabArtwork
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags
)
juce_generate_juce_header(RiddimLab)
